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

Basically any Unix-like system should work. If you're on Windows, you can use WSL or a virtual machine, but your mileage may vary.

### Building
```bash
make                        # Build the entire project
make clean                  # Clean up build artifacts
```

### Running
1. Start the Naming Server
   ```bash
   ./naming_server.out
   ```

   Enter the port and the naming server will start running.

2. Start the Storage Server
   ```bash
    ./storage_server.out <naming_server_ip> <naming_server_port>
    ```

    You can start as many storage servers as you want. The naming server will automatically detect and register them.

3. Start the Client
    ```bash
    ./client.out <naming_server_ip> <naming_server_port>
    ```
    
    Use the client interface to perform file operations.

## 📂 Project Structure
```
.
├── LICENSE
├── Makefile
├── README.md
├── most_wanted
├── nfs
│   ├── client
│   │   ├── client.c
│   │   ├── client.h
│   │   └── revenge.sh
│   ├── common
│   │   ├── include
│   │   │   ├── colors.h
│   │   │   ├── dataTypes.h
│   │   │   ├── defs.h
│   │   │   ├── errors.h
│   │   │   ├── hashMap.h
│   │   │   ├── linkedList.h
│   │   │   └── network.h
│   │   └── src
│   │       ├── errors.c
│   │       ├── hashMap.c
│   │       ├── linkedList.c
│   │       └── network.c
│   ├── naming_server
│   │   ├── cluster.c
│   │   ├── cluster.h
│   │   ├── commSS.c
│   │   ├── commSS.h
│   │   ├── fileSystem.c
│   │   ├── fileSystem.h
│   │   ├── naming.c
│   │   └── naming.h
│   └── storage_server
│       ├── storage.c
│       └── storage.h
└── protocols.md
```

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

<!-- LICENSE -->
## 📜 License

Distributed under the GNU General Public License v3.0. See [`LICENSE`](./LICENSE) for more information.

---
Built with ❤️ at IIIT Hyderabad