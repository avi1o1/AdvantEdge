# AdvantEdge NFS 🌐

A robust distributed Network File System implementation that enables seamless file operations across networked systems. Built as part of the Operating Systems and Networks course at IIIT Hyderabad.

## 🌟 Features

- **Distributed Architecture** 🏗️
  - Naming Server for centralized coordination
  - Multiple Storage Servers for distributed data storage
  - Client interface for seamless interaction

- **Core Operations** 📝
  - File/Directory Creation and Deletion
  - Synchronous and Asynchronous File Writing
  - File Reading and Information Retrieval
  - Audio File Streaming
  - Inter-Server File/Directory Copying

- **Advanced Capabilities** 🚀
  - Multiple Client Support with Concurrency Control
  - LRU Caching for Optimized Performance
  - Efficient Search using Advanced Data Structures
  - Comprehensive Error Handling
  - Data Replication and Backup
  - Server Recovery Management

## 🛠️ Technical Architecture

### Components

1. **Naming Server**
   - Central coordinator for the entire system
   - Manages directory structure and file locations
   - Handles client request routing
   - Implements LRU caching and efficient search

2. **Storage Servers**
   - Physical storage management
   - File operations execution
   - Data replication support
   - Asynchronous write capabilities

3. **Clients**
   - User interface for file system operations
   - Direct communication with storage servers
   - Support for concurrent operations

## 🚀 Getting Started

### Prerequisites

- C compiler (gcc recommended)
- POSIX-compliant system
- TCP/IP network connection

Basically any Unix-like system should work. If you're on Windows, you can use WSL or a virtual machine.

### Building
```bash
make                        # Build the entire project
make clean                  # Clean up build artifacts
```

### Running
1. Start the Naming Server
   ```bash
   ./naming_server
   ```

2. Start the Storage Server
   ```bash
    ./storage_server <naming_server_ip> <naming_server_port>
    ```

3. Start the Client
    ```bash
    ./client <naming_server_ip> <naming_server_port>
    ```

Use the client interface to perform file operations.

## 📂 Project Structure
```
nfs/
├── src/
│   ├── common/
│   │   ├── include/
│   │   │   ├── network.h        # Network utilities
│   │   │   ├── protocol.h       # Message protocols
│   │   │   └── errors.h         # Error codes
│   │   └── src/
│   │       ├── network.c
│   │       ├── protocol.c
│   │       └── errors.c
│   │
│   ├── naming_server/
│   │   ├── include/
│   │   │   ├── server.h         # NM server definitions
│   │   │   ├── storage_info.h   # SS tracking structures
│   │   │   ├── cache.h          # LRU cache implementation
│   │   │   └── search.h         # Search optimization structures
│   │   └── src/
│   │       ├── server.c
│   │       ├── storage_info.c
│   │       ├── cache.c
│   │       └── search.c
│   │
│   ├── storage_server/
│   │   ├── include/
│   │   │   ├── server.h         # SS server definitions
│   │   │   ├── file_ops.h       # File operation handlers
│   │   │   └── backup.h         # Backup management
│   │   └── src/
│   │       ├── server.c
│   │       ├── file_ops.c
│   │       └── backup.c
│   │
│   └── client/
│       ├── include/
│       │   ├── client.h         # Client definitions
│       │   └── operations.h     # Client operations
│       └── src/
│           ├── client.c
│           └── operations.c
│
└── Makefile                     # Build system
```

## 📅 Project Plan

### Basic Infrastructure (Week 1)
1. ~~Set up the project structure and build system~~
   - ~~Create directories~~
   - ~~Write basic Makefile~~
   - ~~Set up version control workflow~~

2. Implement basic TCP socket communication
   - Create basic server socket in NM
   - Implement basic client connection
   - Test basic connectivity

3. Implement basic Storage Server registration
   - Create SS registration protocol
   - Store SS information in NM
   - Test SS connection and registration

### Core Features (Week 2)
1. Implement basic file operations
   - Create file read/write operations in SS
   - Implement file creation/deletion
   - Add directory operations

2. Implement Naming Server core
   - Add path management
   - Implement SS lookup
   - Create basic client request handling

3. Implement basic client operations
   - Add file operation requests
   - Implement direct SS communication
   - Test basic end-to-end operations

4. Implement search optimization
   - Add trie/hashmap for path lookup
   - Implement LRU cache
   - Optimize search operations

5. Add concurrent access handling
   - Implement file locking mechanism
   - Add multi-client support
   - Handle synchronization issues

6. Implement asynchronous operations
   - Add async write support
   - Implement write queuing
   - Add notification system

### Reliability Features (Week 3)
1. Implement backup system
   - Add SS replication
   - Implement async duplication
   - Add failure detection

2. Add error handling
   - Implement error codes
   - Add proper error responses
   - Implement timeout handling

3. Implement logging and monitoring
   - Add comprehensive logging
   - Implement operation tracking
   - Add system monitoring

## 👥 Team Members

- **Anirudh Sankar**
- **Arihant Tripathy** [![wakatime](https://wakatime.com/badge/user/77cdaa68-53d6-4cf6-8c9c-7ec147407ce9/project/1589f3c7-f137-4020-b5a1-25bbf0a192ba.svg)](https://wakatime.com/badge/user/77cdaa68-53d6-4cf6-8c9c-7ec147407ce9/project/1589f3c7-f137-4020-b5a1-25bbf0a192ba)
- **Aviral Gupta**
- **Mohit Kumar Singh**

## 🙏 Acknowledgments

- Operating Systems and Networks Course, IIIT Hyderabad
- [POSIX](https://pubs.opengroup.org/onlinepubs/9699919799/) standards
- [Claude](https://claude.ai) and [ChatGPT](https://chatgpt.com) for inspiration
- Beyoncé


---
Built with ❤️ at IIIT Hyderabad