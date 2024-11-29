#include "../include/errors.h"

// Error codes:
// 0: Socket creation error
// 1: Socket bind error
// 2: Socket listen error
// 3: Socket attachment error
// 4: Connection accept failed
// 5: Thread creation failed
// 6: Did not receive message
// 7: Input read error
// 8: Invalid input format
// 9: Write to socket failed
// 10: File not found error
// 11: File already exists error
// 12: No storage servers error
// 13: Client socket closed error
// 14: Cluster not found error
// 15: Storage server socket closed error
// 16: Receive error
// 17: Unknown request type error
// 18: Write error
// 19: Bad file descriptor error
// 20: Metadata read error
// 21: Connection closed by peer error
// 22: Storage server not found error
// 23: Something went wrong
// 24: Possibly disconnected from NS error
// 25: Fork failed
// 26: Pipe failed
// 27: MPV player not found
// 28: Storage server not acknowledging
// 29: Network interfaces error
// 30: Socket name error
// 31: Storage server disconnected
// 32: Send error
// 33: mkdir error
// 34: open error
// 35: remove error
// 36: ftw error
// 37: File read error
// 38: creat error
// 39: rename error
// 40: copy error
// 41: nftw error

char *error_messages[NUM_ERRORS] = {
    "Socket creation error",
    "Socket bind error",
    "Socket listen error",
    "Socket attachment error",
    "Connection accept failed",
    "Thread creation failed",
    "Did not receive message",
    "Input read error",
    "Invalid input format",
    "Write to socket failed",
    "File not found error",
    "File already exists error",
    "No storage servers error",
    "Client socket closed error",
    "Cluster not found error",
    "Storage server socket closed error",
    "Receive error",
    "Unknown request type error",
    "Write error",
    "Bad file descriptor error",
    "Metadata read error",
    "Connection closed by peer error",
    "Storage server not found error",
    "Something went wrong",
    "Possibly disconnected from NS error",
    "Fork failed",
    "Pipe failed",
    "MPV player not found",
    "Storage server not acknowledging",
    "Network interfaces error",
    "Socket name error",
    "Storage server disconnected",
    "mkdir error",
    "open error",
    "remove error",
    "ftw error",
    "File read error",
    "creat error",
    "rename error"
};

void log_NM_error(int error_code)
{
    if (error_code < 0 || error_code >= NUM_ERRORS)
    {
        fprintf(stderr, "Invalid error code: %d\n", error_code);
        return;
    }

    time_t now;
    char timestamp[26];

    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';

    printf(ERROR_COLOR "[%s] %d: %s%s\n", timestamp, error_code, error_messages[error_code], COLOR_RESET);

    // Log to file
    FILE *log_file = fopen("nfs.log", "a");
    if (log_file == NULL)
    {
        fprintf(stderr, "Failed to open log file\n");
        return;
    }

    fprintf(log_file, "[%s] %d: %s\n", timestamp, error_code, error_messages[error_code]);
    fclose(log_file);
}

void log_CL_error(int error_code)
{
    if (error_code < 0 || error_code >= NUM_ERRORS)
    {
        fprintf(stderr, "Invalid error code: %d\n", error_code);
        return;
    }

    printf(ERROR_COLOR "[%d] Oopsie Woopsie: %s%s\n", error_code, error_messages[error_code], COLOR_RESET);
}

void log_SS_error(int error_code)
{
    if (error_code < 0 || error_code >= NUM_ERRORS)
    {
        fprintf(stderr, "Invalid error code: %d\n", error_code);
        return;
    }

    time_t now;
    char timestamp[26];

    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';

    printf(ERROR_COLOR "[%s] %d: %s%s\n", timestamp, error_code, error_messages[error_code], COLOR_RESET);

    // Log to file
    FILE *log_file = fopen("storage_server.log", "a");
    if (log_file == NULL)
    {
        fprintf(stderr, "Failed to open log file\n");
        return;
    }

    fprintf(log_file, "[%s] %d: %s\n", timestamp, error_code, error_messages[error_code]);
    fclose(log_file);
}

void log_CL(char *message)
{
    time_t now;
    char timestamp[26];

    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';

    printf(INFO_COLOR "[%s] %s%s\n", timestamp, message, COLOR_RESET);

    // Log to file
    FILE *log_file = fopen("client.log", "a");
    if (log_file == NULL)
    {
        fprintf(stderr, "Failed to open log file\n");
        return;
    }

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);
}

void log_SS(int port, char *message)
{
    time_t now;
    char timestamp[26];

    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';

    printf(INFO_COLOR "[%s] %s%s\n", timestamp, message, COLOR_RESET);
    
    // Log to file
    FILE *log_file = fopen("storage_server.log", "a");
    if (log_file == NULL)
    {
        fprintf(stderr, "Failed to open log file\n");
        return;
    }

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);
}