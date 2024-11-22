#ifndef NAMING_H
#define NAMING_H

#include "../common/include/defs.h"
#include "../common/include/dataTypes.h"
#include "../common/include/network.h"
#include "../common/include/errors.h"
#include "cluster.h"
#include "fileSystem.h"

int NS_PORT;    // Naming server port number
char NS_IP[16]; // Naming server IP address

/**
 * @brief Sets up the server socket for listening.
 *
 * This function prompts the user for a port number, retrieves the machine's actual IP address,
 * and configures a socket bound to that IP address and port. The socket is prepared for
 * accepting incoming connections.
 *
 * @return int The server socket file descriptor, or terminates the program on error.
 */
int setupServer();

/**
 * @brief Handles client requests.
 */
void *handleClient(void *arg);

/**
 * @brief Handles storage server requests.
 */
void *initializeStorageServer(void *arg);

/**
 * @brief Creates file/directory, given the path.
 * @param isFile: true if file, false if directory
 * @param path: path of the file/directory
 * @return int 0 on success, -1 on failure
 */
int CreateCL(bool isFile, char *path);

/**
 * @brief Copies the file/directory from source to destination.
 * @param isFile: true if file, false if directory
 * @param srcPath: source path
 * @param dstPath: destination path
 * @return int 0 on success, -1 on failure
 */
int CopyCL(bool isFile, char *srcPath, char *dstPath);

/**
 * @brief Deletes file/directory, given the path.
 * @param isFile: true if file, false if directory
 * @param path: path of the file/directory
 * @return int 0 on success, -1 on failure
 */
int DeleteCL(bool isFile, char *path);

/**
 * @brief Renames file/directory, given the path.
 * @param isFile: true if file, false if directory
 * @param path: path of the file/directory
 * @param newName: new name of the file/directory
 * @return int 0 on success, -1 on failure
 */
int RenameCL(bool isFile, char *path, char *newName);

/**
 * @brief Streams audio file, given the path.
 * @param path: path of the file/directory
 * @return int 0 on success, -1 on failure
 */
int StreamCL(char *path);

/**
 * @brief Reads from file, given the path.
 * @param path: path of the file/directory
 * @return int 0 on success, -1 on failure
 */
int ReadCL(char *path);

/**
 * @brief Writes to file, given the path.
 * @param path: path of the file/directory
 * @param data: data to be written
 * @param async: true for asynchronous write, false for synchronous write
 * @param client_ip: IP address of the client
 * @param client_port: port number of the client
 * @return int 0 on success, -1 on failure
 */
int WriteCL(char *path, char *data, bool async, char *client_ip, int client_port);

/**
 * @brief Lists all the paths in the NFS.
 * @param data: buffer to store the list of paths
 * @param path: path of the file/directory to search in
 * @return int 0 on success, -1 on failure
 */
int ListPathsCL(char *data, char *path);

/**
 * @brief Retrieves information about a file from the Storage Server.
 * @param path: path of the file
 * @return char* Information about the file, or NULL on failure
 */
char *GetInfoCL(char *path);

#endif // NAMING_H