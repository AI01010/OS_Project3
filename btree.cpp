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


// Minimum degree (defines the range for number of keys)
const int t = 3;


class BTreeNode 
{
    public:
    int *keys;       // An array of keys
    int t;           // Minimum degree (defines the range for number of keys)
    BTreeNode **C;   // An array of child pointers
    int n;           // Current number of keys
    bool leaf;       // Is true when node is leaf. Otherwise false

    BTreeNode(int _t, bool _leaf) // Constructor to initialize the node
    {
        t = _t;
        leaf = _leaf;
        n = 0;
        keys = new int[2 * t - 1];
        C = new BTreeNode*[2 * t];
    }

    ~BTreeNode() // Destructor to free memory
    {
        delete[] keys;
        for (int i = 0; i < n+1; i++) 
        {
            delete C[i];
        }
        delete[] C;
    }

    // A function to traverse all nodes in a subtree rooted with this node.
    void traverse() 
    {
        int i;
        for (i = 0; i < n; i++) 
        {
            // If this is not leaf, then before printing key[i],
            // traverse the subtree rooted with child C[i].
            if (!leaf)
            {
                C[i]->traverse();
            }    
            std::cout << " " << keys[i];
        }
        // Print the subtree rooted with last child
        if (!leaf)
        {
            C[i]->traverse();
        }
    }

    // A function to search a key in subtree rooted with this node.
    BTreeNode* search(int k) 
    {
        int i = 0;
        while (i < n && k > keys[i])
        {
            i++;
        }   
        if (i < n && keys[i] == k)
        {
            return this;
        }
        if (leaf)
        {
            return nullptr;
        }
        return C[i]->search(k);
    }

    // A function to insert a new key in the subtree rooted with this node.
    // The node must be non-full when this function is called.
    void insertNonFull(int k) 
    {
        int i = n - 1;
        if (leaf) 
        {
            // Find the location of new key to be inserted and shift keys greater than k one space ahead.
            while (i >= 0 && keys[i] > k) 
            {
                keys[i + 1] = keys[i];
                i--;
            }
            keys[i + 1] = k;
            n = n + 1;
        }
        else 
        {
            // Find the child that is going to have the new key
            while (i >= 0 && keys[i] > k)
            {
                i--;
            }
            i++;
            // See if the found child is full
            if (C[i]->n == 2 * t - 1) 
            {
                splitChild(i, C[i]);
                if (keys[i] < k)
                {
                    i++;
                }
            }
            C[i]->insertNonFull(k);
        }
    }

    // A utility function to split the child y of this node.
    // Note that y must be full when this function is called.
    void splitChild(int i, BTreeNode *y) 
    {
        // Create a new node which will store (t-1) keys of y
        BTreeNode *z = new BTreeNode(y->t, y->leaf);
        z->n = t - 1;

        for (int j = 0; j < t - 1; j++)
        {
            z->keys[j] = y->keys[j + t];
        }
        if (!y->leaf) 
        {
            for (int j = 0; j < t; j++)
                z->C[j] = y->C[j + t];
        }
        y->n = t - 1;

        // Create space for new child
        for (int j = n; j >= i + 1; j--)
        {    
            C[j + 1] = C[j];
        }
        // Link new child to this node
        C[i + 1] = z;

        // Move a key from y to this node
        for (int j = n - 1; j >= i; j--)
        {
            keys[j + 1] = keys[j];
        }
        keys[i] = y->keys[t - 1];
        n = n + 1;
    }
};

class BTree 
{
    public:
    BTreeNode *root; // Pointer to root node
    int t;         // Minimum degree

    BTree(int _t)
    {
        root = nullptr;
        t = _t;
    }

    // Function to traverse the tree
    void traverse() 
    {
        if (root != nullptr)
            root->traverse();
    }

    // Function to search a key in this tree
    BTreeNode* search(int k) 
    {
        return (root == nullptr) ? nullptr : root->search(k);
    }

    // The main function that inserts a new key in this B-Tree
    void insert(int k) 
    {
        if (root == nullptr) 
        {
            root = new BTreeNode(t, true);
            root->keys[0] = k;
            root->n = 1;
        }
        else 
        {
            if (root->n == 2 * t - 1) 
            {
                BTreeNode *s = new BTreeNode(t, false);
                s->C[0] = root;
                s->splitChild(0, root);

                int i = 0;
                if (s->keys[0] < k)
                    i++;
                s->C[i]->insertNonFull(k);
                root = s;
            }
            else 
            {
                root->insertNonFull(k);
            }
        }
    }
};



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
                std::cout << "Tree already initialized.\n";
                break;
            case 2:
                std::cout << "Enter key to insert: ";
                std::cin >> key;
                tree.insert(key);
                std::cout << "Key " << key << " inserted in the tree.\n";
                break;
            case 3:
                std::cout << "Enter key to search: ";
                std::cin >> key;
                BTreeNode* result = tree.search(key);
                if (result != nullptr)
                    std::cout << "Key " << key << " found in the tree.\n";
                else
                    std::cout << "Key " << key << " not found in the tree.\n";
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
