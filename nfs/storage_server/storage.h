#ifndef STORAGE_H
#define STORAGE_H

#include "../common/include/dataTypes.h"
#include "../common/include/colors.h"
#include "../common/include/defs.h"
#include "../common/include/network.h"
#include "../common/include/errors.h"

#define MAX_PATHS 1024

/**
 * @brief Structure to store path information for each file
 * @details Contains the file path and its last modification timestamp
 */
typedef struct
{
    char path[MAX_PATH_LENGTH]; /**< Full path of the file */
    time_t timestamp;           /**< Last modification timestamp */
} PathInfo;

/**
 * @brief Sets the socket to blocking mode
 *
 * @param sockfd The socket file descriptor
 * @return int Returns 0 on success, -1 on failure
 * @throws Will throw error if fcntl operations fail
 */
int set_socket_blocking(int sockfd);

/**
 * @brief Initializes the storage server with given parameters
 *
 * @param nm_sockfd Socket file descriptor for naming server connection
 * @param dma_sockfd Socket file descriptor for DMA operations
 * @param init_sock Initial socket file descriptor for initial communication
 * @return int Returns 0 on successful initialization, -1 on failure
 * @throws Will throw error if socket creation fails
 * @note Requires proper network configuration before calling
 */
int server_init(int nm_sockfd, int dma_sockfd, int init_sock);

/**
 * @brief Callback function to process and send file paths to naming server
 *
 * @param fpath File path being processed
 * @param sb Pointer to stat structure containing file information
 * @param typeflag Type of file (regular file, directory, etc.)
 * @return int Returns 0 to continue traversal, -1 on error
 * @note Called by ftw() during directory traversal
 */
int send_file_paths(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

/**
 * @brief Thread function to handle sending data to client
 *
 * @param arg Thread arguments (client_data_t structure)
 * @return void* Returns NULL on completion
 * @throws Will throw error if socket operations fail
 * @note Runs in separate thread
 */
void *send_to_client(void *arg);

/**
 * @brief Thread function to handle receiving data from naming server
 *
 * @param arg Thread arguments (write_data_t structure)
 * @return void* Returns NULL on completion
 * @throws Will throw error if socket operations fail
 * @note Runs in separate thread
 */
void *receive_from_ns(void *arg);

/**
 * @brief Recursively deletes files and directories
 *
 * @param fpath Path to file/directory to delete
 * @param sb Pointer to stat structure containing file information
 * @param typeflag Type of file (regular file, directory, etc.)
 * @return int Returns 0 on successful deletion, -1 on error
 * @throws Will throw error if deletion fails
 * @warning Use with caution - permanent deletion
 */
int delete_files_and_directories(const char *fpath, const struct stat *sb, int typeflag);

/**
 * @brief Copies a file from source to destination
 *
 * @param source Path to source file
 * @param destination Path to destination file
 * @return int Returns 0 on successful copy, -1 on error
 * @throws Will throw error if file operations fail
 * @note Creates destination file if it doesn't exist
 * @warning Will overwrite destination file if it exists
 */
int copy_file(const char *source, const char *destination);

/**
 * @brief Thread function to process requests from the naming server
 *
 * @param arg Thread arguments (socket file descriptor)
 * @return void* Returns NULL on completion
 * @throws Will throw error if socket operations fail
 * @note Runs in separate thread
 */
void *process_naming_server_requests(void *arg);

/**
 * @brief Thread function to process requests from the client
 *
 * @param arg Thread arguments (socket file descriptor)
 * @return void* Returns NULL on completion
 * @throws Will throw error if socket operations fail
 * @note Runs in separate thread
 */
void *process_client_requests(void *arg);

#endif // STORAGE_H