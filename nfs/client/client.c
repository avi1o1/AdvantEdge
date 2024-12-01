#include "client.h"

int NS_PORT;    // Naming server port number
char NS_IP[16]; // Naming server IP address
char CL_IP[16]; // Client IP address

char log_message[100420];

// Helper function to display ll available commands
void displayCommands(void)
{
    printf("\n%sAvailable commands:%s\n", INFO_COLOR, COLOR_RESET);
    printf(COLOR_CYAN);
    printf("0.\tHELP\n");
    printf("1.\tCREATE FILE <path/to/file>\n");
    printf("2.\tCREATE DIRECTORY <path/to/directory>\n");
    printf("3.\tCOPY FILE <path/to/file> <path/to/dst>\n");
    printf("4.\tCOPY DIRECTORY <path/to/directory> <path/to/dst>\n");
    printf("5.\tDELETE FILE <path/to/file>\n");
    printf("6.\tDELETE DIRECTORY <path/to/directory>\n");
    printf("7.\tRENAME FILE <path/to/file> <new_name>\n");
    printf("8.\tRENAME DIRECTORY <path/to/directory> <new_name>\n");
    printf("9.\tSTREAM FILE <path/to/audio/file>\n");
    printf("10.\tREAD FILE <path/to/file>\n");
    printf("11.\tWRITE FILE <path/to/file>\n");
    printf("12.\tPRIORITYWRITE FILE <path/to/file>\n");
    printf("13.\tLIST PATHS <path/to/serch/folder>\n");
    printf("14.\tINFO <path/to/file>\n");
    printf("15.\tGIVE GRADES\n");
    printf("16.\tEXIT\n");
    printf(COLOR_RESET);
}

// Function to stream audio data from the storage server
int streamAudioData(char *ip, int port, char *request)
{
    // Create a pipe for communication
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        log_CL_error(26);
        return -1;
    }

    // Fork the process
    pid_t pid = fork();
    if (pid < 0)
    {
        log_CL_error(25);
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0)
    {                     // Child process - will handle mpv player
        close(pipefd[1]); // Close write end

        // Redirect stdin to read end of pipe
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            log_CL_error(27);
            close(pipefd[0]);
            exit(1);
        }
        close(pipefd[0]);

        // Execute mpv to play the audio
        execlp("mpv", "mpv", "-", "--no-cache", "--no-video", "--term-osd-bar", "--no-resume-playback", NULL);

        // If exec fails
        log_CL_error(27);
        exit(1);
    }

    // Parent process - will handle receiving data
    close(pipefd[0]); // Close read end

    // Connect to the storage server
    int new_sock = connect_to_server(ip, port, CONNECTION_TIMEOUT);
    if (new_sock < 0)
    {
        log_CL_error(29);
        close(pipefd[1]);
        return -1;
    }
    log_CL("Connected to storage server for fetching STREAM data");

    // Send the streaming request
    if (send(new_sock, request, strlen(request), 0) == -1) {
        log_CL_error(30);
        close(new_sock);
        close(pipefd[1]);
        return -1;
    }
    log_CL("Resending STREAM request to storage server");

    // Get acknowledgment from storage server
    char buffer[MINI_CHUNGUS] = {0};
    if (recv(new_sock, buffer, MINI_CHUNGUS, 0) <= 0) {
        log_CL_error(16);
        close(new_sock);
        close(pipefd[1]);
        return -1;
    }
    if (strcasecmp(buffer, "S|ACK") != 0)
    {
        log_CL_error(28);
        close(new_sock);
        close(pipefd[1]);
        return -1;
    }
    log_CL("Received STREAM ACK from storage server");

    printf("%sStreaming audio... Press Ctrl+C to stop%s\n", INFO_COLOR, COLOR_RESET);

    // Receive and write data to pipe
    while (1)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        ssize_t bytes_received = recv(new_sock, buffer, MINI_CHUNGUS, 0);

        if (bytes_received <= 0)
        {
            log_CL_error(16);
            break;
        }

        // Check if we received the stop signal
        if (strcasecmp(buffer, "STOP") == 0)
        {
            log_CL("Received STOP signal from storage server");
            break;
        }

        // Write received data to pipe
        if (write(pipefd[1], buffer, bytes_received) == -1) {
            log_CL_error(18);
            break;
        }
    }

    // Clean up
    close(pipefd[1]);
    close(new_sock);

    // Wait for mpv to finish
    int status;
    waitpid(pid, &status, 0);
    log_CL("Streaming completed");

    return 0;
}


// Function to get data from the storage server (in case of READ & STREAM requests)
int getActualData(char *ip, int port, char *request)
{
    // Check if this is a streaming request and handle it separately
    char *req_copy = strdup(request);
    char *token = strtok(req_copy, "|");
    token = strtok(NULL, "|");
    if (strcasecmp(token, "STREAM") == 0)
    {
        free(req_copy);
        return streamAudioData(ip, port, request);
    }
    free(req_copy);

    // Connect to the new IP and port using a new socket
    int new_sock = connect_to_server(ip, port, CONNECTION_TIMEOUT);
    log_CL("Connected to storage server for fetching READ data");

    // Resending request to the storage server
    send(new_sock, request, strlen(request), 0);
    log_CL("Resending READ request to storage server");

    // Receive acknowledgment from the storage server
    char buffer[MINI_CHUNGUS] = {0};
    recv(new_sock, buffer, MINI_CHUNGUS, 0);
    
    sprintf(log_message, "Received acknowledgment from storage server: %s", buffer);
    log_CL(log_message);
    
    if (strcasecmp(buffer, "S|ACK") != 0)
    {
        log_CL_error(28);
        close(new_sock);
        return -1;
    }

    // Start receiving data from the storage server
    printf("%s\nResponse\n%s", PROMPT_COLOR, COLOR_RESET);
    while (1)
    {
        memset(buffer, 0, MINI_CHUNGUS);
        recv(new_sock, buffer, MINI_CHUNGUS, 0);
        if (strcasecmp(buffer, "STOP") == 0)
            break;
        printf("%s\n", buffer);
    }
    printf("\n==========================================================\n");

    log_CL("Received data from storage server");

    // Close the new socket after use
    close(new_sock);
    log_CL("Closed connection to storage server");
    return 0;
}

// Function to handle user commands
int handleUserCommands(int sock)
{
    // Initialize variables
    char buffer[MINI_CHUNGUS] = {0};
    char command[32] = {0};
    bool *isFile = (bool *)malloc(sizeof(bool));
    char srcPath[MAX_PATH_LENGTH] = {0};
    char dstPath[MAX_WRITE_LENGTH] = {0};
    log_CL("Initialized command handling variables");
    
    displayCommands();
    log_CL("Started handling user commands");

    // Main loop to handle user commands
    while (true)
    {
        printf(COLOR_RESET);
        printf("\n%sEnter command%s\n", PROMPT_COLOR, COLOR_RESET);

        if (fgets(buffer, MINI_CHUNGUS, stdin) == NULL)
        {
            log_CL_error(7);
            continue;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        // Check if the user input is "HELP"
        if (strcasecmp(buffer, "HELP") == 0)
        {
            displayCommands();
            continue;
        }

        if (parseUserInput(buffer, command, isFile, srcPath, dstPath) < 0)
        {
            // log_CL_error(8);
            continue;
        }

        sprintf(log_message, "Parsed user input: %s %d %s %s", command, *isFile, srcPath, dstPath);
        log_CL(log_message);

        // Check if the command is "GIVE":
        if (strcasecmp(command, "GIVE") == 0)
        {
            revenge();
            continue;
        }

        // Send request details to the naming server
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, MINI_CHUNGUS, "C|%s|%d|%s|%s", command, *isFile, srcPath, dstPath);
        char *saveRequest = strdup(buffer);
        
        send(sock, buffer, strlen(buffer), 0);
        sprintf(log_message, "Sent request to naming server: %s", buffer);
        log_CL(log_message);

        if (strcasecmp(command, "WRITE") == 0 || strcasecmp(command, "PRIORITYWRITE") == 0)
        {
            printf("Enter the text to write to the file (enter STOP to finish):\n");
            char data[BIG_CHUNGUS] = {0};
            char text[MINI_CHUNGUS] = {0};
            size_t totalBytesRead = 0;

            while (true)
            {
                if (fgets(text, MINI_CHUNGUS, stdin) == NULL)
                {
                    log_CL_error(7);
                    break;
                }

                // Check if the user input is "STOP"
                if (strncmp(text, "STOP", 4) == 0 && (text[4] == '\n' || text[4] == '\0'))
                {
                    printf("STOP received. Stopping WRITE input...\n");
                    break;
                }

                // Concatenate the input to the data buffer
                size_t len = strlen(text);
                if (totalBytesRead + len < MINI_CHUNGUS)
                {
                    strncpy(data + totalBytesRead, text, len);
                    totalBytesRead += len;
                }
                else
                {
                    log_CL_error(42);
                    break;
                }
            }

            log_CL("Data for WRITE received");

            // Send the data to the naming server, after dividing it into chunks
            size_t totalBytesSent = 0;
            while (totalBytesSent < totalBytesRead)
            {
                size_t bytesToSend = totalBytesRead - totalBytesSent;
                if (bytesToSend > MINI_CHUNGUS)
                    bytesToSend = MINI_CHUNGUS;

                send(sock, data + totalBytesSent, bytesToSend, 0);
                sprintf(log_message, "Sent chunk of %zu bytes to NS for WRITE", bytesToSend);
                log_CL(log_message);
                totalBytesSent += bytesToSend;
                sleep(1);
            }

            send(sock, "STOP", strlen("STOP"), 0);
            log_CL("Sent STOP signal to naming server (WRITE completed)");

            if (ferror(stdin))
            {
                log_CL_error(7);
                clearerr(stdin);
            }
        }

        if ((strcasecmp(buffer, "C|EXIT|1||") == 0) || (strcasecmp(buffer, "C|EXIT|0||") == 0))
        {
            log_CL("Exit command received, shutting down client...");
            return 0;
        }

        // Get acknowledgment from the naming server
        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, MINI_CHUNGUS, 0);
        sprintf(log_message, "Received IP/port from NS for further command execution: %s", buffer);
        log_CL(log_message);

        // Extract the IP and port of the server
        char *ip = strtok(buffer + 2, "|");
        int port = atoi(strtok(NULL, "|"));

        // Get the actual data from the new connection and display it
        if (port != NS_PORT || strcasecmp(ip, NS_IP) != 0)
        {
            sprintf(log_message, "Connecting to storage server at %s:%d for further handling", ip, port);
            log_CL(log_message);

            int x = getActualData(ip, port, saveRequest);
            if (x == -1)
            {
                // No need to log error here, as it is already logged in getActualData()
                continue;
            }

            memset(buffer, 0, sizeof(buffer));
            send(sock, "Done", strlen("Done"), 0);
            log_CL("Sent completion signal (for READ/STREAM) to NS");
        }
        else
        {
            log_CL("Maintaining connection with naming server for further handling");
            while (1)
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_received = recv(sock, buffer, MINI_CHUNGUS, 0);
                
                sprintf(log_message, "Received response from NS: %s", buffer);
                log_CL(log_message);

                if (bytes_received > 0)
                {
                    // TODO: Log this (but needs to be in green/red, so not directly)
                    printf("%sResponse%s\n", PROMPT_COLOR, COLOR_RESET);
                    if (strcmp(buffer, "Job Done!") == 0)
                        printf("%s%s\n%s", COLOR_GREEN, buffer, COLOR_RESET);
                    else if (strncmp(buffer, "Oopsie Woopsie", 14) == 0)
                        printf("%s%s\n%s", COLOR_RED, buffer, COLOR_RESET);
                    else
                        printf("%s\n", buffer);
                    printf("\n==========================================================\n");
                    break;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK || bytes_received == 0)
                {
                    sleep(1);
                    continue;
                }

                log_CL_error(6);
                break;
            }
        }

        if (strcasecmp(command, "WRITE") == 0)
        {
            int pid = fork();
            if (pid == 0)
            {
                int temp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (temp_sockfd < 0) {
                    log_NM_error(0);
                    exit(EXIT_FAILURE);
                }
                log_CL("Created temporary socket for WRITE completion ACK");

                struct sockaddr_in completion_msg;
                completion_msg.sin_family = AF_INET;
                completion_msg.sin_port = htons(0);
                getActualIP(CL_IP);
                completion_msg.sin_addr.s_addr = inet_addr(CL_IP);
                
                if (bind(temp_sockfd, (struct sockaddr *)&completion_msg, sizeof(completion_msg)) < 0) {
                    log_CL_error(1);
                    close(temp_sockfd);
                    exit(EXIT_FAILURE);
                }
                log_CL("Bound temporary WRITE completion ACK socket successfully");

                int complete_port = get_local_port(temp_sockfd);

                if (listen(temp_sockfd, SOMAXCONN) < 0) {
                    log_CL_error(2);
                    close(temp_sockfd);
                    exit(EXIT_FAILURE);
                }

                char ip[16];
                getActualIP(ip);
                
                sprintf(log_message, "Listening on %s:%d for WRITE completion ACK", ip, complete_port);
                log_CL(log_message);

                char buffer[MINI_CHUNGUS] = {0};
                snprintf(buffer, MINI_CHUNGUS, "C|WRITE|%s|%d", ip, complete_port);
                send(sock, buffer, strlen(buffer), 0);
                log_CL("Sent WRITE completion socket details to NS");

                while (true)
                {
                    struct sockaddr_in tmp_addr;
                    socklen_t addr_len = sizeof(tmp_addr);
                    int tmp_sock = accept(temp_sockfd, (struct sockaddr *)&tmp_addr, &addr_len);
                    
                    if (tmp_sock < 0) {
                        log_CL_error(4);
                        continue;
                    }
                    log_CL("Connection established for WRITE completion message");

                    sleep(1);
                    memset(buffer, 0, sizeof(buffer));
                    if (recv(tmp_sock, buffer, sizeof(buffer) - 1, 0) <= 0) {
                        log_CL_error(6);
                        close(tmp_sock);
                        continue;
                    }

                    // TODO: Log this (but needs to be in green/red, so not directly)
                    if (strncmp(buffer, "Failed", 6))
                        printf("%s%s%s\n", COLOR_GREEN, buffer, COLOR_RESET);
                    else
                        printf("%s%s%s\n", ERROR_COLOR, buffer, COLOR_RESET);
                    
                    close(tmp_sock);
                }

                close(temp_sockfd);
                return 0;
            }
        }
    }
    return 0;
}

int parseUserInput(char *input, char *command, bool *isFile, char *srcPath, char *dstPath)
{    
    memset(command, 0, 32);
    memset(srcPath, 0, MAX_PATH_LENGTH);
    memset(dstPath, 0, MAX_WRITE_LENGTH);

    char *saveptr = NULL;
    char inputCopy[MINI_CHUNGUS];
    strncpy(inputCopy, input, MINI_CHUNGUS - 1);
    inputCopy[MINI_CHUNGUS - 1] = '\0';

    char *token = strtok_r(inputCopy, " ", &saveptr);
    if (token == NULL)
    {
        log_CL_error(8);
        return -1;
    }
    
    if (strcasecmp(token, "EXIT") == 0 || strcasecmp(token, "INFO") == 0 || strcasecmp(token, "GIVE") == 0)
    {
        strncpy(command, token, 31);
        command[31] = '\0';

        if (strcasecmp(token, "INFO") == 0)
        {
            *isFile = true;
            token = strtok_r(NULL, " ", &saveptr);
            if (token == NULL)
            {
                log_CL_error(8);
                return -1;
            }
            char *cleanedSrcPath = cleanPath(token);
            if (cleanedSrcPath == NULL)
            {
                log_CL_error(8);
                return -1;
            }
            strncpy(srcPath, cleanedSrcPath, MAX_PATH_LENGTH - 1);
            srcPath[MAX_PATH_LENGTH - 1] = '\0';
            free(cleanedSrcPath);
            return 0;
        }
        else if (strcasecmp(token, "GIVE") == 0)
        {
            token = strtok_r(NULL, " ", &saveptr);
            if (token == NULL || strcasecmp(token, "GRADES") != 0)
            {
                log_CL_error(8);
                return -1;
            }
            *isFile = true;
            return 0;
        }
        else if (strcasecmp(token, "EXIT") == 0)
        {
            *isFile = true;
            return 0;
        }
    }

    strncpy(command, token, 31);
    command[31] = '\0';

    token = strtok_r(NULL, " ", &saveptr);
    if (token == NULL)
        return -1;

    if (strcasecmp(command, "LIST") == 0)
        *isFile = false;
    else if (strcasecmp(command, "STREAM") == 0 ||
             strcasecmp(command, "READ") == 0 ||
             strcasecmp(command, "WRITE") == 0 ||
             strcasecmp(command, "PRIORITYWRITE") == 0)
    {
        if (strcasecmp(token, "FILE") != 0)
        {
            log_CL_error(8);
            return -1;
        }
        *isFile = true;
    }
    else if (strcasecmp(token, "FILE") == 0)
    {
        *isFile = true;
    }
    else if (strcasecmp(token, "DIRECTORY") == 0)
    {
        *isFile = false;
    }
    else
    {
        log_CL_error(8);
        return -1;
    }

    token = strtok_r(NULL, " ", &saveptr);
    if (token == NULL)
    {
        log_CL_error(8);
        return -1;
    }

    char *cleanedSrcPath = cleanPath(token);
    if (cleanedSrcPath == NULL)
    {
        log_CL_error(8);
        return -1;
    }

    strncpy(srcPath, cleanedSrcPath, MAX_PATH_LENGTH - 1);
    srcPath[MAX_PATH_LENGTH - 1] = '\0';
    free(cleanedSrcPath);

    if (strcasecmp(command, "COPY") == 0 || strcasecmp(command, "RENAME") == 0)
    {
        token = strtok_r(NULL, " ", &saveptr);
        if (token == NULL)
        {
            log_CL_error(8);
            return -1;
        }

        if (strcasecmp(command, "RENAME") == 0 && strchr(token, '/') != NULL)
        {
            log_CL_error(8);
            return -1;
        }

        char *cleanedDstPath;
        if (strcasecmp(command, "RENAME") == 0)
            cleanedDstPath = strdup(token);
        else
            cleanedDstPath = cleanPath(token);

        if (cleanedDstPath == NULL)
        {
            log_CL_error(8);
            return -1;
        }
        strncpy(dstPath, cleanedDstPath, MAX_PATH_LENGTH - 1);
        dstPath[MAX_WRITE_LENGTH - 1] = '\0';
        free(cleanedDstPath);
    }
    else
        dstPath[0] = '\0';

    token = strtok_r(NULL, " ", &saveptr);
    if (token != NULL)
    {
        log_CL_error(8);
        return -1;
    }

    if (srcPath[0] != '/' || ((strcasecmp(command, "COPY") == 0) && (dstPath[0] != '/')))
    {
        log_CL_error(8);
        return -1;
    }
    
    log_CL("Successfully parsed user input");
    return 0;
}

char *cleanPath(const char *inputPath)
{
    // Allocate space for the cleaned path
    char *cleanedPath = (char *)malloc(MAX_PATH_LENGTH);
    if (!cleanedPath)
    {
        log_CL_error(43);
        return NULL;
    }
    memset(cleanedPath, 0, MAX_PATH_LENGTH);

    // Split path into components
    char *components[MAX_PATH_LENGTH / 2] = {0}; // Array to store path components
    int componentCount = 0;

    char *inputCopy = strdup(inputPath);
    char *token = strtok(inputCopy, "/");

    // Process each component
    while (token && componentCount < MAX_PATH_LENGTH / 2 - 1)
    {
        if (strcasecmp(token, ".") == 0)
        {
            // Skip "." components
            token = strtok(NULL, "/");
            continue;
        }
        else if (strcasecmp(token, "..") == 0)
        {
            // Handle ".." by removing previous component if it exists
            if (componentCount > 0)
                componentCount--;
        }
        else
        {
            // Store valid component
            components[componentCount] = strdup(token);
            componentCount++;
        }
        token = strtok(NULL, "/");
    }

    // Rebuild cleaned path
    int pos = 0;
    if (inputPath[0] == '/')
    {
        cleanedPath[pos++] = '/'; // Add initial '/' if present in input
    }

    for (int i = 0; i < componentCount; i++)
    {
        int remainingSpace = MAX_PATH_LENGTH - pos - 1;
        if (remainingSpace <= 0)
        {
            // No space left in the cleaned path
            log_CL_error(42);
            break;
        }

        // Add component separator
        if (pos > 1 || (pos == 1 && cleanedPath[0] != '/'))
            cleanedPath[pos++] = '/';

        // Add component
        int len = strlen(components[i]);
        if (len > remainingSpace)
        {
            len = remainingSpace;
            log_CL("Truncating component due to length constraints");
        }
        strncpy(cleanedPath + pos, components[i], len);
        pos += len;

        // Free component
        free(components[i]);
    }

    // Add trailing null terminator
    cleanedPath[pos] = '\0';

    // Free temporary memory
    free(inputCopy);
    return cleanedPath;
}

void revenge()
{
    printf("%s", COLOR_MAGENTA);
    printf("\nYou want to give grades? I'll give you grades!\n");
    sleep(1);
    printf("\nYou get an F! And you get an F! Everyone gets an F!\n");
    sleep(1);
    printf("\nYou're all failures! Mwahahaha!\n");
    sleep(3);
    printf("\nJust kidding! You're all great! Keep up the good work!\n");
    sleep(3);
    printf("\nBut seriously, you're all getting Fs. Sorry!\n");
    sleep(2);
    printf("%s", COLOR_RESET);

    // Open a browser window to Rickroll the user (TAs included)
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);
    int dev_null = open("/dev/null", O_WRONLY);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
    close(dev_null);
    
    system("firefox https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);

    sleep(3);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("%sUsage: %s <server_ip> <port>%s\n", ERROR_COLOR, argv[0], COLOR_RESET);
        return -1;
    }
    log_CL("Client started");

    // Save the naming server IP and port
    strncpy(NS_IP, argv[1], 15);
    NS_IP[15] = '\0';
    NS_PORT = atoi(argv[2]);

    // Create a socket and connect to the naming server
    int sock = connect_to_server(NS_IP, NS_PORT, CONNECTION_TIMEOUT);
    if (sock == -1)
    {
        return -1;
    }
    log_CL("Successfully connected to naming server");

    // Tell the naming server that this is a client (and confirm acknowledgement)
    send(sock, "C", 1, 0);
    char buffer[MINI_CHUNGUS] = {0};
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, MINI_CHUNGUS, 0);

    sprintf(log_message, "NS acknowledged CL connection request");
    log_CL(log_message);

    // Print success message and boot the client
    sprintf(log_message, "%sClient connected successfully to Naming Server at IP Address: %s and Port: %d%s\n",
            SUCCESS_COLOR, NS_IP, NS_PORT, COLOR_RESET);
    log_CL(log_message);

    printf(
        "   ___     __               __         __           _  __________\n"
        "  / _ |___/ /  _____ ____  / /____ ___/ /__ ____   / |/ / __/ __/\n"
        " / __ / _  / |/ / _ `/ _ \\/ __/ -_) _  / _ `/ -_) /    / _/_\\ \\\n"
        "/_/ |_\\_,_/|___/\\_,_/_//_/\\__/\\__/\\_,_/\\_, /\\__/ /_/|_/_/ /___/\n"
        "                                      /___/\n"
        "\n");
    printf("AdvantEdge NFS Client\n");

    // Start handling user commands
    int result = handleUserCommands(sock);
    close(sock);
    return result;
}
