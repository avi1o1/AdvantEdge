/**
 * @file hashMap.h
 * @brief Generic hash map implementation
 * @details Provides a thread-safe hash map implementation for storing
 *          key-value pairs with dynamic resizing capabilities.
 */

#ifndef _Hash_Map_H_
#define _Hash_Map_H_

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

/**
 * @struct HashNode
 * @brief Node structure for the hash map
 * 
 * @details Represents a single key-value pair in the hash map,
 *          with support for collision handling through chaining.
 */
typedef struct HashNode
{
    char *key;              /**< String key for the hash map entry */
    void *value;            /**< Generic pointer to the stored value */
    struct HashNode *next;  /**< Pointer to next node in case of collision */
} HashNode;

/**
 * @struct HashMapStruct
 * @brief Main hash map structure
 * 
 * @details Contains the hash map metadata and array of buckets,
 *          with thread synchronization support.
 */
typedef struct HashMapStruct {
    int size;              /**< Current number of elements in the map */
    int capacity;          /**< Total number of buckets */
    HashNode **data;       /**< Array of hash node pointers */
    pthread_mutex_t lock;  /**< Mutex for thread-safe operations */
} HashMapStruct;

/** @brief Typedef for HashMapStruct pointer for easier usage */
typedef HashMapStruct *HashMap;

/**
 * @brief Creates a new hash map
 * @param capacity Initial number of buckets in the hash map
 * @return Pointer to the newly created hash map
 */
HashMap create_hash_map(int capacity);

/**
 * @brief Retrieves a value from the hash map
 * @param key The key to look up
 * @param hashMap The hash map to search in
 * @return Pointer to the value if found, NULL otherwise
 */
void *get_hash_map(char *key, HashMap hashMap);

/**
 * @brief Sets a key-value pair in the hash map
 * @param key The key to set
 * @param value Pointer to the value to store
 * @param hashMap The hash map to modify
 */
void set_hash_map(char *key, void *value, HashMap hashMap);

/**
 * @brief Removes a key-value pair from the hash map
 * @param key The key to remove
 * @param hashMap The hash map to modify
 */
void delete_hash_map(char *key, HashMap hashMap);

/**
 * @brief Frees all resources used by the hash map
 * @param hashMap The hash map to destroy
 */
void destroy_hash_map(HashMap hashMap);

/**
 * @brief Prints the contents of the hash map
 * @param hashMap The hash map to print
 */
void print_hash_map(HashMap hashMap);

#endif // _Hash_Map_H_
