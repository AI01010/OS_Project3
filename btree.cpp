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
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <endian.h>  // For htobe64 and be64toh
#include <stdint.h>
#include <sstream>
#include <algorithm>


const int BLOCK_SIZE = 512;
const int B_TREE_MIN_DEGREE = 10;
const int MAX_KEYS = 2 * B_TREE_MIN_DEGREE - 1; // 19 keys
const int MAX_CHILDREN = MAX_KEYS + 1; // 20 children
const std::string MAGIC_NUMBER = "4348PRJ3";

// Check if system is big-endian
bool is_bigendian() {
    int x = 1;
    return ((uint8_t *)&x)[0] != 1;
}

// Reverse bytes for endianness conversion if needed
uint64_t reverse_bytes(uint64_t x) {
    uint8_t dest[sizeof(uint64_t)];
    uint8_t *source = (uint8_t*)&x;
    for(int c = 0; c < sizeof(uint64_t); c++)
        dest[c] = source[sizeof(uint64_t)-c-1];
    return *(uint64_t *)dest;
}

// Convert integer to big-endian if needed
uint64_t to_big_endian(uint64_t value) {
    if (is_bigendian()) {
        return value;
    } else {
        return reverse_bytes(value);
    }
}

// Convert from big-endian to native endianness if needed
uint64_t from_big_endian(uint64_t value) {
    if (is_bigendian()) {
        return value;
    } else {
        return reverse_bytes(value);
    }
}

struct Header
{
    char magic[8];
    uint64_t root_id;
    uint64_t next_block_id;

    Header() : root_id(0), next_block_id(1)
    {
        strncpy(magic, MAGIC_NUMBER.c_str(), 8);
    }
};

struct BTreeNode
{
    uint64_t block_id;
    uint64_t parent_id;
    uint64_t num_keys;
    uint64_t keys[MAX_KEYS];
    uint64_t values[MAX_KEYS];
    uint64_t children[MAX_CHILDREN];

    BTreeNode() : block_id(0), parent_id(0), num_keys(0)
    {
        std::fill(keys, keys + MAX_KEYS, 0);
        std::fill(values, values + MAX_KEYS, 0);
        std::fill(children, children + MAX_CHILDREN, 0);
    }

    bool isLeaf() const
    {
        for (int i = 0; i < MAX_CHILDREN; i++)
        {
            if (children[i] != 0)
            {
                return false;
            }
        }
        return true;
    }
};

class BTreeManager
{
private:
    std::string filename;
    Header header;
    // We track loaded nodes to ensure we never have more than 3 in memory
    std::vector<BTreeNode> loaded_nodes;

    void writeHeader()
    {
        std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Cannot open index file for writing header" << std::endl;
            return;
        }

        // Convert header values to big-endian if needed
        Header big_endian_header = header;
        big_endian_header.root_id = to_big_endian(header.root_id);
        big_endian_header.next_block_id = to_big_endian(header.next_block_id);

        file.seekp(0);
        file.write(reinterpret_cast<const char *>(&big_endian_header), sizeof(Header));
        file.close();
    }

    bool readHeader()
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Cannot open index file for reading header" << std::endl;
            return false;
        }

        file.read(reinterpret_cast<char *>(&header), sizeof(Header));

        // Convert header values from big-endian if needed
        header.root_id = from_big_endian(header.root_id);
        header.next_block_id = from_big_endian(header.next_block_id);

        // Verify magic number
        if (strncmp(header.magic, MAGIC_NUMBER.c_str(), 8) != 0)
        {
            std::cerr << "Error: Invalid index file format" << std::endl;
            return false;
        }

        file.close();
        return true;
    }

    void writeNode(const BTreeNode &node)
    {
        std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Cannot open index file for writing node" << std::endl;
            return;
        }

        // Create a big-endian copy of the node
        BTreeNode big_endian_node = node;
        big_endian_node.block_id = to_big_endian(node.block_id);
        big_endian_node.parent_id = to_big_endian(node.parent_id);
        big_endian_node.num_keys = to_big_endian(node.num_keys);

        for (int i = 0; i < MAX_KEYS; i++)
        {
            big_endian_node.keys[i] = to_big_endian(node.keys[i]);
            big_endian_node.values[i] = to_big_endian(node.values[i]);
        }

        for (int i = 0; i < MAX_CHILDREN; i++)
        {
            big_endian_node.children[i] = to_big_endian(node.children[i]);
        }

        // Calculate position in file
        std::streampos position = node.block_id * BLOCK_SIZE;
        file.seekp(position);
        file.write(reinterpret_cast<const char *>(&big_endian_node), sizeof(BTreeNode));
        file.close();
    }

    BTreeNode readNode(uint64_t block_id)
    {
        // First check if the node is already loaded
        for (const auto &node : loaded_nodes)
        {
            if (node.block_id == block_id)
            {
                return node;
            }
        }

        // Manage loaded nodes to ensure we don't exceed 3
        if (loaded_nodes.size() >= 3)
        {
            loaded_nodes.erase(loaded_nodes.begin());
        }

        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Cannot open index file for reading node" << std::endl;
            return BTreeNode();
        }

        BTreeNode node;
        std::streampos position = block_id * BLOCK_SIZE;
        file.seekg(position);
        file.read(reinterpret_cast<char *>(&node), sizeof(BTreeNode));

        // Convert from big-endian if needed
        node.block_id = from_big_endian(node.block_id);
        node.parent_id = from_big_endian(node.parent_id);
        node.num_keys = from_big_endian(node.num_keys);

        for (int i = 0; i < MAX_KEYS; i++)
        {
            node.keys[i] = from_big_endian(node.keys[i]);
            node.values[i] = from_big_endian(node.values[i]);
        }

        for (int i = 0; i < MAX_CHILDREN; i++)
        {
            node.children[i] = from_big_endian(node.children[i]);
        }

        file.close();

        // Add to loaded nodes
        loaded_nodes.push_back(node);

        return node;
    }

    uint64_t createNewNode()
    {
        BTreeNode node;
        node.block_id = header.next_block_id;
        header.next_block_id++;
        writeNode(node);
        writeHeader();
        return node.block_id;
    }

    void createRootNode()
    {
        BTreeNode root;
        root.block_id = header.next_block_id;
        header.root_id = root.block_id;
        header.next_block_id++;
        writeNode(root);
        writeHeader();
    }

    int findInsertPosition(const BTreeNode &node, uint64_t key)
    {
        int i = 0;
        while (i < node.num_keys && key > node.keys[i])
        {
            i++;
        }
        return i;
    }

    void splitChild(uint64_t parent_block_id, int child_index)
    {
        BTreeNode parent = readNode(parent_block_id);
        uint64_t child_block_id = parent.children[child_index];
        BTreeNode child = readNode(child_block_id);

        // Create a new node for the right half of the child
        uint64_t new_node_id = createNewNode();
        BTreeNode new_node = readNode(new_node_id);
        new_node.parent_id = parent_block_id;

        // Set the new node as a child of the parent
        for (int i = parent.num_keys; i > child_index; i--)
        {
            parent.children[i + 1] = parent.children[i];
        }
        parent.children[child_index + 1] = new_node_id;

        // Move median key to the parent
        int median = B_TREE_MIN_DEGREE - 1;
        for (int i = parent.num_keys; i > child_index; i--)
        {
            parent.keys[i] = parent.keys[i - 1];
            parent.values[i] = parent.values[i - 1];
        }
        parent.keys[child_index] = child.keys[median];
        parent.values[child_index] = child.values[median];
        parent.num_keys++;

        // Move right half of child keys to the new node
        new_node.num_keys = B_TREE_MIN_DEGREE - 1;
        for (int i = 0; i < new_node.num_keys; i++)
        {
            new_node.keys[i] = child.keys[i + B_TREE_MIN_DEGREE];
            new_node.values[i] = child.values[i + B_TREE_MIN_DEGREE];
        }

        // Move right half of child's children to the new node
        if (!child.isLeaf())
        {
            for (int i = 0; i <= new_node.num_keys; i++)
            {
                new_node.children[i] = child.children[i + B_TREE_MIN_DEGREE];
                // Update parent pointers of the moved children
                BTreeNode moved_child = readNode(new_node.children[i]);
                moved_child.parent_id = new_node_id;
                writeNode(moved_child);
            }
        }

        // Update child to only contain left half
        child.num_keys = B_TREE_MIN_DEGREE - 1;

        // Write all modified nodes back to disk
        writeNode(parent);
        writeNode(child);
        writeNode(new_node);
    }

    void insertNonFull(uint64_t block_id, uint64_t key, uint64_t value)
    {
        BTreeNode node = readNode(block_id);

        // If this is a leaf node, insert the key here
        if (node.isLeaf())
        {
            int i = node.num_keys - 1;
            while (i >= 0 && key < node.keys[i])
            {
                node.keys[i + 1] = node.keys[i];
                node.values[i + 1] = node.values[i];
                i--;
            }
            node.keys[i + 1] = key;
            node.values[i + 1] = value;
            node.num_keys++;
            writeNode(node);
        }
        // If this is an internal node, find the child to recurse to
        else
        {
            int i = node.num_keys - 1;
            while (i >= 0 && key < node.keys[i])
            {
                i--;
            }
            i++;

            BTreeNode child = readNode(node.children[i]);

            // If the child is full, split it first
            if (child.num_keys == MAX_KEYS)
            {
                splitChild(block_id, i);

                // After splitting, the median key is in the parent
                // Determine which child to follow
                node = readNode(block_id);
                if (key > node.keys[i])
                {
                    i++;
                }
            }
            insertNonFull(node.children[i], key, value);
        }
    }

    std::pair<bool, uint64_t> searchInNode(uint64_t block_id, uint64_t key)
    {
        BTreeNode node = readNode(block_id);
        int i = 0;
        while (i < node.num_keys && key > node.keys[i])
        {
            i++;
        }
        if (i < node.num_keys && key == node.keys[i])
        {
            return {true, node.values[i]};
        }
        if (node.isLeaf())
        {
            return {false, 0};
        }
        return searchInNode(node.children[i], key);
    }

    void traverseTree(uint64_t block_id, void (*process)(uint64_t, uint64_t))
    {
        if (block_id == 0)
        {
            return;
        }

        BTreeNode node = readNode(block_id);

        // For each key in this node
        for (int i = 0; i < node.num_keys; i++)
        {
            // Recurse to left child
            if (!node.isLeaf())
            {
                traverseTree(node.children[i], process);
            }

            // Process current key
            process(node.keys[i], node.values[i]);

            // Recurse to rightmost child after all keys
            if (i == node.num_keys - 1 && !node.isLeaf())
            {
                traverseTree(node.children[i + 1], process);
            }
        }
    }

public:
    BTreeManager(const std::string &filename) : filename(filename)
    {
    }

    bool createIndexFile()
    {
        std::ifstream check_file(filename);
        if (check_file)
        {
            std::cerr << "Error: File already exists" << std::endl;
            check_file.close();
            return false;
        }

        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Cannot create index file" << std::endl;
            return false;
        }

        // Initialize header
        Header header;
        Header big_endian_header = header;
        big_endian_header.root_id = to_big_endian(header.root_id);
        big_endian_header.next_block_id = to_big_endian(header.next_block_id);
        file.write(reinterpret_cast<const char *>(&big_endian_header), sizeof(Header));

        // Pad the rest of the first block with zeros
        char padding[BLOCK_SIZE - sizeof(Header)] = {0};
        file.write(padding, sizeof(padding));

        file.close();

        // Initialize class header
        this->header = header;

        return true;
    }

    bool openIndexFile()
    {
        if (!readHeader())
        {
            return false;
        }
        return true;
    }

    bool insert(uint64_t key, uint64_t value)
    {
        // If the tree is empty, create a root node
        if (header.root_id == 0)
        {
            createRootNode();
        }

        BTreeNode root = readNode(header.root_id);

        // If the root is full, the tree needs to grow in height
        if (root.num_keys == MAX_KEYS)
        {
            // Create a new root
            uint64_t new_root_id = createNewNode();
            BTreeNode new_root = readNode(new_root_id);

            // Make the old root a child of the new root
            new_root.children[0] = header.root_id;

            // Update the old root's parent
            root.parent_id = new_root_id;
            writeNode(root);

            // Set the new root in the header
            header.root_id = new_root_id;
            writeHeader();

            // Split the old root
            splitChild(new_root_id, 0);

            // Insert the key-value pair into the new structure
            insertNonFull(new_root_id, key, value);
        }
        else
        {
            // Insert into non-full root
            insertNonFull(header.root_id, key, value);
        }

        return true;
    }

    std::pair<bool, uint64_t> search(uint64_t key)
    {
        if (header.root_id == 0)
        {
            return {false, 0}; // Empty tree
        }

        return searchInNode(header.root_id, key);
    }

    void print()
    {
        if (header.root_id == 0)
        {
            std::cout << "Index is empty" << std::endl;
            return;
        }

        auto printKeyValue = [](uint64_t key, uint64_t value)
        {
            std::cout << key << " " << value << std::endl;
        };

        traverseTree(header.root_id, printKeyValue);
    }

    bool extract(const std::string &output_filename)
    {
        std::ofstream output_file(output_filename);
        if (!output_file)
        {
            std::cerr << "Error: Cannot create output file" << std::endl;
            return false;
        }

        if (header.root_id == 0)
        {
            output_file.close();
            return true; // Empty index, create empty CSV
        }

        auto writeToCSV = [&output_file](uint64_t key, uint64_t value)
        {
            output_file << key << "," << value << std::endl;
        };

        traverseTree(header.root_id, writeToCSV);

        output_file.close();
        return true;
    }

    bool load(const std::string &input_filename)
    {
        std::ifstream input_file(input_filename);
        if (!input_file)
        {
            std::cerr << "Error: Cannot open input file" << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(input_file, line))
        {
            std::stringstream ss(line);
            std::string key_str, value_str;

            if (std::getline(ss, key_str, ',') && std::getline(ss, value_str))
            {
                try
                {
                    uint64_t key = std::stoull(key_str);
                    uint64_t value = std::stoull(value_str);
                    insert(key, value);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error parsing line: " << line << std::endl;
                }
            }
        }

        input_file.close();
        return true;
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <command> [args]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "create")
    {
        if (argc < 3)
        {
            std::cerr << "Error: Missing filename for create command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        BTreeManager manager(filename);

        if (!manager.createIndexFile())
        {
            return 1;
        }

        std::cout << "Index file created successfully" << std::endl;
    }
    else if (command == "insert")
    {
        if (argc < 5)
        {
            std::cerr << "Error: Missing arguments for insert command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        uint64_t key, value;

        try
        {
            key = std::stoull(argv[3]);
            value = std::stoull(argv[4]);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Invalid key or value format" << std::endl;
            return 1;
        }

        BTreeManager manager(filename);

        if (!manager.openIndexFile())
        {
            return 1;
        }

        if (!manager.insert(key, value))
        {
            return 1;
        }

        std::cout << "Key-value pair inserted successfully" << std::endl;
    }
    else if (command == "search")
    {
        if (argc < 4)
        {
            std::cerr << "Error: Missing arguments for search command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        uint64_t key;

        try
        {
            key = std::stoull(argv[3]);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Invalid key format" << std::endl;
            return 1;
        }

        BTreeManager manager(filename);

        if (!manager.openIndexFile())
        {
            return 1;
        }

        auto [found, value] = manager.search(key);

        if (found)
        {
            std::cout << key << " " << value << std::endl;
        }
        else
        {
            std::cout << "Key not found" << std::endl;
        }
    }
    else if (command == "load")
    {
        if (argc < 4)
        {
            std::cerr << "Error: Missing arguments for load command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::string csv_filename = argv[3];

        BTreeManager manager(filename);

        if (!manager.openIndexFile())
        {
            return 1;
        }

        if (!manager.load(csv_filename))
        {
            return 1;
        }

        std::cout << "Data loaded successfully" << std::endl;
    }
    else if (command == "print")
    {
        if (argc < 3)
        {
            std::cerr << "Error: Missing filename for print command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];

        BTreeManager manager(filename);

        if (!manager.openIndexFile())
        {
            return 1;
        }

        manager.print();
    }
    else if (command == "extract")
    {
        if (argc < 4)
        {
            std::cerr << "Error: Missing arguments for extract command" << std::endl;
            return 1;
        }

        std::string filename = argv[2];
        std::string output_filename = argv[3];

        // Check if output file already exists
        std::ifstream check_file(output_filename);
        if (check_file)
        {
            std::cerr << "Error: Output file already exists" << std::endl;
            check_file.close();
            return 1;
        }

        BTreeManager manager(filename);

        if (!manager.openIndexFile())
        {
            return 1;
        }

        if (!manager.extract(output_filename))
        {
            return 1;
        }

        std::cout << "Data extracted successfully" << std::endl;
    }
    else
    {
        std::cerr << "Error: Unknown command" << std::endl;
        return 1;
    }

    return 0;
}