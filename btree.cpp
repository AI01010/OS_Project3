/**
 * CS 4348.501 
 * Ashraful Islam
 * OS Project 3
 * 
 * B-Tree implementation in C++
 * Interactive program that creates and manages index files: create, insert, and search operations.
 *
 * B-Tree properties (for minimum degree t):
 * 1. Every node has at most 2t - 1 keys and at least t - 1 keys.
 * 2. Every internal node (except root) has at least t children.
 * 3. All leaves are at the same level.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <endian.h>  // For htobe64 and be64toh
#include <algorithm>
#include <stdint.h>

// Check if system is big-endian
bool is_bigendian()
{
    int x = 1;
    return ((uint8_t *)&x)[0] != 1;
}

// Reverse bytes for endianness conversion if needed
uint64_t reverse_bytes(uint64_t x)
{
    uint8_t dest[sizeof(uint64_t)];
    uint8_t *source = (uint8_t*)&x;
    for (int c = 0; c < sizeof(uint64_t); c++)
    {
        dest[c] = source[sizeof(uint64_t)-c-1];
    }
    return *(uint64_t *)dest;
}

// B-Tree Node structure
struct BTreeNode
{
    uint64_t block_id;
    uint64_t parent_id;
    uint64_t num_keys;
    uint64_t keys[19];
    uint64_t values[19];
    uint64_t children[20];

    BTreeNode() : block_id(0), parent_id(0), num_keys(0)
    {
        std::memset(keys, 0, sizeof(keys));
        std::memset(values, 0, sizeof(values));
        std::memset(children, 0, sizeof(children));
    }
};

// Header structure
struct Header
{
    char magic[8];
    uint64_t root_block_id;
    uint64_t next_block_id;

    Header() : root_block_id(0), next_block_id(1)
    {
        std::memcpy(magic, "4348PRJ3", 8);
    }
};

// Function to write header to file
void write_header(std::fstream &file, const Header &header)
{
    file.seekp(0);
    file.write(header.magic, 8);
    file.write(reinterpret_cast<const char*>(&header.root_block_id), 8);
    file.write(reinterpret_cast<const char*>(&header.next_block_id), 8);
}

// Function to read header from file
void read_header(std::fstream &file, Header &header)
{
    file.seekg(0);
    file.read(header.magic, 8);
    file.read(reinterpret_cast<char*>(&header.root_block_id), 8);
    file.read(reinterpret_cast<char*>(&header.next_block_id), 8);
}

// Function to write node to file
void write_node(std::fstream &file, const BTreeNode &node, uint64_t block_id)
{
    file.seekp(block_id * 512);
    file.write(reinterpret_cast<const char*>(&node.block_id), 8);
    file.write(reinterpret_cast<const char*>(&node.parent_id), 8);
    file.write(reinterpret_cast<const char*>(&node.num_keys), 8);
    file.write(reinterpret_cast<const char*>(node.keys), 152);
    file.write(reinterpret_cast<const char*>(node.values), 152);
    file.write(reinterpret_cast<const char*>(node.children), 160);
}

// Function to read node from file
void read_node(std::fstream &file, BTreeNode &node, uint64_t block_id)
{
    file.seekg(block_id * 512);
    file.read(reinterpret_cast<char*>(&node.block_id), 8);
    file.read(reinterpret_cast<char*>(&node.parent_id), 8);
    file.read(reinterpret_cast<char*>(&node.num_keys), 8);
    file.read(reinterpret_cast<char*>(node.keys), 152);
    file.read(reinterpret_cast<char*>(node.values), 152);
    file.read(reinterpret_cast<char*>(node.children), 160);
}

// ------------------------ Command functions ------------------------ 

// Function to create a new index file
void create(const std::string& filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error creating file: " << filename << std::endl;
        return;
    }
    Header header;
    std::fstream fstream_file(filename, std::ios::in | std::ios::out | std::ios::binary);
    write_header(fstream_file, header);
    file.close();
}

// Function to insert a key/value pair into the index file
void insert(const std::string& filename, uint64_t key, uint64_t value)
{
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    Header header;
    read_header(file, header);

    // Insert key/value into B-Tree (simplified for demonstration)
    BTreeNode root;
    if (header.root_block_id == 0)
    {
        root.block_id = header.next_block_id++;
        root.keys[0] = key;
        root.values[0] = value;
        root.num_keys = 1;
        header.root_block_id = root.block_id;
        write_node(file, root, root.block_id);
        write_header(file, header);
    }
    else
    {
        read_node(file, root, header.root_block_id);
        if (root.num_keys < 19)
        {
            root.keys[root.num_keys] = key;
            root.values[root.num_keys] = value;
            root.num_keys++;
            write_node(file, root, root.block_id);
        }
        else
        {
            std::cerr << "Node is full, splitting not implemented in this example." << std::endl;
        }
    }
    file.close();
}

// Function to search for a key in the index file
void search(const std::string& filename, uint64_t key)
{
    std::fstream file(filename, std::ios::in | std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    Header header;
    read_header(file, header);

    BTreeNode node;
    read_node(file, node, header.root_block_id);
    for (uint64_t i = 0; i < node.num_keys; ++i)
    {
        if (node.keys[i] == key)
        {
            std::cout << "Found: " << key << " -> " << node.values[i] << std::endl;
            file.close();
            return;
        }
    }
    std::cerr << "Key not found." << std::endl;
    file.close();
}

// Function to load key/value pairs from a CSV file into the index file
void load(const std::string& index_filename, const std::string& csv_filename)
{
    std::fstream index_file(index_filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!index_file)
    {
        std::cerr << "Error opening index file: " << index_filename << std::endl;
        return;
    }
    std::ifstream csv_file(csv_filename);
    if (!csv_file)
    {
        std::cerr << "Error opening CSV file: " << csv_filename << std::endl;
        return;
    }

    Header header;
    read_header(index_file, header);

    std::string line;
    while (std::getline(csv_file, line))
    {
        std::stringstream ss(line);
        std::string key_str, value_str;
        if (std::getline(ss, key_str, ',') && std::getline(ss, value_str, ','))
        {
            uint64_t key = std::stoull(key_str);
            uint64_t value = std::stoull(value_str);
            insert(index_filename, key, value);
        }
    }

    csv_file.close();
    index_file.close();
}

// Function to print index from file
void print(const std::string& filename)
{
    std::fstream file(filename, std::ios::in | std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    Header header;
    read_header(file, header);

    // Check if index is empty
    if (header.root_block_id == 0)
    {
        std::cout << "Index is empty." << std::endl;
        file.close();
        return;
    }

    BTreeNode node;
    read_node(file, node, header.root_block_id);
    if (node.num_keys == 0)
    {
        std::cout << "Index is empty." << std::endl;
        file.close();
        return;
    }

    // Print keys and values
    for (uint64_t i = 0; i < node.num_keys; ++i)
    {
        std::cout << node.keys[i] << " -> " << node.values[i] << std::endl;
    }

    file.close();
}

// Function to extract key/value pairs to a CSV file
void extract(const std::string& index_filename, const std::string& csv_filename)
{
    std::fstream index_file(index_filename, std::ios::in | std::ios::binary);
    if (!index_file)
    {
        std::cerr << "Error opening index file: " << index_filename << std::endl;
        return;
    }
    std::ofstream csv_file(csv_filename);
    if (!csv_file)
    {
        std::cerr << "Error creating CSV file: " << csv_filename << std::endl;
        return;
    }

    Header header;
    read_header(index_file, header);

    BTreeNode node;
    read_node(index_file, node, header.root_block_id);
    for (uint64_t i = 0; i < node.num_keys; ++i)
    {
        csv_file << node.keys[i] << "," << node.values[i] << std::endl;
    }

    csv_file.close();
    index_file.close();
}


int main(int argc, char* argv[])
{
    // take command line arguments
    // check if the arguments are valid
    
    if (argc < 2)
    {
        std::cerr << "Invalid command" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    if (command == "create")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: project3 create <fname>" << std::endl;
            return 1;
        }
        create(argv[2]);
    }
    else if (command == "insert")
    {
        if (argc != 5)
        {
            std::cerr << "Usage: project3 insert <fname> <key> <value>" << std::endl;
            return 1;
        }
        uint64_t key = std::stoull(argv[3]);
        uint64_t value = std::stoull(argv[4]);
        insert(argv[2], key, value);
    }
    else if (command == "search")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 search <fname> <key>" << std::endl;
            return 1;
        }
        uint64_t key = std::stoull(argv[3]);
        search(argv[2], key);
    }
    else if (command == "load")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 load <fname> <input.csv>" << std::endl;
            return 1;
        }
        load(argv[2], argv[3]);
    }
    else if (command == "print")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: project3 print <fname>" << std::endl;
            return 1;
        }
        print(argv[2]);
    }
    else if (command == "extract")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 extract <fname> <output.csv>" << std::endl;
            return 1;
        }
        extract(argv[2], argv[3]);
    }
    return 0;
}