#ifndef CLUSTER_H
#define CLUSTER_H

#include "../common/include/dataTypes.h"

/**
 * @file cluster.h
 * @brief Header file containing cluster management data structures and functions
 * @details This file defines the core data structures and functions for managing
 * storage clusters, including storage servers, clusters, and super clusters.
 */

#define SUPERCLUSTER_SIZE 1024

/**
 * @brief Data structure representing a storage server node
 * @details Contains information about a single storage server including its
 * identification, status, and network details
 */
typedef struct StorageServer
{
    int id;      /**< Unique identifier for the storage server */
    int isAlive; /**< Flag indicating if the server is currently active (1) or inactive (0) */
    char *ip;    /**< IP address of the storage server */
    int port;    /**< Port number where the storage server listens */
    int DMAport; /**< Port number where the storage server listens for DMA requests */
} StorageServer;

/**
 * @brief Data structure representing a cluster of storage servers
 * @details Contains cluster management information including root inode,
 * list of storage servers, and synchronization primitives
 */
typedef struct Cluster
{
    int id;                    /**< Unique identifier for the cluster */
    Inode *root;               /**< Root inode of the cluster's file system */
    LinkedList storageServers; /**< List of storage servers in this cluster */
    int readerCount;           /**< Number of active readers */
    bool isWriterActive;       /**< Flag indicating if a writer is currently active */
    pthread_mutex_t lock;      /**< Mutex for synchronizing cluster access */
} Cluster;

/**
 * @brief Data structure representing a super cluster containing multiple clusters
 * @details Manages a fixed-size array of cluster pointers with synchronization
 * capabilities
 */
typedef struct SuperCluster
{
    int from;                             /**< Starting cluster ID in range */
    int to;                               /**< Ending cluster ID in range */
    pthread_mutex_t lock;                 /**< Mutex for synchronizing super cluster access */
    Cluster *clusters[SUPERCLUSTER_SIZE]; /**< Array of cluster pointers */
} SuperCluster;

/** @brief Linked list type definition for managing multiple super clusters */
typedef LinkedList SuperClusterList;

/**
 * @brief Gets the current number of active clusters
 * @return int Number of active clusters
 */
int getActiveClusterCount();

/**
 * @brief Calculates cluster ID from storage server ID
 * @param storageServerID The storage server ID
 * @return int The calculated cluster ID
 */
int getClusterID(int storageServerID);

/**
 * @brief Initializes the super cluster list
 * @details Creates and sets up the initial state of the super cluster list
 * including any necessary memory allocation and initialization
 */
void initSuperClusterList();

/**
 * @brief Creates a new super cluster
 * @param from The starting cluster ID for the new super cluster
 * @return Pointer to the newly created SuperCluster structure
 * @details Allocates and initializes a new super cluster structure with the
 * specified starting ID
 */
SuperCluster *createSuperCluster(int from);

/**
 * @brief Adds a new storage server to the system
 * @param ip The IP address of the storage server to add
 * @param port The port number of the storage server
 * @param DMAport The port number for DMA requests
 * @return int Returns 0 on success, negative value on failure
 * @details Registers a new storage server in the system with the specified
 * network parameters
 */
int addStorageServer(char *ip, int port, int DMAport);

/**
 * @brief Retrieves a storage server by its ID
 * @param storageServerID The unique identifier of the storage server to retrieve
 * @return StorageServer* Pointer to the found StorageServer structure, NULL if not found
 * @details Searches for and returns a pointer to the storage server with the
 * specified ID
 */
StorageServer *getStorageServer(int storageServerID);

/**
 * @brief Retrieves a storage server from a specific cluster by index
 * @param index The index of the storage server in the cluster
 * @param clusterID The unique identifier of the cluster
 * @return StorageServer* Pointer to the found StorageServer structure, NULL if not found
 * @details Searches for and returns a pointer to the storage server with the
 * specified index in the given cluster
 */
StorageServer *getStorageServerFromCluster(int index, int clusterID);

/**
 * @brief Retrieves the first alive storage server from a specific cluster
 * @param clusterID The unique identifier of the cluster
 * @return StorageServer* Pointer to the found StorageServer structure, NULL if not found
 * @details Searches for and returns a pointer to the first alive storage server
 * in the given cluster
 */
StorageServer *getAliveStorageServerFromCluster(int clusterID);

#endif // CLUSTER_H