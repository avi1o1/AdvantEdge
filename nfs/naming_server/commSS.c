#include "commSS.h"

int createFileSS(Inode *inode)
{
    for (int i = 0; i < 3; i++)
    {
        StorageServer *storageServer = getStorageServerFromCluster(i, inode->clusterID);
        if (storageServer == NULL || !storageServer->isAlive)
            continue;

        int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);
        if (sock < 0)
        {
            printf("Failed to connect to storage server %d\n", storageServer->id);
            return -1;
        }

        char buffer[MINI_CHUNGUS] = {0};
        if (inode->isDir)
            sprintf(buffer, "N|CREATE_DIRECTORY|%s", inode->userPath);
        else
            sprintf(buffer, "N|CREATE|%s", inode->userPath);

        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, MINI_CHUNGUS);
        recv(sock, buffer, MINI_CHUNGUS, 0);
        close(sock);

        if (strcasecmp(buffer, "S|C_COMPLETE") && strcasecmp(buffer, "S|CD_COMPLETE"))
        {
            printf("returned: %s\n", buffer);
            return -1;
        }
    }

    printf("success\n");
    return 0;
}

char *readFileSS(Inode *inode)
{
    StorageServer *storageServer = getAliveStorageServerFromCluster(inode->clusterID);
    if (storageServer == NULL)
        return NULL;

    int sock = connect_to_server(storageServer->ip, storageServer->DMAport, CONNECTION_TIMEOUT);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "C|READ|1|%s", inode->userPath);
    send(sock, buffer, strlen(buffer), 0);

    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|ACK"))
    {
        close(sock);
        return NULL;
    }

    char *data = (char *)malloc(BIG_CHUNGUS);
    int bytesRead = 0;
    while (true)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        int bytes = recv(sock, buffer, MINI_CHUNGUS, 0);
        if (bytes <= 0 || strcasecmp(buffer, "STOP") == 0)
            break;
        memcpy(data + bytesRead, buffer, bytes);
        bytesRead += bytes;
    }

    data[bytesRead] = '\0';

    close(sock);
    return data;
}

void writeFileNotAsyncSS(StorageServer *storageServer, char *path, char *data, int *success, pthread_mutex_t *lock)
{
    int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "N|PRIORITYWRITE|%s", path);
    send(sock, buffer, strlen(buffer), 0);

    // Divide data into packets of size MINI_CHUNGUS and send
    size_t data_len = strlen(data);
    size_t sent = 0;

    sleep(1);

    while (sent < data_len)
    {
        size_t chunk_size = (data_len - sent) < MINI_CHUNGUS ? (data_len - sent) : MINI_CHUNGUS;
        memcpy(buffer, data + sent, chunk_size);
        send(sock, buffer, chunk_size, 0);
        sent += chunk_size;
        sleep(1);
    }

    // Send a separate packet of STOP
    send(sock, "STOP", strlen("STOP"), 0);

    // receive S|WRITE_COMPLETE
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|WRITE_COMPLETE"))
    {
        pthread_mutex_lock(lock);
        *success = false;
        pthread_mutex_unlock(lock);
    }

    close(sock);
}

typedef struct
{
    StorageServer *storageServer;
    char *path;
    char *data;
    int *success;
    pthread_mutex_t *lock;
} ThreadArgs;

void *writeFileNotAsyncThread(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    writeFileNotAsyncSS(threadArgs->storageServer, threadArgs->path, threadArgs->data, threadArgs->success, threadArgs->lock);
    return NULL;
}

void writeFileAsyncSS(StorageServer *storageServer, char *path, char *data, int *success, pthread_mutex_t *lock)
{
    int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);

    printf("Connected to storage server %s:%d\n", storageServer->ip, storageServer->port);
    printf("sock: %d\n", sock);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "N|WRITE|%s", path);
    send(sock, buffer, strlen(buffer), 0);

    // wait for S|WRITE_ACCEPTED
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|WRITE_ACCEPTED"))
    {
        pthread_mutex_lock(lock);
        *success = -100;
        pthread_mutex_unlock(lock);
        close(sock);
        return;
    }

    printf("Write accepted\n");

    pthread_mutex_lock(lock);
    *success += 1;
    pthread_mutex_unlock(lock);

    // Divide data into packets of size MINI_CHUNGUS and send

    size_t data_len = strlen(data);
    size_t sent = 0;

    sleep(1);

    while (sent < data_len)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        size_t chunk_size = (data_len - sent) < MINI_CHUNGUS ? (data_len - sent) : MINI_CHUNGUS;
        memcpy(buffer, data + sent, chunk_size);
        send(sock, buffer, chunk_size, 0);
        printf("Sent %ld bytes\n", chunk_size);
        printf("Data: %s\n", buffer);
        sent += chunk_size;
        sleep(1);
    }

    // Send a separate packet of STOP
    printf("Sending STOP\n");
    send(sock, "STOP", strlen("STOP"), 0);
    printf("Sent STOP\n");

    // wait for S|WRITE_COMPLETE
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|WRITE_COMPLETE"))
    {
        pthread_mutex_lock(lock);
        *success = -100;
        pthread_mutex_unlock(lock);
    }

    printf("Write complete\n");

    close(sock);
}

void *writeFileAsyncThread(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    writeFileAsyncSS(threadArgs->storageServer, threadArgs->path, threadArgs->data, threadArgs->success, threadArgs->lock);
    return NULL;
}

typedef struct
{
    Inode *inode;
    char *data;
    int threadCount;
    pthread_t *threads;
    char *client_ip;
    int client_port;
    int *success;
    ThreadArgs *threadArgs; // New field to track thread arguments
} CompletionArgs;

void *completionThread(void *args)
{
    CompletionArgs *completionArgs = (CompletionArgs *)args;

    // Wait for all threads to complete
    for (int i = 0; i < completionArgs->threadCount; i++)
    {
        pthread_join(completionArgs->threads[i], NULL);

        // Free resources for each thread
        free(completionArgs->threadArgs[i].path);
        if (completionArgs->threadArgs[i].data != completionArgs->data)
            free(completionArgs->threadArgs[i].data);
    }

    // Update inode information
    pthread_mutex_lock(&completionArgs->inode->available);
    completionArgs->inode->writersCount--;
    completionArgs->inode->lastModificationTime = time(NULL);
    completionArgs->inode->lastAccessTime = time(NULL);
    completionArgs->inode->size = strlen(completionArgs->data);
    pthread_mutex_unlock(&completionArgs->inode->available);

    int client_sock = connect_to_server(completionArgs->client_ip, completionArgs->client_port, CONNECTION_TIMEOUT);
    if (client_sock < 0)
    {
        free(completionArgs);
        return NULL;
    }

    char buffer[MINI_CHUNGUS] = {0};
    if (*(completionArgs->success) < 0)
        sprintf(buffer, "Failed to write at: %s", completionArgs->inode->userPath);
    else
        sprintf(buffer, "Completed asynchronous write at: %s", completionArgs->inode->userPath);
    send(client_sock, buffer, strlen(buffer), 0);

    close(client_sock);

    // Free additional resources
    free(completionArgs->threads);
    free(completionArgs->threadArgs);
    free(completionArgs->client_ip);
    free(completionArgs);
    return NULL;
}

int writeFileSS(Inode *inode, void *data, bool async, char *ip, int port)
{
    if (data == NULL)
        return -1;
    char *client_ip = NULL;
    if (ip != NULL)
        client_ip = strdup(ip);
    int client_port = port;

    // Dynamically allocate threads and threadArgs
    pthread_t *threads = malloc(3 * sizeof(pthread_t));
    ThreadArgs *threadArgs = malloc(3 * sizeof(ThreadArgs));

    int success = !async;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    int threadCount = 0;

    sleep(1);

    for (int i = 0; i < 3; i++)
    {
        StorageServer *storageServer = getStorageServerFromCluster(i, inode->clusterID);
        if (storageServer == NULL || !storageServer->isAlive || helloSS(storageServer))
            continue;

        // Duplicate strings to ensure they persist
        threadArgs[threadCount].storageServer = storageServer;
        threadArgs[threadCount].path = strdup(inode->userPath);
        threadArgs[threadCount].data = async ? strdup(data) : data;
        threadArgs[threadCount].success = &success;
        threadArgs[threadCount].lock = &lock;

        if (async)
            pthread_create(&threads[threadCount], NULL, writeFileAsyncThread, &threadArgs[threadCount]);
        else
            pthread_create(&threads[threadCount], NULL, writeFileNotAsyncThread, &threadArgs[threadCount]);
        threadCount++;
    }

    // Wait for all threads to complete if not async
    if (!async)
    {
        for (int i = 0; i < threadCount; i++)
            pthread_join(threads[i], NULL);

        free(threads);
        free(threadArgs);

        if (!success)
            return -1;
    }
    else
    {
        while (success < threadCount)
        {
            if (success < 0)
            {
                // Cleanup
                free(threads);
                free(threadArgs);
                free(client_ip);
                return -1;
            }
            sleep(1);
        }

        CompletionArgs *completionArgs = malloc(sizeof(CompletionArgs));
        completionArgs->inode = inode;
        completionArgs->data = data;
        completionArgs->threadCount = threadCount;
        completionArgs->threads = threads; // Pass dynamically allocated threads
        completionArgs->client_ip = client_ip;
        completionArgs->client_port = client_port;
        completionArgs->success = &success;
        completionArgs->threadArgs = threadArgs; // Pass dynamically allocated threadArgs

        pthread_t completionThreadID;
        pthread_create(&completionThreadID, NULL, completionThread, completionArgs);
        pthread_detach(completionThreadID);
    }


    // Update file size
    inode->size = strlen(data);

    return 0;
}

int copyFileSS(Inode *fromInode, Inode *toInode)
{
    if (fromInode->isDir)
        return 0;
    return writeFileSS(toInode, readFileSS(fromInode), false, NULL, 0);
}

int deleteFileSS(Inode *inode)
{
    for (int i = 0; i < 3; i++)
    {
        StorageServer *storageServer = getStorageServerFromCluster(i, inode->clusterID);
        if (storageServer == NULL || !storageServer->isAlive)
            continue;

        int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);
        if (sock < 0)
            return -1;

        char buffer[MINI_CHUNGUS] = {0};
        if (inode->isDir)
            sprintf(buffer, "N|DELETE_DIRECTORY|%s", inode->userPath);
        else
            sprintf(buffer, "N|DELETE|%s", inode->userPath);

        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, MINI_CHUNGUS);
        recv(sock, buffer, MINI_CHUNGUS, 0);
        close(sock);

        if (strcasecmp(buffer, "S|D_COMPLETE") && strcasecmp(buffer, "S|DD_COMPLETE"))
            return -1;
    }
    return 0;
}

int renameFileSS(char *oldPath, Inode *newInode)
{
    for (int i = 0; i < 3; i++)
    {
        StorageServer *storageServer = getStorageServerFromCluster(i, newInode->clusterID);
        if (storageServer == NULL || !storageServer->isAlive)
            continue;

        int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);
        if (sock < 0)
            return -1;

        char buffer[MINI_CHUNGUS] = {0};
        sprintf(buffer, "N|RENAME|%s|%s", oldPath, newInode->userPath);
        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, MINI_CHUNGUS);
        recv(sock, buffer, MINI_CHUNGUS, 0);
        close(sock);

        if (strcasecmp(buffer, "S|RENAME_COMPLETE"))
            return -1;
    }

    return 0;
}

int helloSS(StorageServer *storageServer)
{
    // Connect to storage server
    int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);
    if (sock < 0)
        return -1;

    // Send hello message
    send(sock, "N|HEYYY", strlen("N|HEYYY"), 0);

    // Receive response
    char buffer[MINI_CHUNGUS] = {0};
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    close(sock);

    if (strcasecmp(buffer, "S|HEYYY") == 0)
        printf("Connected to storage server (alive) %s:%d\n", storageServer->ip, storageServer->port);
    else
    {
        printf("Failed to connect to storage server %s:%d\n", storageServer->ip, storageServer->port);
        return -1;
    }

    return 0;
}

int createFileSpecificSS(StorageServer *storageServer, char *path)
{
    int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "N|CREATE|%s", path);
    send(sock, buffer, strlen(buffer), 0);

    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    close(sock);

    if (strcasecmp(buffer, "S|C_COMPLETE"))
        return -1;

    return 0;
}

char *readFileFromSpecificSS(StorageServer *storageServer, char *path)
{
    int sock = connect_to_server(storageServer->ip, storageServer->DMAport, CONNECTION_TIMEOUT);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "C|READ|1|%s", path);
    send(sock, buffer, strlen(buffer), 0);

    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|ACK"))
    {
        close(sock);
        return NULL;
    }

    char *data = (char *)malloc(BIG_CHUNGUS);
    int bytesRead = 0;
    while (true)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        int bytes = recv(sock, buffer, MINI_CHUNGUS, 0);
        if (bytes <= 0 || strcasecmp(buffer, "STOP") == 0)
            break;
        memcpy(data + bytesRead, buffer, bytes);
        bytesRead += bytes;
    }

    data[bytesRead] = '\0';

    close(sock);
    return data;
}

int writeFileAsyncSpecificSS(StorageServer *storageServer, char *path, char *data)
{
    int sock = connect_to_server(storageServer->ip, storageServer->port, CONNECTION_TIMEOUT);

    char buffer[MINI_CHUNGUS] = {0};
    sprintf(buffer, "N|WRITE|%s", path);
    send(sock, buffer, strlen(buffer), 0);

    // wait for S|WRITE_ACCEPTED
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);
    if (strcasecmp(buffer, "S|WRITE_ACCEPTED"))
    {
        close(sock);
        return -1;
    }

    // Divide data into packets of size MINI_CHUNGUS and send

    size_t data_len = strlen(data);
    size_t sent = 0;

    sleep(1);

    while (sent < data_len)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        size_t chunk_size = (data_len - sent) < MINI_CHUNGUS ? (data_len - sent) : MINI_CHUNGUS;
        memcpy(buffer, data + sent, chunk_size);
        send(sock, buffer, chunk_size, 0);
        sent += chunk_size;
        sleep(1);
    }

    // Send a separate packet of STOP
    send(sock, "STOP", strlen("STOP"), 0);

    // wait for S|WRITE_COMPLETE
    memset(buffer, 0, MINI_CHUNGUS);
    recv(sock, buffer, MINI_CHUNGUS, 0);

    if (strcasecmp(buffer, "S|WRITE_COMPLETE"))
    {
        close(sock);
        return -1;
    }

    free(data);

    close(sock);
    return 0;
}

int copySpecificSStoSpecificSS(StorageServer *fromSS, StorageServer *toSS, char *path, bool isFile)
{
    if (!isFile)
        return createFileSpecificSS(toSS, path);
    return writeFileAsyncSpecificSS(toSS, path, readFileFromSpecificSS(fromSS, path));
}
