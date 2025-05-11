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

using namespace std;

const int HEADER_SIZE = 512;
const int NODE_SIZE = 512;
const int KEYS_PER_NODE = 19;
const int VALUES_PER_NODE = 19;
const int CHILDREN_PER_NODE = 20;
const std::string MAGIC_NUMBER = "4348PRJ3";

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
    uint8_t *source = (uint8_t *)&x;
    for (int c = 0; c < sizeof(uint64_t); c++)
    {
        dest[c] = source[sizeof(uint64_t) - c - 1];
    }
    return *(uint64_t *)dest;
}

// Convert integer to big-endian if needed
uint64_t to_big_endian(uint64_t value)
{
    if (is_bigendian())
    {
        return value;
    }
    else
    {
        return reverse_bytes(value);
    }
}

// Convert from big-endian to native endianness if needed
uint64_t from_big_endian(uint64_t value)
{
    if (is_bigendian())
    {
        return value;
    }
    else
    {
        return reverse_bytes(value);
    }
}

class BTree
{
private:
    std::string index_file;
    uint64_t root_id;
    uint64_t next_block_id;

public:
    BTree() : root_id(0), next_block_id(1)
    {
    }

    bool isIndexOpen() const
    {
        return !index_file.empty();
    }

    void create(const std::string &filename)
    {
        // If the file exists, ask for confirmation to overwrite
        ifstream infile(filename, ios::binary);
        if (infile.good())
        {
            cout << "File " << filename << " already exists. Do you want to overwrite (y/n)?: ";
            string response;
            getline(cin, response);
            if (response != "y")
            {
                cout << "Menu option discarded. Did not overwrite " << filename << "." << endl;
                return;
            }
        }

        // Create and initialize file with header
        ofstream f(filename, ios::binary | ios::trunc);
        if (!f)
        {
            cout << "Error: Could not create file " << filename << "." << endl;
            return;
        }

        // Write magic number
        f.write(MAGIC_NUMBER.c_str(), MAGIC_NUMBER.size());

        // Write root_id (0) and next_block_id in big-endian format
        uint64_t r = htobe64((uint64_t)0);
        uint64_t nb = htobe64(next_block_id);
        f.write(reinterpret_cast<const char *>(&r), sizeof(uint64_t));
        f.write(reinterpret_cast<const char *>(&nb), sizeof(uint64_t));

        // Pad remaining header space
        int header_written = MAGIC_NUMBER.size() + 2 * sizeof(uint64_t);
        int remaining = HEADER_SIZE - header_written;
        std::vector<char> padding(remaining, 0);
        f.write(padding.data(), padding.size());
        f.close();

        index_file = filename;
        cout << "Created index file " << filename << "." << endl;
        // Automatically open the file
        open(filename);
    }

    void open(const std::string &filename)
    {
        ifstream f(filename, ios::binary);
        if (!f)
        {
            cout << "Error: File " << filename << " does not exist." << endl;
            return;
        }
        char magic_buffer[MAGIC_NUMBER.size()];
        f.read(magic_buffer, MAGIC_NUMBER.size());
        if (std::string(magic_buffer, MAGIC_NUMBER.size()) != MAGIC_NUMBER)
        {
            cout << "Error: Invalid file format." << endl;
            f.close();
            return;
        }
        // Read root_id and next_block_id
        uint64_t r, nb;
        f.read(reinterpret_cast<char *>(&r), sizeof(uint64_t));
        f.read(reinterpret_cast<char *>(&nb), sizeof(uint64_t));
        root_id = be64toh(r);
        next_block_id = be64toh(nb);
        f.close();

        index_file = filename;
        cout << "Opened index file " << filename << "." << endl;
    }

    void insert(uint64_t key, uint64_t value)
    {
        if (index_file.empty())
        {
            cout << "No index file is open." << endl;
            return;
        }
        if (root_id == 0)
        {
            create_root_node(key, value);
        }
        else
        {
            insert_into_node(root_id, key, value);
        }
    }

    void create_root_node(uint64_t key, uint64_t value)
    {
        uint64_t node_id = next_block_id++;
        fstream f(index_file, ios::in | ios::out | ios::binary);
        if (!f)
        {
            cout << "Error: Failed to open index file " << index_file << "." << endl;
            return;
        }

        // Seek to where the node should be written
        f.seekp(HEADER_SIZE + (node_id * NODE_SIZE));

        // Write node id, parent id (0 for root), and number of keys (1) in big-endian
        uint64_t be_node_id = htobe64(node_id);
        uint64_t be_parent = htobe64((uint64_t)0);
        uint64_t be_num = htobe64((uint64_t)1);
        f.write(reinterpret_cast<const char *>(&be_node_id), sizeof(uint64_t));
        f.write(reinterpret_cast<const char *>(&be_parent), sizeof(uint64_t));
        f.write(reinterpret_cast<const char *>(&be_num), sizeof(uint64_t));

        // Write the key
        uint64_t be_key = htobe64(key);
        f.write(reinterpret_cast<const char *>(&be_key), sizeof(uint64_t));

        // Pad remaining keys
        int pad_keys = (KEYS_PER_NODE - 1) * sizeof(uint64_t);
        vector<char> pad(pad_keys, 0);
        f.write(pad.data(), pad.size());

        // Write the value
        uint64_t be_value = htobe64(value);
        f.write(reinterpret_cast<const char *>(&be_value), sizeof(uint64_t));

        // Pad remaining values
        int pad_values = (VALUES_PER_NODE - 1) * sizeof(uint64_t);
        vector<char> padVal(pad_values, 0);
        f.write(padVal.data(), padVal.size());

        // Write two child pointers (left and right) and pad remaining children
        uint64_t be_zero = htobe64((uint64_t)0);
        f.write(reinterpret_cast<const char *>(&be_zero), sizeof(uint64_t));
        f.write(reinterpret_cast<const char *>(&be_zero), sizeof(uint64_t));
        int pad_children = (CHILDREN_PER_NODE - 2) * sizeof(uint64_t);
        vector<char> padChild(pad_children, 0);
        f.write(padChild.data(), padChild.size());

        // Pad the rest of the node so its size is NODE_SIZE
        streamoff pos = f.tellp();
        int already = pos - (HEADER_SIZE + (node_id * NODE_SIZE));
        int remaining = NODE_SIZE - already;
        vector<char> padNode(remaining, 0);
        f.write(padNode.data(), padNode.size());

        f.close();
        root_id = node_id;
        // Update header with new root/next_block_id values
        updateHeader();
        cout << "Root Node created using key: " << key << ", value: " << value << "." << endl;
    }

    void insert_into_node(uint64_t node_id, uint64_t key, uint64_t value)
    {
        fstream f(index_file, ios::in | ios::out | ios::binary);
        if (!f)
        {
            cout << "Error: Failed to open index file " << index_file << "." << endl;
            return;
        }

        f.seekg(HEADER_SIZE + (node_id * NODE_SIZE));
        vector<char> node_data(NODE_SIZE);
        f.read(node_data.data(), NODE_SIZE);
        if (f.gcount() < NODE_SIZE)
        {
            cout << "Error: Node data is not long enough." << endl;
            f.close();
            return;
        }

        // Read current number of keys (at offset 16)
        uint64_t num_keys;
        memcpy(&num_keys, &node_data[16], sizeof(uint64_t));
        num_keys = be64toh(num_keys);

        // For simplicity, append the key
        num_keys++;
        f.seekp(HEADER_SIZE + (node_id * NODE_SIZE) + 16);
        uint64_t be_num_keys = htobe64(num_keys);
        f.write(reinterpret_cast<const char *>(&be_num_keys), sizeof(uint64_t));

        // Write new key at offset 24 + ((num_keys-1) * 8)
        f.seekp(HEADER_SIZE + (node_id * NODE_SIZE) + 24 + ((num_keys - 1) * sizeof(uint64_t)));
        uint64_t be_key = htobe64(key);
        f.write(reinterpret_cast<const char *>(&be_key), sizeof(uint64_t));

        // Write new value at offset 176 + ((num_keys-1) * 8)
        f.seekp(HEADER_SIZE + (node_id * NODE_SIZE) + 176 + ((num_keys - 1) * sizeof(uint64_t)));
        uint64_t be_value = htobe64(value);
        f.write(reinterpret_cast<const char *>(&be_value), sizeof(uint64_t));

        f.close();
        cout << "Inserted key: " << key << ", value: " << value << " in node " << node_id << "." << endl;
        updateHeader();
    }

    // Search for a key in the index file
    void search(uint64_t search_key)
    {
        if (index_file.empty())
        {
            cout << "Error: No index file is open." << endl;
            return;
        }
        ifstream f(index_file, ios::binary);
        if (!f)
        {
            cout << "Error: Failed to open index file." << endl;
            return;
        }
        // Validate magic number
        char magic_buffer[MAGIC_NUMBER.size()];
        f.read(magic_buffer, MAGIC_NUMBER.size());
        if (std::string(magic_buffer, MAGIC_NUMBER.size()) != MAGIC_NUMBER)
        {
            cout << "Error: Invalid index file." << endl;
            f.close();
            return;
        }
        // Read header: root_id and next_block_id
        uint64_t file_root_id, file_next_block_id;
        f.read(reinterpret_cast<char *>(&file_root_id), sizeof(uint64_t));
        f.read(reinterpret_cast<char *>(&file_next_block_id), sizeof(uint64_t));
        file_root_id = be64toh(file_root_id);
        file_next_block_id = be64toh(file_next_block_id);

        bool found = false;
        // Iterate over nodes in the file
        for (uint64_t node_id = 1; node_id < file_next_block_id; node_id++)
        {
            f.seekg(HEADER_SIZE + (node_id * NODE_SIZE), ios::beg);
            vector<char> node_data(NODE_SIZE);
            f.read(node_data.data(), NODE_SIZE);
            if (f.gcount() < NODE_SIZE)
                continue;
            uint64_t num_keys;
            memcpy(&num_keys, &node_data[16], sizeof(uint64_t));
            num_keys = be64toh(num_keys);
            for (uint64_t i = 0; i < num_keys; i++)
            {
                uint64_t curr_key;
                memcpy(&curr_key, &node_data[24 + i * sizeof(uint64_t)], sizeof(uint64_t));
                curr_key = be64toh(curr_key);
                if (curr_key == search_key)
                {
                    uint64_t value;
                    memcpy(&value, &node_data[176 + i * sizeof(uint64_t)], sizeof(uint64_t));
                    value = be64toh(value);
                    cout << "Found key: " << curr_key << ", value: " << value << "." << endl;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (!found)
        {
            cout << "Key " << search_key << " was not found in index." << endl;
        }
        f.close();
    }

    void load(const std::string &filename)
    {
        if (index_file.empty())
        {
            cout << "Error: No index file is open." << endl;
            return;
        }
        cout << "Loading key, value pairs from " << filename << "..." << endl;
        ifstream f(filename);
        if (!f)
        {
            cout << "Error: File " << filename << " does not exist." << endl;
            return;
        }
        string line;
        while (getline(f, line))
        {
            istringstream iss(line);
            string keyStr, valueStr;
            if (getline(iss, keyStr, ',') && getline(iss, valueStr))
            {
                uint64_t key = stoull(keyStr);
                uint64_t value = stoull(valueStr);
                insert(key, value);
                cout << "  Loaded Key: " << key << ", value: " << value << "." << endl;
            }
        }
        f.close();
        cout << "Completed loading from " << filename << "." << endl;
    }

    void print_index()
    {
        if (index_file.empty())
        {
            cout << "Error: No index file is open." << endl;
            return;
        }
        if (root_id == 0)
        {
            cout << "Index is empty." << endl;
            return;
        }
        ifstream f(index_file, ios::binary);
        if (!f)
        {
            cout << "Error: Failed to open index file." << endl;
            return;
        }
        cout << "Index file contents: " << endl;
        // Skip header
        f.seekg(HEADER_SIZE, ios::beg);
        // For each node (node ids start at 1)
        for (uint64_t node_id = 1; node_id < next_block_id; node_id++)
        {
            f.seekg(HEADER_SIZE + (node_id * NODE_SIZE), ios::beg);
            vector<char> node_data(NODE_SIZE);
            f.read(node_data.data(), NODE_SIZE);
            if (f.gcount() < NODE_SIZE)
                continue;
            uint64_t num_keys;
            memcpy(&num_keys, &node_data[16], sizeof(uint64_t));
            num_keys = be64toh(num_keys);
            if (num_keys == 0)
                continue;
            cout << "Node " << node_id << ", keys: " << num_keys << endl;
            for (uint64_t i = 0; i < num_keys; i++)
            {
                uint64_t key;
                memcpy(&key, &node_data[24 + i * sizeof(uint64_t)], sizeof(uint64_t));
                key = be64toh(key);
                uint64_t value;
                memcpy(&value, &node_data[176 + i * sizeof(uint64_t)], sizeof(uint64_t));
                value = be64toh(value);
                cout << "  Key: " << key << ", value: " << value << endl;
            }
        }
        f.close();
    }

    void extract(const std::string &filename)
    {
        if (index_file.empty())
        {
            cout << "Error: No index file is open." << endl;
            return;
        }
        if (root_id == 0)
        {
            ofstream out(filename);
            out.close();
            cout << "Extracted key, value pairs to " << filename << "." << endl;
            return;
        }
        ifstream f(index_file, ios::binary);
        if (!f)
        {
            cout << "Error: Failed to open index file." << endl;
            return;
        }
        ofstream out(filename);
        if (!out)
        {
            cout << "Error: Failed to create output file." << filename << "." << endl;
            return;
        }
        // Skip header
        f.seekg(HEADER_SIZE, ios::beg);
        // For each node
        for (uint64_t node_id = 1; node_id < next_block_id; node_id++)
        {
            f.seekg(HEADER_SIZE + (node_id * NODE_SIZE), ios::beg);
            vector<char> node_data(NODE_SIZE);
            f.read(node_data.data(), NODE_SIZE);
            if (f.gcount() < NODE_SIZE)
                continue;
            uint64_t num_keys;
            memcpy(&num_keys, &node_data[16], sizeof(uint64_t));
            num_keys = be64toh(num_keys);
            for (uint64_t i = 0; i < num_keys; i++)
            {
                uint64_t key;
                memcpy(&key, &node_data[24 + i * sizeof(uint64_t)], sizeof(uint64_t));
                key = be64toh(key);
                uint64_t value;
                memcpy(&value, &node_data[176 + i * sizeof(uint64_t)], sizeof(uint64_t));
                value = be64toh(value);
                out << key << "," << value << "\n";
            }
        }
        f.close();
        out.close();
        cout << "Completed extracting key, value pairs to " << filename << "." << endl;
    }

    void quit()
    {
        cout << "Quiting program." << endl;
    }

private:
    // Update header information in the index file (root_id and next_block_id)
    void updateHeader()
    {
        fstream f(index_file, ios::in | ios::out | ios::binary);
        if (!f)
            return;
        // Skip magic number
        f.seekp(MAGIC_NUMBER.size());
        uint64_t be_root = htobe64(root_id);
        uint64_t be_next = htobe64(next_block_id);
        f.write(reinterpret_cast<const char *>(&be_root), sizeof(uint64_t));
        f.write(reinterpret_cast<const char *>(&be_next), sizeof(uint64_t));
        f.close();
    }
};

int main()
{
    BTree btree;
    string inputLine;
    while (true)
    {
        // Display menu options
        cout << "\nMenu Commands:" << endl;
        cout << "1 - [create <index file name>] Create new index file. " << endl;
        cout << "2 - [insert <index file name> <uint key> <uint val>] Insert key, value pair." << endl;
        cout << "3 - [search <index file name> <uint key>] Search key." << endl;
        cout << "4 - [load <index file name> <csv file name>] Load key, value pairs from file." << endl;
        cout << "5 - [print <index file name>] Print key, value pairs." << endl;
        cout << "6 - [extract <index file name> <csv file name>] Extract key, value pairs to file." << endl;
        cout << "\nEnter command: ";
        getline(cin, inputLine);
        istringstream iss(inputLine);
        vector<string> tokens;
        string token;
        while (iss >> token)
        {
            tokens.push_back(token);
        }
        if (tokens.empty())
        {
            continue;
        }
        // Convert command token to lowercase
        string cmd = tokens[0];
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "create")
        {
            if (tokens.size() != 2)
            {
                cout << "Usage: create <index file name>" << endl;
                continue;
            }
            btree.create(tokens[1]);
        }
        else if (cmd == "insert")
        {
            if (tokens.size() != 4)
            {
                cout << "Usage: insert <index file name> <uint key> <uint val>" << endl;
                continue;
            }
            btree.open(tokens[1]);
            uint64_t key = stoull(tokens[2]);
            uint64_t value = stoull(tokens[3]);
            btree.insert(key, value);
        }
        else if (cmd == "search")
        {
            if (tokens.size() != 3)
            {
                cout << "Usage: search <index file name> <uint key>" << endl;
                continue;
            }
            btree.open(tokens[1]);
            uint64_t key = stoull(tokens[2]);
            btree.search(key);
        }
        else if (cmd == "load")
        {
            if (tokens.size() != 3)
            {
                cout << "Usage: load <index file name> <csv file name>" << endl;
                continue;
            }
            btree.open(tokens[1]);
            btree.load(tokens[2]);
        }
        else if (cmd == "print")
        {
            if (tokens.size() != 2)
            {
                cout << "Usage: print <index file name>" << endl;
                continue;
            }
            btree.open(tokens[1]);
            btree.print_index();
        }
        else if (cmd == "extract")
        {
            if (tokens.size() != 3)
            {
                cout << "Usage: extract <index file name> <csv file name>" << endl;
                continue;
            }
            btree.open(tokens[1]);
            btree.extract(tokens[2]);
        }
        else if (cmd == "quit")
        {
            btree.quit();
            break;
        }
        else
        {
            cout << "Invalid command." << endl;
        }
    }
    return 0;
}