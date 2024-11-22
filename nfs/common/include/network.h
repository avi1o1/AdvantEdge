/**
 * @file network.h
 * @brief Common network connection utilities for NFS components
 * @details This module provides common networking functionality used by both
 * client and storage server components to connect to the naming server
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "defs.h"
#include "colors.h"

/**
 * @brief Connect to a server using specified IP and port
 * @details Creates a socket and establishes a TCP connection with configurable timeout
 *
 * @param server_ip IP address of the server to connect to
 * @param server_port Port number of the server
 * @param timeout_sec Timeout in seconds (use 0 for no timeout)
 * @return int Socket file descriptor if successful, -1 on error
 */
int connect_to_server(const char *server_ip, int server_port, int timeout_sec);

/**
 * @brief Sets socket timeout options
 * @details Configures both send and receive timeouts for a socket
 *
 * @param sockfd Socket file descriptor
 * @param timeout_sec Timeout in seconds
 * @return int 0 if successful, -1 on error
 */
int set_socket_timeout(int sockfd, int timeout_sec);

/**
 * @brief Safely send data over a socket
 * @details Ensures complete message transmission with error handling
 *
 * @param sockfd Socket file descriptor
 * @param data Pointer to data buffer
 * @param len Length of data to send
 * @return int Number of bytes sent if successful, -1 on error
 */
int send_data(int sockfd, const void *data, size_t len);

/**
 * @brief Safely receive data from a socket
 * @details Ensures complete message reception with error handling
 *
 * @param sockfd Socket file descriptor
 * @param buffer Buffer to store received data
 * @param len Maximum length of data to receive
 * @return int Number of bytes received if successful, -1 on error
 */
int receive_data(int sockfd, void *buffer, size_t len);

/**
 * @brief Retrieves the actual IPv4 address of the machine.
 *
 * This function iterates over network interfaces to find a non-loopback IPv4 address
 * and stores it in the provided buffer.
 *
 * @param[out] ip_buffer Buffer to store the resulting IP address.
 */
void getActualIP(char *ip_buffer);

/**
 * @brief Retrieves the local port number of a connected socket.
 *
 * @param sockfd The socket file descriptor.
 * @return int The local port number on success, -1 on failure.
 *
 * This function uses the `getsockname` system call to retrieve the local address
 * and port number associated with a socket. It then extracts and returns the port number.
 *
 * @note The socket must be connected before calling this function.
 */
int get_local_port(int sockfd);

/**
 * @brief Clear the socket buffer to prevent data loss
 *
 * @param sockfd Socket file descriptor
 */
void clear_socket_buffer(int sockfd);

#endif // NETWORK_H