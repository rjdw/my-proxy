/**
This hash table is an approximate LRU cache.
The timestamps are not strictly LRU, and
are approximated due to the read write lock's
allowing changes to the timestamps in nonlinear
order in concurrent runs.
The concurrency of the cache is maintained by
read write locks, where all functions modifying
the hash table are in a write lock and all read
only functions are in a read lock.
**/
#include "hash_table.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "buffer.h"


#define TABLE_SIZE 10

//1 MiB in bytes
#define MAX_CACHE_SIZE (1048576 * sizeof(char))

//100 KiB in bytes
#define MAX_OBJECT_SIZE (100 * 1024 * sizeof(char))


//node for separate chaining
//for a singly-linked list in the hash table
typedef struct node {
    char *key;
    //this is a buffer_t
    //the buffer_t stores a byte array
    buffer_t *value;
    //next node for the separate chaining in each bucket
    node *next;
    time_t time;

    //mutex lock for the node
    //pthread_mutex_t node_lock;

} node;


//array of node pointers which will act as the hash table
static node *table[TABLE_SIZE] = {NULL};

//current size of the hash table in bytes
static uint64_t size = 0;


//static read write lock for entire file
static pthread_rwlock_t full_lock = PTHREAD_RWLOCK_INITIALIZER;


//inserts an element with a key and
//a value into the hash table
//with separate chaining to
//avoid collisions
void insert(char *key, buffer_t *value) {
    pthread_rwlock_wrlock(&full_lock);

    //printf("\ninserting into the cache\n");
    // printf("\n\ninserting: %s\n\n", key);

    if (sizeof(buffer_data(value)) > MAX_OBJECT_SIZE){
        pthread_rwlock_unlock(&full_lock);

        return;
    }

    uint64_t index = hash_function(key)%TABLE_SIZE;

    //printf("\ninserting in index %llu\n", index);

    node * newnode = (node *)malloc(sizeof(node));


    //set elements in the node
    newnode->key = key;
    newnode->value = value;
    newnode->next = NULL;
    newnode->time = time(NULL);
    //newnode->node_lock = pthread_mutex_init(&(newnode->node_lock), NULL);



    //test if there is a collision
    if(table[index] == NULL) {
        table[index] = newnode;
    }
    else {
        //there is a collision
        node *curr = table[index];
        while(curr->next != NULL) {
            //if the key already exists
            if (strcmp(curr->key, key) == 0){
                //update the size of the table
                size-=sizeof(buffer_data(curr->value));
                size+=sizeof(buffer_data(value));

                update_table();

                curr->value = value;
                curr->time = time(NULL);

                //newnode isn't used in this case
                free(newnode);
                pthread_rwlock_unlock(&full_lock);
                return;
            }
            curr = curr->next;
        }
        //add the new node at the end of the linked list
        size+=sizeof(buffer_data(value));

        update_table();

        curr->next=newnode;
    }
    pthread_rwlock_unlock(&full_lock);
}

//get for an value with a given key
//returns the value of the found element, otherwise returns NULL
//get is O(1) on average; the collision for-loop will rarely occur
buffer_t * get(char *key) {

    pthread_rwlock_rdlock(&full_lock);


    // printf("\ngetting from the cache\n");
    // printf("\n\ngetting: %s\n\n", key);

    uint64_t index = hash_function(key)%TABLE_SIZE;


    if(table[index] == NULL) {
        //printf("\nno such element with key: %s\n", key);
        pthread_rwlock_unlock(&full_lock);
        return NULL;
    }
    else{
        //the element possibly exists
        node *curr;
        for(curr = table[index]; curr!=NULL; curr = curr->next) {
            if(strcmp(curr->key, key) == 0) {
                curr->time = time(NULL);
                // printf("\nitem found\n");
                // printf("\n\nget key: %s :  %s\n\n", key, buffer_string(curr->value));

                pthread_rwlock_unlock(&full_lock);

                return curr->value;
            }
        }
        //element didn't exist inside hash table
        if(curr==NULL){
            //printf("\nno such element with key: %s\n", key);
            pthread_rwlock_unlock(&full_lock);
            return NULL;
        }
        printf("\nthis should not happen\n");
    }
    pthread_rwlock_unlock(&full_lock);
    return NULL;
}

//deletes an element with the given key
//this is on average O(1)
void remove_element(char *key) {
    pthread_rwlock_wrlock(&full_lock);
    // printf("\n\nremoving\n\n");
    //
    //
    // printf("\nremoving an element from the cache\n");
    // printf("\nremoving: key: %s\n", key);

    //printf("\ngets past mutex\n");
    uint64_t index = hash_function(key)%TABLE_SIZE;

    if(table[index] == NULL) {
        //printf("no such element with key: %s", key);
        pthread_rwlock_unlock(&full_lock);
        return;
        // exit(1);
    }
    else{


        //the element possibly exists

        //checking the first element of the chain
        node *first = table[index];
        if(strcmp(first->key, key) == 0) {
            size -= sizeof(buffer_data(first->value));
            table[index] = first->next;
            buffer_free(first->value);
            free(first);
        }
        else {
            //if the first element in the chain
            //is not the element we are deleting
            node *curr;
            for(curr = table[index]; curr!=NULL; curr = curr->next) {
                //see if there is a next element
                if (curr->next != NULL) {
                    if(strcmp(curr->next->key, key) == 0) {
                        node *rem = curr->next;

                        size-=sizeof(buffer_data(rem->value));
                        curr->next = rem->next;
                        buffer_free(rem->value);
                        free(rem);
                    }
                }
            }
            //element didn't exist inside hash table
            if(curr==NULL){
                //printf("no such element with key: %s", key);
                pthread_rwlock_unlock(&full_lock);
                return;
                // exit(1);
            }
        }


    }
    pthread_rwlock_unlock(&full_lock);

}

//evicts the least recently used element from the cache
void evict() {

    //pthread_rwlock_rdlock(&full_lock);

    //pthread_mutex_lock(&full_lock);
    node *to_evict;
    time_t least;

    //set the least time_t
    //to the time of the first element found
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        if (table[i] != NULL){
            least = table[i]->time;
            break;
        }
    }

    //find the least time
    //and the element to be evicted
    for (size_t i = 0; i < TABLE_SIZE; i++) {

        node *curr = table[i];
        if (curr != NULL && curr->time < least){
            least = curr->time;
            to_evict = curr;
        }
    }

    //pthread_rwlock_unlock(&full_lock);
    remove_element(to_evict->key);
    //pthread_mutex_unlock(&full_lock);

}

//updates the table until the size of the table
//no longer exceeds the MAX_CACHE_SIZE
//using evict
void update_table(){
    while (size > MAX_CACHE_SIZE){
        evict();
    }
}

//hashes a string
//this is a version of the sdbm hash function used in the SDBM project
//this version was used in gawk
uint64_t hash_function(char *str) {
    if (str == NULL){
        return size%TABLE_SIZE;
    }
    uint64_t hash = 0;
    uint64_t i;

   for (i = 0; str!=NULL && i < strlen(str); ++str, ++i)
   {
      hash = (*str) + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}

//frees the hash_table and the data within
void free_hash_table(){
    pthread_rwlock_wrlock(&full_lock);
    //loops through table
    for (size_t i = 0; i < TABLE_SIZE; i++) {

        node *curr = table[i];
        if (curr != NULL){
            remove_node_chain(curr);
            table[i] = NULL;
        }
    }
    pthread_rwlock_unlock(&full_lock);
    pthread_rwlock_destroy(&full_lock);
}


//recursively removes a singly-linked list chain from a bucket
void remove_node_chain(node *head){
    //pthread_mutex_lock(&full_lock);
    if (head == NULL){
        return;
    }

    size -= sizeof(buffer_data(head->value));
    node *next = head->next;
    buffer_free(head->value);
    free(head);

    //pthread_mutex_unlock(&full_lock);
    remove_node_chain(next);
}

//displays the entire hash table in the terminal
void display_table() {
    pthread_rwlock_rdlock(&full_lock);

    uint64_t index;
    //loop through the buckets
    for(index=0; index<TABLE_SIZE; index++) {
        printf("\nentries at index %lu\n",index);
        if(table[index] == NULL) {
            printf("No Hash Entry");
        }
        else
        {
            //loops through the current bucket
            for(node *curr = table[index]; curr != NULL; curr=curr->next){
                printf("%s:%s->", curr->key, buffer_string(curr->value));
            }
        }

	printf("\n");
    }
    pthread_rwlock_unlock(&full_lock);

}
