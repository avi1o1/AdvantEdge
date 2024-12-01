/**
 * @file defs.h
 * @brief Common definitions and includes for the distributed file system
 * @details Contains all standard library includes needed across the project,
 *          including C standard library, POSIX library, and networking components.
 */

#ifndef DEFS_H
#define DEFS_H

#define FILE_MAPPING_SIZE 1024
#define MAX_WRITE_LENGTH 2048
#define CONNECTION_TIMEOUT 3
#define MAX_PATH_LENGTH 256
#define BIG_CHUNGUS 1000000
#define MINI_CHUNGUS 4096
#define MAX_OPEN_DIRS 20

// #define _POSIX_C_SOURCE 2024L
// #define _GNU_SOURCE
#define _XOPEN_SOURCE 800
#define _GNU_SOURCE

// C standard library
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// POSIX library
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <ftw.h>
#include <dirent.h>

// Network library
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#endif // DEFS_H
