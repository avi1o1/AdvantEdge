#ifndef COMMSS_H
#define COMMSS_H

#include "cluster.h"
#include "../common/include/defs.h"
#include "../common/include/dataTypes.h"
#include "../common/include/network.h"

/**
 * @brief Creates a file on the Storage Server.
 * 
 * @param inode Pointer to the inode structure for the file to be created.
 * @return int 0 on success, non-zero on failure.
 */
int createFileSS(Inode *inode);

/**
 * @brief Reads a file from the Storage Server.
 * 
 * @param inode Pointer to the inode structure for the file to be read.
 * @return char* Pointer to the data read from the file, or NULL on failure.
 */
char *readFileSS(Inode *inode);

/**
 * @brief Writes data to a file on the Storage Server.
 * 
 * @param inode Pointer to the inode structure for the file to be written.
 * @param data Pointer to the data to be written.
 * @param async Boolean indicating if the write should be asynchronous.
 * @param ip Client IP address for sending completion message.
 * @param port Client port for sending completion message.
 * @return int 0 on success, non-zero on failure.
 */
int writeFileSS(Inode *inode, void *data, bool async, char *ip, int port);

/**
 * @brief Copies a file from one inode to another on the Storage Server.
 * 
 * @param fromInode Pointer to the source inode structure.
 * @param toInode Pointer to the destination inode structure.
 * @return int 0 on success, non-zero on failure.
 */
int copyFileSS(Inode *fromInode, Inode *toInode);

/**
 * @brief Deletes a file from the Storage Server.
 * 
 * @param inode Pointer to the inode structure for the file to be deleted.
 * @return int 0 on success, non-zero on failure.
 */
int deleteFileSS(Inode *inode);

/**
 * @brief Renames a file on the Storage Server.
 * 
 * @param oldPath Pointer to the current file path.
 * @param newInode Pointer to the new inode structure.
 * @return int 0 on success, non-zero on failure.
 */
int renameFileSS(char *oldPath, Inode *newInode);

/**
 * @brief Sends a hello message to the Storage Server.
 * 
 * @param storageServer Pointer to the Storage Server structure.
 * @return int 0 on success, non-zero on failure.
 */
int helloSS(StorageServer *storageServer);


/**
 * @brief Copies data from one storage server to another for a specific path.
 *
 * This function copies data from the source storage server (`fromSS`) to the 
 * destination storage server (`toSS`) for the specified path.
 *
 * @param fromSS Pointer to the source StorageServer structure.
 * @param toSS Pointer to the destination StorageServer structure.
 * @param path The path specifying the data to be copied.
 * @param isFile Boolean indicating if the path is a file or directory.
 * @return int Returns 0 on success, or a negative error code on failure.
 */
int copySpecificSStoSpecificSS(StorageServer *fromSS, StorageServer *toSS, char *path, bool isFile);

#endif // COMMSS_H