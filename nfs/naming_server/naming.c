#include "naming.h"
#include "cluster.h"
#include "fileSystem.h"

FILE *logFile;

// TODO: Acknowledging the client: If the SS decides to write data asynchronously (possibly judging by the data size), it sends an immediate acknowledgement to the client that their request has been accepted. After successful write operation, the SS again informs the client about the same through the NS since the connection between the SS and client will be closed after the first acknowledgment.
// TODO: Handling partial writes: If the SS goes down while it was in the process of an asynchronous write, the NS informs the client about this failure.
// TODO: Priority writes : If the client wants to write data synchronously irrespective of the response time overhead, it can prioritize the task while initiating a write request to the NS, say through a-- SYNC flag or any other reasonable method.

// TODO: LRU Caching: Implement LRU (Least Recently Used) caching for recent searches. By caching recently accessed information, the NM can expedite subsequent requests for the same data, further improving response times and system efficiency.

// TODO: Logging and Message Display: Implement a logging mechanism where the NM records every request or acknowledgment received from clients or Storage Servers. Additionally, the NM should display or print relevant messages indicating the status and outcome of each operation. This bookkeeping ensures traceability and aids in debugging and system monitoring.
// TODO: IP Address and Port Recording : The log should include relevant information such as IP addresses and ports used in each communication, enhancing the ability to trace and diagnose issues.

int setupServer()
{
    int serverFD;
    struct sockaddr_in address;
    int opt = 1;

    // Get a port number for the naming server
    printf("Provide a port number for the naming server: ");
    scanf("%d", &NS_PORT);

    // Get the actual IP address of the machine
    getActualIP(NS_IP);

    // Create socket file descriptor
    if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        log_NM_error(0);
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port
    if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        log_NM_error(3);
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(NS_IP);
    address.sin_port = htons(NS_PORT);

    // Bind the socket to the port
    if (bind(serverFD, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        log_NM_error(1);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverFD, SOMAXCONN) < 0)
    {
        log_NM_error(2);
        exit(EXIT_FAILURE);
    }

    return serverFD;
}

void *handleClient(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[MINI_CHUNGUS];
    sprintf(buffer, "Thread started for client socket: %d\n", client_sock);
    ssize_t bytes_read;

    int flag = 1;
    setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    while (true)
    {
        // Read data from client
        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0)
        {
            log_NM_error(7);
            break;
        }

        // Log the client request
        buffer[bytes_read] = '\0';
        printf("Received from client: %s\n", buffer);
        fflush(stdout);

        // Check if the client requested to exit
        if ((strcasecmp(buffer, "C|EXIT|0||") == 0) || (strcasecmp(buffer, "C|EXIT|1||") == 0))
        {
            printf("\033[1;33mClient requested to exit!\033[0m\n");
            break;
        }

        // Extract information from the client request
        char *saveptr = NULL;
        char *token = strtok_r(buffer, "|", &saveptr);
        if (token[0] != 'C')
        {
            log_NM_error(8);
            break;
        }

        char command[32] = {0};
        bool isFile = false;
        char srcPath[MAX_PATH_LENGTH] = {0};
        char dstPath[MAX_WRITE_LENGTH] = {0};

        token = strtok_r(NULL, "|", &saveptr);
        strcpy(command, token);
        token = strtok_r(NULL, "|", &saveptr);
        isFile = (strcasecmp(token, "0") == 0) ? false : true;

        token = strtok_r(NULL, "|", &saveptr);
        if (token != NULL)
            strcpy(srcPath, token);

        token = strtok_r(NULL, "|", &saveptr);
        if (token != NULL)
            strcpy(dstPath, token);

        // Pass the IP address and port number to the client for further communication
        char info[64];
        int Next_PORT = NS_PORT;
        char Next_IP[16];
        strcpy(Next_IP, NS_IP);
        int status;
        if (strcasecmp(command, "STREAM") == 0)
        {
            if (isFile)
            {
                Inode *iNode = getInode(srcPath);
                if (iNode == NULL || iNode->isDir || iNode->toBeDeleted)
                    status = -3;
                else
                {
                    if (iNode->writersCount > 0)
                        status = -6; // Error: File is being written to or busy
                    else
                    {
                        StorageServer *ss = getAliveStorageServerFromCluster(iNode->clusterID);
                        if (ss == NULL)
                            status = -1;
                        else
                        {
                            Next_PORT = ss->DMAport;
                            strcpy(Next_IP, ss->ip);
                            pthread_mutex_lock(&iNode->available);
                            iNode->readersCount++;
                            iNode->lastAccessTime = time(NULL);
                            pthread_mutex_unlock(&iNode->available);
                        }
                    }
                }
            }
            else
                status = -1;
        }
        else if (strcasecmp(command, "READ") == 0)
        {
            if (isFile)
            {
                Inode *iNode = getInode(srcPath);
                if (iNode == NULL || iNode->isDir || iNode->toBeDeleted)
                    status = -3;
                else
                {
                    // TODO: handle permissions everywhere
                    if (iNode->writersCount > 0)
                        status = -6; // Error: File is being written to or busy
                    else
                    {
                        StorageServer *ss = getAliveStorageServerFromCluster(iNode->clusterID);
                        if (ss == NULL)
                            status = -1;
                        else
                        {
                            // TODO: send hello to the storage server and if not acknowledged, mark as not alive and try again
                            Next_PORT = ss->DMAport;
                            strcpy(Next_IP, ss->ip);
                            pthread_mutex_lock(&iNode->available);
                            iNode->readersCount++;
                            iNode->lastAccessTime = time(NULL);
                            pthread_mutex_unlock(&iNode->available);
                            status = 0;
                        }
                    }
                }
            }
            else
                status = -1;
        }

        // Send response to client
        snprintf(info, sizeof(info), "N|%s|%d", Next_IP, Next_PORT);

        if (send(client_sock, info, strlen(info), MSG_NOSIGNAL) < 0)
        {
            log_NM_error(9);
            break;
        }

        char data[MINI_CHUNGUS];
        strcpy(data, "Job Done!");
        if (strcasecmp(command, "STREAM") && strcasecmp(command, "READ"))
        {
            // Pass the request to respective functions
            if (strcasecmp(command, "CREATE") == 0)
                status = CreateCL(isFile, srcPath);
            else if (strcasecmp(command, "COPY") == 0)
                status = CopyCL(isFile, srcPath, dstPath);
            else if (strcasecmp(command, "DELETE") == 0)
                status = DeleteCL(isFile, srcPath);
            else if (strcasecmp(command, "RENAME") == 0)
                status = RenameCL(isFile, srcPath, dstPath);
            else if (strcasecmp(command, "WRITE") == 0)
            {
                if (isFile)
                {
                    // Receive data
                    char writeData[BIG_CHUNGUS];
                    memset(writeData, 0, BIG_CHUNGUS);
                    while (true)
                    {
                        memset(buffer, 0, sizeof(buffer));
                        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
                        if (bytes_read <= 0)
                        {
                            log_NM_error(7);
                            break;
                        }
                        if (strcasecmp(buffer, "STOP") == 0)
                            break;

                        printf("Received data: %s\n", buffer);
                        strcat(writeData, buffer);
                    }
                    printf("Final write data: %s\n", writeData);

                    send(client_sock, data, strlen(data), MSG_NOSIGNAL);

                    memset(buffer, 0, sizeof(buffer));
                    recv(client_sock, buffer, sizeof(buffer) - 1, 0);
                    char *saveptr;
                    char *token = strtok_r(buffer, "|", &saveptr);
                    if (strcasecmp(token, "C"))
                    {
                        log_NM_error(8);
                        status = -1;
                    }
                    else
                    {
                        token = strtok_r(NULL, "|", &saveptr);
                        if (strcasecmp(token, "WRITE"))
                        {
                            log_NM_error(8);
                            status = -1;
                        }
                        else
                        {
                            int CL_PORT;
                            token = strtok_r(NULL, "|", &saveptr);
                            char *CL_IP = strdup(token);
                            token = strtok_r(NULL, "|", &saveptr);
                            CL_PORT = atoi(token);
                            status = WriteCL(srcPath, writeData, true, CL_IP, CL_PORT);
                            if (status)
                            {
                                printf("attempting to send error to client on the server port\n");
                                int cl_sock = connect_to_server(CL_IP, CL_PORT, CONNECTION_TIMEOUT);
                                if (cl_sock < 0)
                                {
                                    log_NM_error(9);
                                    break;
                                }
                                char error[MINI_CHUNGUS];
                                snprintf(error, sizeof(error), "N|ERROR|%d", status);
                                send(cl_sock, error, strlen(error), 0);
                                close(cl_sock);
                            }
                        }
                    }
                }
                else
                    status = -1;
            }
            else if (strcasecmp(command, "PRIORITYWRITE") == 0)
            {
                if (isFile)
                {
                    // Receive data
                    char writeData[BIG_CHUNGUS];
                    memset(writeData, 0, BIG_CHUNGUS);
                    while (true)
                    {
                        memset(buffer, 0, sizeof(buffer));
                        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
                        if (bytes_read <= 0)
                        {
                            log_NM_error(7);
                            break;
                        }
                        if (strcasecmp(buffer, "STOP") == 0)
                            break;

                        strcat(writeData, buffer);
                    }

                    status = WriteCL(srcPath, writeData, false, NULL, 0);
                }
                else
                    status = -1;
            }
            else if (strcasecmp(command, "LIST") == 0)
                status = ListPathsCL(data, srcPath);
            else if (strcasecmp(command, "INFO") == 0)
            {
                char *temp = GetInfoCL(srcPath);
                if (temp == NULL)
                    status = -3;
                else
                    strcpy(data, temp);
            }
            else
                status = -1;
        }

        if (status == -3)
        {
            log_NM_error(10);
            strcpy(data, "Oopsie Woopsie: File not found!");
        }
        else if (status == -4)
        {
            log_NM_error(11);
            strcpy(data, "Oopsie Woopsie: File already exists!");
        }
        else if (status == -5)
        {
            log_NM_error(12);
            strcpy(data, "Oopsie Woopsie: No storage servers!");
        }

        else if (status < 0)
        {
            log_NM_error(23);
            strcpy(data, "Oopsie Woopsie: Something went wrong!");
        }

        sleep(1);
        if (strcasecmp(command, "STREAM") == 0 || strcasecmp(command, "READ") == 0)
        {
            if (status)
            {
                if (send(client_sock, data, strlen(data), MSG_NOSIGNAL) < 0)
                {
                    printf("Write error\n");
                    break;
                }
            }
            else
            {
                // Wait until the client finishes reading / streaming
                char buffer[MINI_CHUNGUS];
                ssize_t bytes_read;
                while (true)
                {
                    bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0)
                    {
                        log_NM_error(7);
                        break;
                    }
                    buffer[bytes_read] = '\0';
                    if (strcasecmp(buffer, "Done") == 0)
                        break;
                }

                Inode *iNode = getInode(srcPath);
                if (iNode)
                {
                    pthread_mutex_lock(&iNode->available);
                    iNode->readersCount--;
                    pthread_mutex_unlock(&iNode->available);
                }
            }
        }
        else
        {
            if (strcasecmp(command, "WRITE") && send(client_sock, data, strlen(data), MSG_NOSIGNAL) < 0)
            {
                printf("Write error\n");
                break;
            }
        }
    }
    // Close client socket
    close(client_sock);
    log_NM_error(13);
    return NULL;
}

int CreateCL(bool isFile, char *path)
{
    return addFile(path, !isFile);
}

int CopyCL(bool isFile, char *srcPath, char *dstPath)
{
    Inode *srcInode = getInode(srcPath);
    if (!srcInode || srcInode->toBeDeleted)
        return -3;
    if (srcInode->writersCount > 0)
        return -6;
    addFile(dstPath, !isFile);
    Inode *dstInode = getInode(dstPath);
    if (!dstInode)
        return -7; // error: couldn't create destination file
    pthread_mutex_lock(&srcInode->available);
    srcInode->readersCount++;
    pthread_mutex_unlock(&srcInode->available);
    pthread_mutex_lock(&dstInode->available);
    dstInode->writersCount++;
    pthread_mutex_unlock(&dstInode->available);
    int ret_val = copyFileSS(srcInode, dstInode);
    pthread_mutex_lock(&srcInode->available);
    srcInode->readersCount--;
    pthread_mutex_unlock(&srcInode->available);
    pthread_mutex_lock(&dstInode->available);
    dstInode->writersCount--;
    pthread_mutex_unlock(&dstInode->available);

    return ret_val;
}

int DeleteCL(bool isFile, char *path)
{
    Inode *iNode = getInode(path);
    if (!iNode)
        return -3;
    if (iNode->writersCount > 0 || iNode->readersCount > 0)
        return -6;
    pthread_mutex_lock(&iNode->available);
    iNode->toBeDeleted = true;
    pthread_mutex_unlock(&iNode->available);
    return deleteFile(path);
}

int RenameCL(bool isFile, char *path, char *newName)
{
    Inode *iNode = getInode(path);
    if (!iNode || iNode->toBeDeleted)
        return -3;
    if (iNode->writersCount > 0)
        return -6;
    pthread_mutex_lock(&iNode->available);
    iNode->readersCount++;
    iNode->writersCount++;
    pthread_mutex_unlock(&iNode->available);
    int ret_val = renameFile(path, newName);
    pthread_mutex_lock(&iNode->available);
    iNode->readersCount--;
    iNode->writersCount--;
    pthread_mutex_unlock(&iNode->available);

    return ret_val;
}

int WriteCL(char *path, char *data, bool async, char *client_ip, int client_port)
{
    Inode *iNode = getInode(path);
    if (!iNode || iNode->toBeDeleted || iNode->isDir)
        return -3;
    if (iNode->readersCount || iNode->writersCount > 0)
        return -6;
    pthread_mutex_lock(&iNode->available);
    iNode->writersCount++;
    pthread_mutex_unlock(&iNode->available);

    int ret_val = writeFileSS(iNode, data, async, client_ip, client_port);

    if (!async && !ret_val)
    {
        pthread_mutex_lock(&iNode->available);
        iNode->writersCount--;
        iNode->lastModificationTime = time(NULL);
        iNode->lastAccessTime = time(NULL);
        iNode->size = strlen(data);
        pthread_mutex_unlock(&iNode->available);
    }

    return ret_val;
}

int ListPathsCL(char *data, char *path)
{
    printf("Listing all paths\n");
    data[0] = '\0';
    int numClusters = getActiveClusterCount();
    printf("Number of clusters: %d\n", numClusters);
    for (int i = 0; i < numClusters; i++)
    {
        StorageServer *ss = getAliveStorageServerFromCluster(i);
        if (!ss)
            return -1;
        char request[32];
        snprintf(request, sizeof(request), "S|LIST");

        // Connect to the naming server
        int ss_sock = connect_to_server(ss->ip, ss->port, CONNECTION_TIMEOUT);

        send(ss_sock, request, strlen(request), 0);
        // Receive the list of files from the other storage server and check if they exist in the file mappings
        while (true)
        {
            char buffer[MINI_CHUNGUS] = {0};
            ssize_t bytes_received;
            while ((bytes_received = recv(ss_sock, buffer, MINI_CHUNGUS, 0)) < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Resource temporarily unavailable, try again
                    usleep(1000); // Sleep for 1ms before retrying
                    continue;
                }
                else
                {
                    perror("recv");
                    break;
                }
            }

            if (bytes_received <= 0)
            {
                if (bytes_received == 0)
                {
                    printf("Storage server disconnected\n");
                    break;
                }
                perror("recv");
                continue;
            }

            char *saveptr;
            char *token = strtok_r(buffer, "|", &saveptr);

            // Check if it's a storage server message
            if (!token || strcasecmp(token, "S") != 0)
            {
                printf("Invalid message format\n");
                continue;
            }

            // Get message type
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing message type\n");
                continue;
            }

            // Handle file/directory path message
            if (strcasecmp(token, "P") == 0)
            {
                // Get server ID
                token = strtok_r(NULL, "|", &saveptr);
                if (!token)
                {
                    printf("Missing server ID\n");
                    continue;
                }
                int server_id = atoi(token);

                // Get file type (F/D)
                token = strtok_r(NULL, "|", &saveptr);
                if (!token)
                {
                    printf("Missing file type\n");
                    continue;
                }
                char type = token[0];

                // Get file path
                token = strtok_r(NULL, "|", &saveptr);
                if (!token)
                {
                    printf("Missing file path\n");
                    continue;
                }

                // Check if the file/directory path matches the requested path
                if (strncmp(token, path + 1, strlen(path) - 1) == 0)
                {
                    // Print the information
                    printf("Server %d: %c %s\n", server_id, type, token);

                    if (type == 'D')
                        strcat(data, COLOR_BLUE);
                    strcat(data, token);
                    if (type == 'D')
                        strcat(data, COLOR_RESET);
                    strcat(data, "\n");
                }

                // Send acknowledgment
                char ack[32];
                snprintf(ack, sizeof(ack), "N|ACK");
                send(ss_sock, ack, strlen(ack), 0);
            }
            // Handle list completion message
            else if (strcasecmp(token, "LIST_COMPLETE") == 0)
            {
                printf("Storage server completed file list successfully\n");
                break;
            }
            // Handle list failure message
            else if (strcasecmp(token, "LIST_NOTCOMPLETE") == 0)
            {
                printf("Storage server failed to complete file list\n");
                break;
            }
            else
            {
                printf("Unknown message type: %s\n", token);
                continue;
            }
        }
    }
    return 0;
}

char *GetInfoCL(char *path)
{
    Inode *inode = getInode(path);
    if (!inode)
    {
        printf("File not found\n");
        return NULL;
    }

    pthread_mutex_lock(&inode->available);
    inode->readersCount++;
    pthread_mutex_unlock(&inode->available);

    // Calculate required buffer size
    char timeStr[32];
    size_t bufSize = strlen(path) + 256; // Base size for fixed strings

    char *result = (char *)malloc(bufSize);
    if (!result)
    {
        printf("Memory allocation failed\n");
        pthread_mutex_lock(&inode->available);
        inode->readersCount--;
        pthread_mutex_unlock(&inode->available);
        return NULL;
    }

    int offset = snprintf(result, bufSize,
                          "userPath: %s\n"
                          "\tisDir: %s\n"
                          "\tsize: %d\n"
                          "\tpermission: %o\n",
                          path,
                          inode->isDir ? "true" : "false",
                          inode->size,
                          inode->permission);

    // Format creation time
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&inode->creationTime));
    offset += snprintf(result + offset, bufSize - offset,
                       "\tcreationTime: %s\n", timeStr);

    // Format modification time
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&inode->lastModificationTime));
    offset += snprintf(result + offset, bufSize - offset,
                       "\tlastModificationTime: %s\n", timeStr);

    // Format access time
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&inode->lastAccessTime));
    offset += snprintf(result + offset, bufSize - offset,
                       "\tlastAccessTime: %s\n", timeStr);

    // Add cluster ID
    snprintf(result + offset, bufSize - offset,
             "\tclusterID: %d\n", inode->clusterID);

    pthread_mutex_lock(&inode->available);
    inode->readersCount--;
    pthread_mutex_unlock(&inode->available);

    return result;
}

typedef struct StoSerInit
{
    char *path;
    bool isFile;
} StoSerInit;

void *initializeStorageServer(void *arg)
{
    int storage_sock = *(int *)arg;
    free(arg);
    printf("Thread started for storage server socket: %d\n", storage_sock);

    // Check if the storage server is already registered
    char buffer[MINI_CHUNGUS] = {0};
    memset(buffer, 0, sizeof(buffer));
    recv(storage_sock, buffer, MINI_CHUNGUS, 0);
    printf("%s\n", buffer);

    // Extract information from the storage server request <S|hasID|SS_IP|SS_PORT|DMA_PORT> <S|-1|10.211.55.3|6969|0>
    char *saveptr = NULL;
    char *token = strtok_r(buffer, "|", &saveptr);
    if (token[0] != 'S')
    {
        log_NM_error(8);
        close(storage_sock);
        return NULL;
    }

    token = strtok_r(NULL, "|", &saveptr);
    int SSid = atoi(token);
    token = strtok_r(NULL, "|", &saveptr);
    char *SS_IP = token;
    token = strtok_r(NULL, "|", &saveptr);
    int SS_PORT = atoi(token);
    token = strtok_r(NULL, "|", &saveptr);
    int DMA_PORT = atoi(token);

    if (SSid == -1)
    {
        // Register the storage server
        SSid = addStorageServer(SS_IP, SS_PORT, DMA_PORT);
        printf("Storage server registered with ID: %d\n", SSid);
    }

    // Send acknowledgment to the storage server (with the assigned ID)
    char response[32];
    snprintf(response, sizeof(response), "N|S|%d|", SSid);
    send(storage_sock, response, strlen(response), 0);

    // Check if any other storage server exists in that cluster
    int clusterID = getClusterID(SSid);
    if (clusterID == -1)
    {
        log_NM_error(14);
        close(storage_sock);
        return NULL;
    }

    int anotherSSid = -1;
    for (int i = 0; i < 3; i++)
    {
        StorageServer *anotherSS = getStorageServerFromCluster(i, clusterID);
        if (anotherSS && anotherSS->id != SSid)
        {
            anotherSSid = anotherSS->id;
            break;
        }
    }

    if (anotherSSid == -1)
        return NULL;

    printf("Another storage server in the cluster: %d\n", anotherSSid);

    sleep(5);

    // Request the storage server to send the list of files
    close(storage_sock);
    storage_sock = connect_to_server(SS_IP, SS_PORT, CONNECTION_TIMEOUT);
    char request[32];
    snprintf(request, sizeof(request), "S|LIST");
    send(storage_sock, request, strlen(request), 0);

    printf("Requesting file list from storage server: %d\n", SSid);

    int exists = 1;
    HashMap filesOnSS = create_hash_map(FILE_MAPPING_SIZE);

    LinkedList missing = create_linked_list();
    LinkedList extra = create_linked_list();

    // Receive the list of files from the storage server and check if they exist in the file mappings
    while (true)
    {
        char buffer[MINI_CHUNGUS] = {0};
        ssize_t bytes_received = recv(storage_sock, buffer, MINI_CHUNGUS, 0);

        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
            {
                printf("Storage server disconnected\n");
                break;
            }
            perror("recv");
            continue;
        }

        char *saveptr;
        char *token = strtok_r(buffer, "|", &saveptr);

        // Check if it's a storage server message
        if (!token || strcasecmp(token, "S") != 0)
        {
            printf("Invalid message format\n");
            continue;
        }

        // Get message type
        token = strtok_r(NULL, "|", &saveptr);
        if (!token)
        {
            printf("Missing message type\n");
            continue;
        }

        // Handle file/directory path message
        if (strcasecmp(token, "P") == 0)
        {
            // Get server ID
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing server ID\n");
                continue;
            }

            // Get file type (F/D)
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing file type\n");
                continue;
            }
            char type = token[0];

            // Get file path
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing file path\n");
                continue;
            }

            // Check if the file exists in the file mappings, and if so, it exists in the same cluster
            Inode *inode = getInode(token);
            if (inode != NULL && inode->clusterID == clusterID)
                set_hash_map(token, &exists, filesOnSS);
            else
            {
                char *temp = strdup(token);
                char *path = malloc(strlen(temp) + 2);
                strcpy(path, "/");
                strcat(path, temp);
                free(temp);
                StoSerInit *init = malloc(sizeof(StoSerInit));
                init->path = path;
                init->isFile = type == 'F' ? true : false;
                printf("Adding to extra: %s\n", path);
                append_linked_list(init, extra);
            }

            // Send acknowledgment
            char ack[32];
            snprintf(ack, sizeof(ack), "N|ACK");
            send(storage_sock, ack, strlen(ack), 0);
        }
        // Handle list completion message
        else if (strcasecmp(token, "LIST_COMPLETE") == 0)
        {
            printf("Storage server completed file list successfully\n");
            break;
        }
        // Handle list failure message
        else if (strcasecmp(token, "LIST_NOTCOMPLETE") == 0)
        {
            printf("Storage server failed to complete file list\n");
            break;
        }
        else
        {
            printf("Unknown message type: %s\n", token);
            continue;
        }
    }

    // Check if any files are missing in the storage server, by going through the same process but with the other storage server in the cluster and comparing the filesOnSS hashmap
    // first, create a new socket for the other storage server

    // Get the IP address and port number of the other storage server
    StorageServer *otherSS = getStorageServer(anotherSSid);
    if (otherSS == NULL)
    {
        log_NM_error(21);
        close(storage_sock);
        return NULL;
    }

    int otherSS_sock = connect_to_server(otherSS->ip, otherSS->port, CONNECTION_TIMEOUT);

    // Request the other storage server to send the list of files
    send(otherSS_sock, request, strlen(request), 0);

    printf("Requesting file list from storage server: %d\n", anotherSSid);

    // Receive the list of files from the other storage server and check if they exist in the file mappings
    while (true)
    {
        char buffer[MINI_CHUNGUS] = {0};
        ssize_t bytes_received = recv(otherSS_sock, buffer, MINI_CHUNGUS, 0);

        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
            {
                printf("Storage server disconnected\n");
                break;
            }
            perror("recv");
            continue;
        }

        char *saveptr;
        char *token = strtok_r(buffer, "|", &saveptr);

        // Check if it's a storage server message
        if (!token || strcasecmp(token, "S") != 0)
        {
            printf("Invalid message format\n");
            continue;
        }

        // Get message type
        token = strtok_r(NULL, "|", &saveptr);
        if (!token)
        {
            printf("Missing message type\n");
            continue;
        }

        // Handle file/directory path message
        if (strcasecmp(token, "P") == 0)
        {
            // Get server ID
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing server ID\n");
                continue;
            }

            // Get file type (F/D)
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing file type\n");
                continue;
            }
            char type = token[0];

            // Get file path
            token = strtok_r(NULL, "|", &saveptr);
            if (!token)
            {
                printf("Missing file path\n");
                continue;
            }

            if (get_hash_map(token, filesOnSS) == NULL)
            {
                char *temp = strdup(token);
                char *path = malloc(strlen(temp) + 2);
                strcpy(path, "/");
                strcat(path, temp);
                free(temp);
                StoSerInit *init = malloc(sizeof(StoSerInit));
                init->path = path;
                init->isFile = type == 'F' ? true : false;
                append_linked_list(init, missing);
            }

            // Send acknowledgment
            char ack[32];
            snprintf(ack, sizeof(ack), "N|ACK");
            send(otherSS_sock, ack, strlen(ack), 0);
        }

        // Handle list completion message
        else if (strcasecmp(token, "LIST_COMPLETE") == 0)
        {
            printf("Storage server completed file list successfully\n");
            break;
        }
        // Handle list failure message
        else if (strcasecmp(token, "LIST_NOTCOMPLETE") == 0)
        {
            printf("Storage server failed to complete file list\n");
            break;
        }
        else
        {
            printf("Unknown message type: %s\n", token);
            continue;
        }
    }

    close(storage_sock);
    close(otherSS_sock);
    destroy_hash_map(filesOnSS);

    // go through the two linked lists and send the missing files to the storage server using copySpecificSStoSpecificSS
    StorageServer *currentSS = getStorageServer(SSid);
    LinkedListNode *current = missing->head;
    printf("Missing files:\n");
    while (current)
    {
        char *path = ((StoSerInit *)current->data)->path;
        printf("%s\n", path);
        bool isFile = ((StoSerInit *)current->data)->isFile;
        copySpecificSStoSpecificSS(otherSS, currentSS, path, isFile);
        free(path);
        free(current->data);
        current = current->next;
    }

    // check if even another storage server exists in the same cluster
    int anotherSSid2 = -1;
    for (int i = 0; i < 3; i++)
    {
        StorageServer *anotherSS = getStorageServerFromCluster(i, clusterID);
        if (anotherSS && anotherSS->id != SSid && anotherSS->id != anotherSSid)
        {
            anotherSSid2 = anotherSS->id;
            break;
        }
    }

    StorageServer *otherSS2 = anotherSSid2 == -1 ? NULL : getStorageServer(anotherSSid2);

    current = extra->head;
    printf("Extra files:\n");
    while (current)
    {
        char *path = ((StoSerInit *)current->data)->path;
        printf("%s\n", path);
        bool isFile = ((StoSerInit *)current->data)->isFile;
        copySpecificSStoSpecificSS(currentSS, otherSS, path, isFile);
        if (otherSS2)
            copySpecificSStoSpecificSS(currentSS, otherSS2, path, isFile);
        justAddFileToMappings(path, !isFile, clusterID);
        free(path);
        free(current->data);
        current = current->next;
    }

    destroy_linked_list(missing);
    destroy_linked_list(extra);

    return NULL;
}

int main()
{
    // Initialize the super cluster
    initSuperClusterList();

    // Initialize the file mappings
    initFileMappings();

    // Get the IP address and port number of the naming server
    int serverFD = setupServer();
    printf("\033[1;32m Server started on IP Address: %s and Port: %d\033[0m\n", NS_IP, NS_PORT);
    printf("ServerFD: %d\n", serverFD);

    // Accept incoming connections
    while (true)
    {
        struct sockaddr_in tmp_addr;
        socklen_t addr_len = sizeof(tmp_addr);
        int tmp_sock;

        // Accept incoming connection
        if ((tmp_sock = accept(serverFD, (struct sockaddr *)&tmp_addr, &addr_len)) < 0)
        {
            log_NM_error(4);
            continue;
        }

        // Receive the first message to determine the type of request
        sleep(1);
        char req_type;
        if (recv(tmp_sock, &req_type, 1, 0) <= 0)
        {
            log_NM_error(16);
            close(tmp_sock);
            continue;
        }

        // Handle connection request
        printf("\033[1;32m New connection request from %s:%d\033[0m\n",
               inet_ntoa(tmp_addr.sin_addr), ntohs(tmp_addr.sin_port));

        pthread_t thread_id;
        int *tmp_sock_ptr = malloc(sizeof(int));
        *tmp_sock_ptr = tmp_sock;

        // Assign the appropriate thread based on the request type
        void *(*thread_func)(void *);
        if (req_type == 'C')
            thread_func = handleClient;
        else if (req_type == 'S')
            thread_func = initializeStorageServer;
        else
        {
            log_NM_error(17);
            close(tmp_sock);
            free(tmp_sock_ptr);
            continue;
        }

        // Send response to the request
        if (write(tmp_sock, "Acknowledged", strlen("Acknowledged")) < 0)
        {
            log_NM_error(18);
            break;
        }

        // Thread creation failed
        if (pthread_create(&thread_id, NULL, thread_func, tmp_sock_ptr) != 0)
        {
            log_NM_error(5);
            close(tmp_sock);
            free(tmp_sock_ptr);
        }
        // Thread creation successful
        else
        {
            printf("New thread created successfully for the request: %d\n", tmp_sock);
            // Detach the thread so that resources are released when it finishes
            pthread_detach(thread_id);
        }
    }

    // Close the server socket
    close(serverFD);
    return 0;
}
