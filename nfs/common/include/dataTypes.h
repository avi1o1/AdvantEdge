/**
 * @file dataTypes.h
 * @brief Core data structures for the distributed file system
 * @details Defines the inode structure and related components used for
 *          file system management.
 */

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "defs.h"
#include "linkedList.h"
#include "hashMap.h"

/**
 * @struct Inode
 * @brief Represents a file or directory in the distributed file system
 *
 * @details This structure maintains metadata about files and directories,
 *          including their attributes, permissions, and synchronization primitives.
 */
typedef struct Inode
{
    int inodeNumber;     /**< Unique identifier for the inode */
    char *userPath;      /**< User-visible path in the file system */
    int isDir;           /**< Flag indicating if this is a directory (1) or file (0) */
    LinkedList children; /**< List of child inodes for directories */

    int size;                    /**< Size of the file in bytes */
    int permission;              /**< Permission bitmap for access control */
    time_t creationTime;         /**< Time when the inode was created */
    time_t lastModificationTime; /**< Time of last content modification */
    time_t lastAccessTime;       /**< Time of last access */

    int readersCount;          /**< Number of current readers */
    int writersCount;          /**< Number of current writers */
    bool toBeDeleted;          /**< Flag indicating if marked for deletion */
    pthread_mutex_t available; /**< Mutex for synchronizing access */

    int clusterID; /**< ID of the storage server containing the data */
} Inode;

#endif // DATA_TYPES_H