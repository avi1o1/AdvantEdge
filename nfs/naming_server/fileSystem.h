#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../common/include/dataTypes.h"
#include "../common/include/hashMap.h"
#include "../common/include/defs.h"
#include "commSS.h"

extern HashMap fileToInode;
extern int inodeNumber;

extern pthread_mutex_t fileMappings;
extern pthread_mutex_t inodeNumberMutex;

/**
 * @brief Get the next available inode number
 * @return The next available inode number
 */
int getNextInodeNumber();

/**
 * @brief Create the file mappings hash map
 * @return None
 */
void initFileMappings();

/**
 * @brief Add a file to the file mappings
 * @param userPath User provided path to the file (assumes all path starts with '/')
 * @return 0 if successful, -1 on error
 */
int addFile(char *userPath, int isDir);

/**
 * @brief Delete a file from the file mappings
 * @param userPath User provided path to the file
 * @return 0 if successful, -1 on error
 */
int deleteFile(char *userPath);

/**
 * @brief Get the Inode object
 * @param userPath
 * @return Inode*
 */
Inode *getInode(char *userPath);

/**
 * @brief Rename a file in the file mappings
 * @param path User provided path to the file
 * @param newName New name for the file
 * @return 0 if successful, -1 on error
 */
int renameFile(char *path, char *newName);

/**
 * @brief Add a file to the file mappings without any checks
 * @param userPath User provided path to the file
 * @param isDir Boolean indicating if the file is a directory
 * @param clusterID Cluster ID where the file is stored
 * @return 0 if successful, -1 on error
 */
int justAddFileToMappings(char *userPath, int isDir, int clusterID);

#endif // FILESYSTEM_H