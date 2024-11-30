/**
 * @file linkedList.h
 * @brief Generic linked list implementation
 * @details Provides a thread-safe linked list implementation for storing
 *          and managing arbitrary data types.
 */

#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

/**
 * @struct LinkedListNode
 * @brief Node structure for the linked list
 * 
 * @details Represents a single element in the linked list,
 *          storing generic data and a pointer to the next node.
 */
typedef struct LinkedListNode
{
    void *data;             /**< Generic pointer to the stored data */
    struct LinkedListNode *next; /**< Pointer to the next node */
} LinkedListNode;

/**
 * @struct LinkedListStruct
 * @brief Main linked list structure
 * 
 * @details Contains the linked list metadata and head pointer,
 *          with thread synchronization support.
 */
typedef struct LinkedListStruct {
    LinkedListNode *head;   /**< Pointer to the first node */
    int size;              /**< Current number of elements in the list */
    pthread_mutex_t lock;   /**< Mutex for thread-safe operations */
} LinkedListStruct;

/** @brief Typedef for LinkedListStruct pointer for easier usage */
typedef LinkedListStruct *LinkedList;

/**
 * @brief Creates a new empty linked list
 * @return Pointer to the newly created linked list
 */
LinkedList create_linked_list();

/**
 * @brief Retrieves an element at a specific position
 * @param pos Zero-based index of the element to retrieve
 * @param linkedList The linked list to search in
 * @return Pointer to the data if found, NULL otherwise
 */
void *get_linked_list(int pos, LinkedList linkedList);

/**
 * @brief Adds an element to the end of the list
 * @param data Pointer to the data to store
 * @param linkedList The linked list to modify
 */
void append_linked_list(void *data, LinkedList linkedList);

/**
 * @brief Inserts an element at a specific position
 * @param data Pointer to the data to store
 * @param pos Zero-based index where to insert the element
 * @param linkedList The linked list to modify
 */
void insert_linked_list(void *data, int pos, LinkedList linkedList);

/**
 * @brief Removes an element at a specific position
 * @param pos Zero-based index of the element to remove
 * @param linkedList The linked list to modify
 */
void delete_linked_list(int pos, LinkedList linkedList);

/**
 * @brief Removes the first occurrence of specific data
 * @param data Pointer to the data to remove
 * @param linkedList The linked list to modify
 */
void delete_by_data_linked_list(void *data, LinkedList linkedList);

/**
 * @brief Frees all resources used by the linked list
 * @param linkedList The linked list to destroy
 */
void destroy_linked_list(LinkedList linkedList);

/**
 * @brief Prints the contents of the linked list
 * @param linkedList The linked list to print
 */
void print_linked_list(LinkedList linkedList);

/**
 * @brief Copies the contents of one linked list to another
 * @param source The source linked list to copy from
 * @param destination The destination linked list to copy to
 */
void copy_linked_list(LinkedList source, LinkedList destination);

#endif // _LINKEDLIST_H_