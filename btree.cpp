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
#include <vector>
#include <cstdlib>

class BTreeNode 
{
    //node code
}

class BTree 
{
    BTreeNode *root; // Pointer to root node
}

int main() 
{
    BTree tree(t); // make B tree
    int option, key;
    bool running = true;

    std::cout << "B-Tree Implementation\n";
    while (running) 
    {
        std::cout << "\nB-Tree Operations:\n";
        std::cout << "1. Create b-tree\n";
        std::cout << "2. Insert key\n";
        std::cout << "3. Search tree\n";
        std::cout << "4. Exit\n";
        std::cout << "Select an option: ";
        std::cin >> option;
        switch (option) 
        {
            case 1:
                std::cout << "Create a b-tree: ";
                std::cin >> key;
                tree.create(key);
                std::cout << "Tree created.\n";
                break;
            case 2:
                std::cout << "Enter key to insert: ";
                std::cin >> key;
                if (tree.insert(key) != nullptr)
                    std::cout << "Key " << key << " inserted in the tree.\n";
                else
                    std::cout << "Key " << key << " not inserted.\n";
                break;
            case 3:
                std::cout << "Searching a tree:";
                tree.search();
                std::cout << "\n";
                break;
            case 4:
                running = false;
                break;
            default:
                std::cout << "Invalid option. Try again.\n";
                break;
        }
    }
    std::cout << "Exiting program.\n";
    return 0;
}
