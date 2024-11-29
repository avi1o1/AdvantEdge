#include "storage.h"

int getClusterID(int storageServerID)
{
    // Calculate cluster ID based on storage server ID ranges
    if (storageServerID >= 6)
        // For IDs 6 and above, divide by 3 to get cluster ID
        return storageServerID / 3;
    else if (storageServerID >= 2)
        // For IDs 2-5, use modulo 2 to get cluster ID
        return storageServerID % 2;
    else
        // For IDs 0-1, use ID directly as cluster ID
        return storageServerID;
}

typedef struct
{
    int client_sockfd;
    int fd;
} client_data_t;

typedef struct
{
    int naming_serverfd;
    int priority;
    char *path;
} write_data_t;

PathInfo paths[MAX_PATHS];
int num_paths = 0;

int nm_sockfd, dma_sockfd;
int temp_naming_serverfd;
static int server_id = -1;
char ip[16];
int dma_port;

int NS_PORT;    // Naming server port number
char NS_IP[16]; // Naming server IP address

int ssID = -1;
int clusterID = -1;

// Function to set the socket to blocking mode
int set_socket_blocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        log_SS_error(19);
        return -1;
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        log_SS_error(19);
        return -1;
    }

    return 0;
}

// Callback function for processing each file
int send_file_paths(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    // Skip "." and ".." directories
    const char *name = strrchr(fpath, '/');
    if (name == NULL)
        return 0;
    if (name)
    {
        name++; // Skip the '/'
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(name, "most_wanted/") == 0 || (clusterID && strcmp(name, "most_wanted/Kalimba.mp3") == 0))
            return 0;
    }

    // Process both files and directories
    if (typeflag == FTW_F || typeflag == FTW_D)
    {
        char buffer[MINI_CHUNGUS] = {0};
        char type = (typeflag == FTW_F) ? 'F' : 'D';

        // Notify Naming Server about the file/directory
        // Format: S|P|server_id|type|path
        snprintf(buffer, sizeof(buffer), "S|P|%d|%c|%s",
                 server_id,
                 type,
                 fpath + 12);

        send(temp_naming_serverfd, buffer, strlen(buffer), 0);

        // Wait for acknowledgment
        memset(buffer, 0, sizeof(buffer));
        recv(temp_naming_serverfd, buffer, MINI_CHUNGUS, 0);

        char *saveptr;
        char *token = strtok_r(buffer, "|", &saveptr);
        if (strcasecmp(token, "N") != 0)
        {
            fprintf(stderr, "%ssend_file_paths: Unexpected packet type: %s%s\n",
                    ERROR_COLOR, token, COLOR_RESET);
            return -1;
        }

        token = strtok_r(NULL, "|", &saveptr);
        if (strcasecmp(token, "ACK") != 0)
        {
            fprintf(stderr, "%ssend_file_paths: ACK not received%s\n",
                    ERROR_COLOR, COLOR_RESET);
            return -1;
        }
    }

    return 0; // Continue traversal
}

int server_init(int nm_sockfd, int dma_sockfd, int init_sock)
{
    char buffer[MINI_CHUNGUS] = {0};

    getActualIP(ip);

    // Read existing ID from metadata file if available
    FILE *fp = fopen(".metadata", "r");
    if (fp == NULL)
    {
        printf("%sMetadata file not found. Requesting new ID...%s\n", INFO_COLOR, COLOR_RESET);
        server_id = -1;
    }
    else
    {
        // Read ID from metadata file
        char id_str[6] = {0};
        if (fscanf(fp, "%5s", id_str) != 1)
        {
            printf("%sError reading ID from metadata file%s\n", ERROR_COLOR, COLOR_RESET);
            server_id = -1;
        }
        else
            server_id = atoi(id_str);
        fclose(fp);
    }

    // Complete the vitals message
    char temp[MINI_CHUNGUS];
    int nm_port = get_local_port(nm_sockfd);
    int dma_port = get_local_port(dma_sockfd);

    // Send vitals to naming server
    snprintf(temp, sizeof(temp), "S|%d|%s|%d|%d", server_id, ip, nm_port, dma_port);
    strncat(buffer, temp, MINI_CHUNGUS - strlen(buffer) - 1);
    if (send(init_sock, buffer, strlen(buffer), 0) == -1)
    {
        log_SS_error(32);
        return -1;
    }

    // Receive the server ID from the naming server [N|S|server_id|]
    memset(buffer, 0, sizeof(buffer));
    if (recv(init_sock, buffer, MINI_CHUNGUS, MSG_WAITALL) == -1)
    {
        log_SS_error(6);
        return -1;
    }
    if (strcasecmp(buffer, "Acknowledged") == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recv(init_sock, buffer, MINI_CHUNGUS, MSG_WAITALL) == -1)
        {
            log_SS_error(6);
            return -1;
        }
    }
    printf("Received from naming server: %s\n", buffer);
    char *saveptr = NULL;
    char *token = strtok_r(buffer, "|", &saveptr);
    if (token == NULL || strcasecmp(token, "N") != 0)
    {
        log_SS_error(17);
        printf("Received: %s\n", buffer);
        return -1;
    }

    token = strtok_r(NULL, "|", &saveptr);
    if (token == NULL || strcasecmp(token, "S") != 0)
    {
        log_SS_error(17);
        return -1;
    }
    token = strtok_r(NULL, "|", &saveptr);
    if (token == NULL || (server_id != -1 && server_id != atoi(token)) || (server_id == -1 && atoi(token) == -1))
    {
        log_SS_error(20);
        return -1;
    }

    server_id = atoi(token);

    // Ensure the "most_wanted" directory exists
    struct stat st;
    if (stat("most_wanted", &st) == -1)
    {
        if (mkdir("most_wanted", 0700) == -1)
        {
            log_SS_error(33);
            return -1;
        }
    }

    // Write the server ID to metadata file
    fp = fopen(".metadata", "w");
    if (fp == NULL)
    {
        log_SS_error(20);
        return -1;
    }
    fprintf(fp, "%d\n", server_id);
    fclose(fp);

    ssID = server_id;
    clusterID = getClusterID(ssID);

    return 0;
}

void *send_to_client(void *arg)
{
    client_data_t *data = (client_data_t *)arg; // Cast to the correct type
    int client_sockfd = data->client_sockfd;
    int fd = data->fd;

    free(data); // Free the memory allocated for the structure

    char buffer[MINI_CHUNGUS];
    int total_bytes_read = 0;

    printf("Sending data to client...\n");
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0)
        {
            break; // No more data to send
        }
        send(client_sockfd, buffer, bytes_read, 0);
        total_bytes_read += bytes_read;
        usleep(1000); // Small delay to avoid flooding the socket
    }
    printf("Total bytes read: %d\n\n", total_bytes_read);

    // Notify the client that the transfer is complete
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "STOP");
    sleep(1); // Wait for the client to receive the data
    send(client_sockfd, buffer, strlen(buffer), 0);

    close(client_sockfd);
    close(fd);
    pthread_exit(NULL);
}

void *receive_from_ns(void *arg)
{
    write_data_t *data = (write_data_t *)arg;
    int naming_serverfd = data->naming_serverfd;
    int priority = data->priority;
    char *path = data->path;

    free(data);

    // Open the file
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        log_SS_error(33);
        send(naming_serverfd, "S|WRITE_NOTCOMPLETE", strlen("S|WRITE_NOTCOMPLETE"), 0);
        return NULL;
    }

    printf("\nFile opened successfully.Sending accept...\n");

    if (!priority)
    {
        // Acknowledge the naming server
        send(naming_serverfd, "S|WRITE_ACCEPTED", strlen("S|WRITE_ACCEPTED"), 0);
    }

    // Write the data to the file
    char buffer[MINI_CHUNGUS];
    int complete = 0;

    printf("Receiving data from naming server...\n");
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = recv(naming_serverfd, buffer, sizeof(buffer), 0);

        if (strcasecmp(buffer, "STOP") == 0)
        {
            printf("Received STOP from naming server\nWRITE completed successfully.\n\n");
            complete = 1;
            break; // No more data to receive
        }

        if (bytes_read == 0)
        {
            fprintf(stderr, "\nreceive_from_ns: WRITE ended with no STOP\n");
            log_SS_error(24);
            break;
        }

        printf("\nWrite attempt:\n");
        printf("fd:%d buffer:%s bytes_read:%ld\n\n", fd, buffer, bytes_read);
        if (write(fd, buffer, bytes_read) == -1)
        {
            log_SS_error(18);
            break;
        }

        if (priority)
            fdatasync(fd);
    }

    // Notify the naming server that the write is complete
    if (complete)
        send(naming_serverfd, "S|WRITE_COMPLETE", strlen("S|WRITE_COMPLETE"), 0);
    else
        send(naming_serverfd, "S|WRITE_NOTCOMPLETE", strlen("S|WRITE_NOTCOMPLETE"), 0);

    // Close the file
    close(fd);
    close(naming_serverfd);

    pthread_exit(NULL);
}

int delete_files_and_directories(const char *fpath, const struct stat *sb, int typeflag)
{
    if (typeflag == FTW_F)
    {
        // Handle files
        if (remove(fpath) == -1)
        {
            log_SS_error(35);
            return -1;
        }
    }
    else if (typeflag == FTW_D)
    {
        // Recursively traverse subdirectories
        char subpath[MAX_PATH_LENGTH];
        struct stat st;

        // Open the directory
        DIR *dir = opendir(fpath);
        if (!dir)
        {
            log_SS_error(34);
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)))
        {
            // Skip special entries "." and ".."
            if (strcasecmp(entry->d_name, ".") == 0 || strcasecmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // Check if the combined length exceeds the buffer size
            if (strlen(fpath) + strlen(entry->d_name) + 1 < MAX_PATH_LENGTH)
            {
                strncpy(subpath, fpath, sizeof(subpath) - 1);
                strncat(subpath, "/", sizeof(subpath) - strlen(subpath) - 1);
                strncat(subpath, entry->d_name, sizeof(subpath) - strlen(subpath) - 1);
            }
            else
            {
                fprintf(stderr, "Error: Path length exceeds buffer size\n");
                subpath[0] = '\0';
            }

            // Check if it's a file or directory
            if (stat(subpath, &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    // Recursive call for subdirectory
                    if (ftw(subpath, delete_files_and_directories, 10) == -1)
                    {
                        log_SS_error(36);
                        closedir(dir);
                        return -1;
                    }
                }
                else
                {
                    // Remove file
                    if (remove(subpath) == -1)
                    {
                        log_SS_error(35);
                        closedir(dir);
                        return -1;
                    }
                }
            }
        }

        closedir(dir);

        // Now remove the directory
        if (rmdir(fpath) == -1)
        {
            log_SS_error(35);
            return -1;
        }
    }

    return 0; // Continue traversal
}

int copy_file(const char *source, const char *destination)
{
    int src_fd, dest_fd;
    ssize_t bytesRead, bytesWritten;
    char buffer[MINI_CHUNGUS];

    // Open source file
    src_fd = open(source, O_RDONLY);
    if (src_fd < 0)
    {
        log_SS_error(34);
        return -1;
    }

    // Open destination file
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0)
    {
        log_SS_error(34);
        close(src_fd);
        return -1;
    }

    // Copy file contents
    while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t totalWritten = 0;
        while (totalWritten < bytesRead)
        {
            bytesWritten = write(dest_fd, buffer + totalWritten, bytesRead - totalWritten);
            if (bytesWritten < 0)
            {
                log_SS_error(18);
                close(src_fd);
                close(dest_fd);
                return -1;
            }
            totalWritten += bytesWritten;
        }
    }

    if (bytesRead < 0)
    {
        log_SS_error(37);
        close(src_fd);
        close(dest_fd);
        return -1;
    }

    // Close files
    close(src_fd);
    close(dest_fd);

    return 0;
}

void *process_naming_server_requests(void *arg)
{
    int nm_sockfd = *(int *)arg;
    free(arg);
    printf("Thread created for naming server requests\n");

    char buffer[MINI_CHUNGUS] = {0};
    while (1)
    {
        // Accept naming server connection
        int naming_serverfd = accept(nm_sockfd, NULL, NULL);
        if (naming_serverfd < 0)
            continue; // If accept fails, continue to the next iteration

        // Get request from naming server
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(naming_serverfd, buffer, sizeof(buffer) - 1);
        // Clear the buffer
        if (bytes_read < 0)
        {
            // TODO: Temporary fix for now, as it works in NS-CL communication
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                // No data available, continue the loop
                continue;
            }
            else
            {
                log_SS_error(7);
                break;
            }
        }
        else if (bytes_read == 0)
        {
            log_SS_error(21);
            break;
        }

        // Log the request
        buffer[bytes_read] = '\0';
        printf("Received from naming server: %s\n", buffer);
        fflush(stdout);

        // Processing the request. Format: <N|{cmnd}|>
        char *saveptr;
        char *token = strtok_r(buffer, "|", &saveptr);
        if (strcasecmp(token, "N") == 0)
        {
            token = strtok_r(NULL, "|", &saveptr);
            if (strcasecmp(token, "HEYYY") == 0)
                send(naming_serverfd, "S|HEYYY", strlen("S|HEYYY"), 0);

            else if (strcasecmp(token, "WRITE") == 0)
            {
                write_data_t *data = malloc(sizeof(write_data_t));
                data->naming_serverfd = naming_serverfd;
                data->priority = 0;

                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH];
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);
                data->path = strdup(real_path);

                pthread_t thread_id;
                pthread_create(&thread_id, NULL, receive_from_ns, (void *)data);
                pthread_detach(thread_id);
            }

            else if (strcasecmp(token, "PRIORITYWRITE") == 0)
            {
                write_data_t *data = malloc(sizeof(write_data_t));
                data->naming_serverfd = naming_serverfd;
                data->priority = 1;

                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH];
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);
                data->path = strdup(real_path);

                pthread_t thread_id;
                pthread_create(&thread_id, NULL, receive_from_ns, (void *)data);
                pthread_join(thread_id, NULL);
            }

            else if (strcasecmp(token, "CREATE") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH] = {0};
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                if (creat(real_path, 0644) < 0)
                {
                    snprintf(buffer, sizeof(buffer), "S|C_NOTCOMPLETE|%s", strerror(errno));
                    log_SS_error(38);
                    send(naming_serverfd, buffer, strlen(buffer), 0);
                }
                else
                    send(naming_serverfd, "S|C_COMPLETE", strlen("S|C_COMPLETE"), 0);

                close(naming_serverfd);
            }

            else if (strcasecmp(token, "CREATE_DIRECTORY") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH] = {0};
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                if (mkdir(real_path, 0700) < 0)
                {
                    snprintf(buffer, sizeof(buffer), "S|CD_NOTCOMPLETE|%s", strerror(errno));
                    log_SS_error(33);
                    send(naming_serverfd, buffer, strlen(buffer), 0);
                }
                else
                    send(naming_serverfd, "S|CD_COMPLETE", strlen("S|CD_COMPLETE"), 0);

                close(naming_serverfd);
            }

            else if (strcasecmp(token, "DELETE") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH] = {0};
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                if (remove(real_path) < 0)
                {
                    snprintf(buffer, sizeof(buffer), "S|D_NOTCOMPLETE|%s", strerror(errno));
                    log_SS_error(35);
                    send(naming_serverfd, buffer, strlen(buffer), 0);
                }
                else
                    send(naming_serverfd, "S|D_COMPLETE", strlen("S|D_COMPLETE"), 0);

                close(naming_serverfd);
            }

            else if (strcasecmp(token, "DELETE_DIRECTORY") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH] = {0}; // Initialize the path buffer
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                if (ftw(real_path, delete_files_and_directories, 20) == -1)
                {
                    log_SS_error(36);
                    send(naming_serverfd, "S|DD_NOTCOMPLETE", strlen("S|DD_NOTCOMPLETE"), 0);
                }

                send(naming_serverfd, "S|DD_COMPLETE", strlen("S|DD_COMPLETE"), 0);
                close(naming_serverfd);
            }

            else if (strcasecmp(token, "RENAME") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char old_path[MAX_PATH_LENGTH] = {0};
                snprintf(old_path, sizeof(old_path), "most_wanted%s", token);
                token = strtok_r(NULL, "|", &saveptr);
                char new_path[MAX_PATH_LENGTH] = {0};
                snprintf(new_path, sizeof(new_path), "most_wanted%s", token);
                if (rename(old_path, new_path) < 0)
                {
                    snprintf(buffer, sizeof(buffer), "S|RENAME_NOTCOMPLETE|%s", strerror(errno));
                    log_SS_error(39);
                    send(naming_serverfd, buffer, strlen(buffer), 0);
                }
                else
                    send(naming_serverfd, "S|RENAME_COMPLETE", strlen("S|RENAME_COMPLETE"), 0);
                close(naming_serverfd);
            }

            else if (strcasecmp(token, "COPY") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char src_path[MAX_PATH_LENGTH] = {0};
                snprintf(src_path, sizeof(src_path), "most_wanted%s", token);
                token = strtok_r(NULL, "|", &saveptr);
                char dst_path[MAX_PATH_LENGTH] = {0};
                snprintf(dst_path, sizeof(dst_path), "most_wanted%s", token);
                if (copy_file(src_path, dst_path) == 0)
                    send(naming_serverfd, "S|COPY_COMPLETE", strlen("S|COPY_COMPLETE"), 0);
                else
                {
                    snprintf(buffer, sizeof(buffer), "S|COPY_NOTCOMPLETE|%s", strerror(errno));
                    log_SS_error(40);
                    send(naming_serverfd, buffer, strlen(buffer), 0);
                }
                close(naming_serverfd);
            }

            else if (strcasecmp(token, "MOVE") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char src_path[MAX_PATH_LENGTH] = {0};
                snprintf(src_path, sizeof(src_path), "most_wanted%s", token);
                token = strtok_r(NULL, "|", &saveptr);
                char dst_path[MAX_PATH_LENGTH] = {0};
                snprintf(dst_path, sizeof(dst_path), "most_wanted%s", token);
                if (copy_file(src_path, dst_path) == 0)
                {
                    if (remove(src_path) < 0)
                    {
                        snprintf(buffer, sizeof(buffer), "S|MOVE_NOTCOMPLETE|%s", strerror(errno));
                        log_SS_error(35);
                        send(naming_serverfd, buffer, strlen(buffer), 0);
                    }
                    else
                        send(naming_serverfd, "S|MOVE_COMPLETE", strlen("S|MOVE_COMPLETE"), 0);
                }
                else
                {
                    send(naming_serverfd, "S|MOVE_NOTCOMPLETE", strlen("S|MOVE_NOTCOMPLETE"), 0);
                }
                close(naming_serverfd);
            }
        }
        else if (strcasecmp(token, "S") == 0)
        {
            // get the next token
            token = strtok_r(NULL, "|", &saveptr);
            if (strcasecmp(token, "LIST") == 0)
            {
                temp_naming_serverfd = naming_serverfd;
                // List all the files and directories
                if (nftw("most_wanted", send_file_paths, MAX_OPEN_DIRS, FTW_PHYS) == -1)
                {
                    log_SS_error(41);
                    send(naming_serverfd, "S|LIST_NOTCOMPLETE", strlen("S|LIST_NOTCOMPLETE"), 0);
                }
                else
                {
                    printf("Sent to naming server: S|LIST_COMPLETE\n");
                    send(naming_serverfd, "S|LIST_COMPLETE", strlen("S|LIST_COMPLETE"), 0);
                }
                close(naming_serverfd);
            }
            else if (strcasecmp(token, "READ") == 0)
            {
                token = strtok_r(NULL, "|", &saveptr);
                char real_path[MAX_PATH_LENGTH];
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                int fd = open(real_path, O_RDONLY);
                if (fd == -1)
                {
                    printf("File not found\n");
                    send(naming_serverfd, "S|READ_NOTCOMPLETE", strlen("S|READ_NOTCOMPLETE"), 0);
                    continue;
                }

                // Acknowledge the naming server
                send(naming_serverfd, "S|READ_ACCEPTED", strlen("S|READ_ACCEPTED"), 0);

                pthread_t thread_id;

                client_data_t *data = malloc(sizeof(client_data_t));
                data->client_sockfd = naming_serverfd;
                data->fd = fd;

                pthread_create(&thread_id, NULL, send_to_client, (void *)data);
                pthread_detach(thread_id);
            }
            else
            {
                log_SS_error(8);
                break;
            }
        }
        else
        {
            log_SS_error(8);
            break;
        }
        usleep(1000);
    }
    return NULL;
}

void *process_client_requests(void *arg)
{
    int dma_sockfd = *(int *)arg;
    free(arg);
    printf("Thread created for client requests\n");

    char buffer[MINI_CHUNGUS] = {0};
    while (1)
    {
        // Accept client connection
        int client_sockfd = accept(dma_sockfd, NULL, NULL);
        if (client_sockfd < 0)
            continue; // If accept fails, continue to the next iteration

        // Get request from client
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_sockfd, buffer, sizeof(buffer) - 1);

        // Clear the buffer
        if (bytes_read < 0)
        {
            // TODO: Temporary fix for now, as it works in NS-CL communication
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                // No data available, continue the loop
                continue;
            }
            else
            {
                log_SS_error(7);
                close(client_sockfd);
                break;
            }
        }
        else if (bytes_read == 0)
        {
            log_SS_error(21);
            close(client_sockfd);
            break;
        }

        // Get the request from the client
        buffer[bytes_read] = '\0';
        printf("\nReceived from client: %s\n", buffer);
        fflush(stdout);

        // Open Path
        char *ret_ptr;
        char *token = strtok_r(buffer, "|", &ret_ptr);
        int fd;
        if (strcasecmp(token, "C") == 0)
        {
            printf("C Matched\n");
            token = strtok_r(NULL, "|", &ret_ptr);
            if (strcasecmp(token, "READ") == 0 || strcasecmp(token, "STREAM") == 0)
            {
                printf("READ/STREAM Matched\n");
                // Open Path
                token = strtok_r(NULL, "|", &ret_ptr);
                token = strtok_r(NULL, "|", &ret_ptr);

                char real_path[MAX_PATH_LENGTH];
                snprintf(real_path, sizeof(real_path), "most_wanted%s", token);

                printf("Opening file: %s\n", real_path);
                fd = open(real_path, O_RDONLY);
                if (fd == -1)
                {
                    printf("File not found\n");
                    close(client_sockfd);
                    send(client_sockfd, "S|READ_NOTCOMPLETE", strlen("S|READ_NOTCOMPLETE"), 0);
                    continue;
                }
                printf("File opened successfully. Sending ACK to Client\n");
                // Acknowledge the client request
                send(client_sockfd, "S|ACK", strlen("S|ACK"), 0);
            }
            else
            {
                close(client_sockfd);
                continue;
            }
        }
        else
        {
            close(client_sockfd);
            continue;
        }

        pthread_t thread_id;

        client_data_t *data = malloc(sizeof(client_data_t));
        data->client_sockfd = client_sockfd;
        data->fd = fd;

        printf("Creating thread to send data to client...\n");
        pthread_create(&thread_id, NULL, send_to_client, (void *)data);
        pthread_detach(thread_id);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("%sUsage: %s <server_ip> <port>%s\n", ERROR_COLOR, argv[0], COLOR_RESET);
        return -1;
    }

    // Save the naming server IP and port
    strncpy(NS_IP, argv[1], 15);
    NS_IP[15] = '\0';
    NS_PORT = atoi(argv[2]);

    // Setting up the socket for the naming server to connect
    nm_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_sockfd == -1)
        return -1;

    struct sockaddr_in nm_server;
    nm_server.sin_family = AF_INET;
    nm_server.sin_port = htons(0);
    nm_server.sin_addr.s_addr = inet_addr(NS_IP);
    bind(nm_sockfd, (struct sockaddr *)&nm_server, sizeof(nm_server));
    listen(nm_sockfd, SOMAXCONN);

    // Connect to the naming server
    int init_sock = connect_to_server(NS_IP, NS_PORT, CONNECTION_TIMEOUT);

    // Tell the naming server that this is a storage server (and confirm acknowledgement)
    send(init_sock, "S", 1, 0);
    char buffer[MINI_CHUNGUS] = {0};
    memset(buffer, 0, sizeof(buffer));
    recv(init_sock, buffer, MINI_CHUNGUS, MSG_WAITALL);
    printf("Server ack: %s\n", buffer);

    // Setting up DMA socket for the clients
    dma_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in dma_server;
    dma_server.sin_family = AF_INET;
    dma_server.sin_port = htons(0);
    dma_server.sin_addr.s_addr = INADDR_ANY;
    bind(dma_sockfd, (struct sockaddr *)&dma_server, sizeof(dma_server));
    listen(dma_sockfd, SOMAXCONN);

    dma_port = ntohs(dma_server.sin_port);

    printf("%sStorage server connected successfully to Naming Server at IP Address: %s and Port: %d%s\n",
           SUCCESS_COLOR, argv[1], atoi(argv[2]), COLOR_RESET);

    int ret = server_init(nm_sockfd, dma_sockfd, init_sock);

    if (ret == -1)
    {
        printf("%sError initializing server%s\n", ERROR_COLOR, COLOR_RESET);
        return -1;
    }

    close(init_sock);
    printf("%sStorage server initialized with ID: %d%s\n", INFO_COLOR, server_id, COLOR_RESET);

    // Thread to process requests from the naming server
    int *nm_sock_ptr = malloc(sizeof(int));
    *nm_sock_ptr = nm_sockfd;
    pthread_t nm_thread;

    // Thread creation failed
    if (pthread_create(&nm_thread, NULL, process_naming_server_requests, nm_sock_ptr) != 0)
    {
        log_SS_error(5);
        close(nm_sockfd);
        free(nm_sock_ptr);
    }
    // Thread creation successful
    else
    {
        printf("New thread created successfully for the request: %d\n", nm_sockfd);
        // Detach the thread so that resources are released when it finishes
        // pthread_detach(nm_thread);
    }

    // Thread to process requests from the client
    int *dma_sock_ptr = malloc(sizeof(int));
    *dma_sock_ptr = dma_sockfd;
    pthread_t dma_thread;

    // Thread creation failed (TODO: initialise process_client_requests function)
    if (pthread_create(&dma_thread, NULL, process_client_requests, dma_sock_ptr) != 0)
    {
        printf("\033[1;31m Oopsie Woopsie : Thread Creation Failed!\033[0m\n");
        close(dma_sockfd);
        free(dma_sock_ptr);
    }
    // Thread creation successful
    else
    {
        printf("New thread created successfully for the request: %d\n", dma_sockfd);
        // Detach the thread so that resources are released when it finishes
        // pthread_detach(dma_thread);
    }

    pthread_join(nm_thread, NULL);
    pthread_join(dma_thread, NULL);

    return 0;
}
