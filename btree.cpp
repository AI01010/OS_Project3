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
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>

const int BLOCK_SIZE = 512;
const int MIN_DEGREE = 10; // max keys: 19, children: 20

int is_big_endian()
{
    int x = 1;
    return ((uint8_t *)&x)[0] != 1;
}

uint64_t reverse_bytes(uint64_t num)
{
    uint8_t dest[sizeof(uint64_t)];
    uint8_t *src = (uint8_t *)&num;
    for (int i = 0; i < (int)sizeof(uint64_t); i++)
    {
        dest[i] = src[sizeof(uint64_t) - i - 1];
    }
    uint64_t rev;
    memcpy(&rev, dest, sizeof(uint64_t));
    return rev;
}

uint64_t to_big_endian(uint64_t num)
{
    if (is_big_endian())
    {
        return num;
    }
    else
    {
        return reverse_bytes(num);
    }
}

uint64_t from_big_endian(uint64_t num)
{
    if (is_big_endian())
    {
        return num;
    }
    else
    {
        return reverse_bytes(num);
    }
}

struct btree_header
{
    char magic[8];             // "4348PRJ3"
    uint64_t root_block;       // 0 if empty
    uint64_t next_block;       // next free block
    char unused[BLOCK_SIZE - 8 - 8 - 8];
};

struct btree_node_disk
{
    uint64_t block_id;         // node id
    uint64_t parent_block;     // 0 if root
    uint64_t num_keys;         // key count
    uint64_t keys[19];         // keys
    uint64_t values[19];       // values
    uint64_t children[20];     // child ids (0 if leaf)
    char unused[BLOCK_SIZE - 8 * (1 + 1 + 1 + 19 + 19 + 20)];
};

bool read_header(std::fstream &fs, btree_header &hdr)
{
    fs.seekg(0, std::ios::beg);
    fs.read(reinterpret_cast<char *>(&hdr), sizeof(btree_header));
    return fs.gcount() == sizeof(btree_header);
}

bool write_header(std::fstream &fs, const btree_header &hdr)
{
    fs.seekp(0, std::ios::beg);
    fs.write(reinterpret_cast<const char *>(&hdr), sizeof(btree_header));
    return fs.good();
}

bool read_node(std::fstream &fs, uint64_t block, btree_node_disk &node)
{
    fs.seekg(block * BLOCK_SIZE, std::ios::beg);
    fs.read(reinterpret_cast<char *>(&node), sizeof(btree_node_disk));
    return fs.gcount() == sizeof(btree_node_disk);
}

bool write_node(std::fstream &fs, const btree_node_disk &node)
{
    fs.seekp(node.block_id * BLOCK_SIZE, std::ios::beg);
    fs.write(reinterpret_cast<const char *>(&node), sizeof(btree_node_disk));
    return fs.good();
}

void create_index(const std::string &idx_file)
{
    std::ifstream in_fs(idx_file, std::ios::binary);
    if (in_fs.good())
    {
        std::cerr << "Error: File exists.\n";
        exit(EXIT_FAILURE);
    }
    in_fs.close();

    std::fstream out_fs(idx_file, std::ios::out | std::ios::binary);
    if (!out_fs.is_open())
    {
        std::cerr << "Error: Cannot create file.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    memcpy(hdr.magic, "4348PRJ3", 8);
    hdr.root_block = 0;
    hdr.next_block = 1;
    memset(hdr.unused, 0, sizeof(hdr.unused));
    if (!write_header(out_fs, hdr))
    {
        std::cerr << "Error: Write header failed.\n";
        exit(EXIT_FAILURE);
    }
    out_fs.seekp(BLOCK_SIZE - 1, std::ios::beg);
    out_fs.put('\0');
    out_fs.close();
    std::cout << "Index '" << idx_file << "' created.\n";
}

void insert_key(const std::string &idx_file, uint64_t key, uint64_t value)
{
    std::fstream fs(idx_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        std::cerr << "Error: Cannot open file.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    if (!read_header(fs, hdr))
    {
        std::cerr << "Error: Invalid index.\n";
        exit(EXIT_FAILURE);
    }
    if (strncmp(hdr.magic, "4348PRJ3", 8) != 0)
    {
        std::cerr << "Error: Bad magic number.\n";
        exit(EXIT_FAILURE);
    }
    // Load up to 3 nodes for insertion (stub).
    std::cout << "Insert key " << key << " value " << value << " (stub).\n";
    fs.close();
}

void search_key(const std::string &idx_file, uint64_t key)
{
    std::fstream fs(idx_file, std::ios::in | std::ios::binary);
    if (!fs.is_open())
    {
        std::cerr << "Error: Cannot open file.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    if (!read_header(fs, hdr))
    {
        std::cerr << "Error: Invalid index.\n";
        exit(EXIT_FAILURE);
    }
    if (strncmp(hdr.magic, "4348PRJ3", 8) != 0)
    {
        std::cerr << "Error: Bad magic number.\n";
        exit(EXIT_FAILURE);
    }
    // Search stub.
    std::cout << "Search key " << key << " (stub).\n";
    fs.close();
}

void load_csv(const std::string &idx_file, const std::string &csv_file)
{
    std::fstream fs(idx_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        std::cerr << "Error: Cannot open index.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    if (!read_header(fs, hdr))
    {
        std::cerr << "Error: Invalid index.\n";
        exit(EXIT_FAILURE);
    }
    if (strncmp(hdr.magic, "4348PRJ3", 8) != 0)
    {
        std::cerr << "Error: Bad magic number.\n";
        exit(EXIT_FAILURE);
    }
    fs.close();

    std::ifstream csv_fs(csv_file);
    if (!csv_fs.is_open())
    {
        std::cerr << "Error: Cannot open CSV.\n";
        exit(EXIT_FAILURE);
    }
    std::string line;
    while (std::getline(csv_fs, line))
    {
        std::istringstream iss(line);
        std::string key_str, val_str;
        if (std::getline(iss, key_str, ',') && std::getline(iss, val_str))
        {
            uint64_t key_val = std::stoull(key_str);
            uint64_t val_num = std::stoull(val_str);
            insert_key(idx_file, key_val, val_num);
        }
    }
    csv_fs.close();
}

void print_index(const std::string &idx_file)
{
    std::ifstream in_fs(idx_file, std::ios::binary);
    if (!in_fs.is_open())
    {
        std::cerr << "Error: Cannot open file.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    in_fs.read(reinterpret_cast<char *>(&hdr), sizeof(btree_header));
    if (strncmp(hdr.magic, "4348PRJ3", 8) != 0)
    {
        std::cerr << "Error: Bad magic.\n";
        exit(EXIT_FAILURE);
    }
    // Print stub.
    std::cout << "Print index (stub).\n";
    in_fs.close();
}

void extract_index(const std::string &idx_file, const std::string &out_csv)
{
    std::ifstream in_fs(idx_file, std::ios::binary);
    if (!in_fs.is_open())
    {
        std::cerr << "Error: Cannot open index.\n";
        exit(EXIT_FAILURE);
    }
    btree_header hdr;
    in_fs.read(reinterpret_cast<char *>(&hdr), sizeof(btree_header));
    if (strncmp(hdr.magic, "4348PRJ3", 8) != 0)
    {
        std::cerr << "Error: Bad magic.\n";
        exit(EXIT_FAILURE);
    }
    in_fs.close();

    std::ifstream out_check(out_csv);
    if (out_check.good())
    {
        std::cerr << "Error: Output exists.\n";
        exit(EXIT_FAILURE);
    }
    out_check.close();

    std::ofstream out_fs(out_csv);
    if (!out_fs.is_open())
    {
        std::cerr << "Error: Cannot create CSV.\n";
        exit(EXIT_FAILURE);
    }
    out_fs << "Key,Value\n";
    // Extraction stub.
    out_fs << "Extraction stub.\n";
    out_fs.close();
    std::cout << "Extracted to " << out_csv << " (stub).\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: project3 <cmd> [args]\n";
        exit(EXIT_FAILURE);
    }
    std::string cmd = argv[1];
    if (cmd == "create")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: project3 create <idx_file>\n";
            exit(EXIT_FAILURE);
        }
        create_index(argv[2]);
    }
    else if (cmd == "insert")
    {
        if (argc != 5)
        {
            std::cerr << "Usage: project3 insert <idx_file> <key> <val>\n";
            exit(EXIT_FAILURE);
        }
        uint64_t key_val = std::stoull(argv[3]);
        uint64_t val_num = std::stoull(argv[4]);
        insert_key(argv[2], key_val, val_num);
    }
    else if (cmd == "search")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 search <idx_file> <key>\n";
            exit(EXIT_FAILURE);
        }
        uint64_t key_val = std::stoull(argv[3]);
        search_key(argv[2], key_val);
    }
    else if (cmd == "load")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 load <idx_file> <csv_file>\n";
            exit(EXIT_FAILURE);
        }
        load_csv(argv[2], argv[3]);
    }
    else if (cmd == "print")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: project3 print <idx_file>\n";
            exit(EXIT_FAILURE);
        }
        print_index(argv[2]);
    }
    else if (cmd == "extract")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: project3 extract <idx_file> <out_csv>\n";
            exit(EXIT_FAILURE);
        }
        extract_index(argv[2], argv[3]);
    }
    else
    {
        std::cerr << "Unknown cmd.\n";
        exit(EXIT_FAILURE);
    }
    return 0;
}
