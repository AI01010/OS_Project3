# OS_Project3

## Operating Systems Project 3: 
B-Tree implementation in C++
 - an interactive program that creates and manages index files. 
 - The index files will contain a b-tree. 
 - The user can create, insert, and search such index files.
 - They can also load and extract index files to and from csv files

Code: 
project3.cpp

To compile (use linux server: WSL (w/ Ubuntu), PuTTY, MobaXterm, etc.):
g++ project3.cpp -o project3
(OR 
 g++ project3.cpp -std=c++11 -o project3 
 if needed
)

To run:
./project3 <command> <fname> ( <key> | <key> <value> | <intput.csv> | <output.csv> )


Summary of Commands:
 - [create <index file name>]                           Create new index file.
 - [insert <index file name> <uint key> <uint value>]   Insert key, value pair.
 - [search <index file name> <uint key>]                Search key.
 - [load <index file name> <csv infile name>]           Load key, value pairs from file.
 - [print <index file name>]                            Print key, value pairs.
 - [extract <index file name> <csv outfile name>]       Extract key, value pairs to file.
