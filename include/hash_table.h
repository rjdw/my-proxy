#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>


//node for separate chaining
//for a singly-linked list in the hash table
typedef struct node node;
typedef struct buffer_t buffer_t;


//inserts a value into the hash table with
//the key and value given
void insert(char *key, buffer_t *value);

//get for an value with a given key
//returns the value of the found element, otherwise returns NULL
//get is O(1) on average; the collision for-loop will rarely occur
buffer_t * get(char *key);

//deletes an element with the given key
//this is on average O(1)
void remove_element(char *key);

//evicts the least recently used element from the cache
void evict();

//updates the table until the size of the table
//no longer exceeds the MAX_CACHE_SIZE
//using evict
void update_table();

//hashes a string
//this is a version of the sdbm hash function used in the SDBM project
//this version was used in gawk
uint64_t hash_function(char *str);

void free_hash_table();

//recursively removes a singly-linked list chain from a bucket
void remove_node_chain(node *head);

//displays the entire hash table in the terminal
void display_table();

#endif
