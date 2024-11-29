#include "../include/network.h"

int connect_to_server(const char *server_ip, int server_port, int timeout_sec)
{
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("%sSocket Creation Error%s\n", ERROR_COLOR, COLOR_RESET);
        return -1;
    }

    // Set socket timeout if specified
    if (timeout_sec > 0)
    {
        if (set_socket_timeout(sockfd, timeout_sec) < 0)
        {
            close(sockfd);
            return -1;
        }
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Convert IP address from text to binary form
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("%sInvalid Address%s\n", ERROR_COLOR, COLOR_RESET);
        printf("address: %s\n", server_ip);
        close(sockfd);
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("%sConnection Failed: ", ERROR_COLOR);
        switch (errno)
        {
        case ETIMEDOUT:
            printf("Connection timed out%s\n", COLOR_RESET);
            break;
        case ECONNREFUSED:
            printf("Connection refused%s\n", COLOR_RESET);
            break;
        case ENETUNREACH:
            printf("Network is unreachable%s\n", COLOR_RESET);
            break;
        default:
            printf("Error code: %d%s\n", errno, COLOR_RESET);
        }

        printf("address: %s, port: %d\n", server_ip, server_port);

        close(sockfd);
        return -1;
    }

    return sockfd;
}

int set_socket_timeout(int sockfd, int timeout_sec)
{
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    // Set receive timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("%sError: Setting Receive Timeout Failed%s\n", ERROR_COLOR, COLOR_RESET);
        return -1;
    }

    // Set send timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("%sError: Setting Send Timeout Failed%s\n", ERROR_COLOR, COLOR_RESET);
        return -1;
    }

    return 0;
}

int send_data(int sockfd, const void *data, size_t len)
{
    size_t total_sent = 0;
    const char *data_ptr = (const char *)data;

    while (total_sent < len)
    {
        ssize_t sent = send(sockfd, data_ptr + total_sent, len - total_sent, 0);
        if (sent < 0)
        {
            if (errno == EINTR)
                continue; // Interrupted by signal, retry
            return -1;
        }
        if (sent == 0)
            return -1; // Connection closed by peer
        total_sent += sent;
    }

    return total_sent;
}

int receive_data(int sockfd, void *buffer, size_t len)
{
    size_t total_received = 0;
    char *buffer_ptr = (char *)buffer;
    memset(buffer, 0, MINI_CHUNGUS);

    while (total_received < len)
    {
        ssize_t received = recv(sockfd, buffer_ptr + total_received, len - total_received, 0);
        if (received < 0)
        {
            if (errno == EINTR)
                continue; // Interrupted by signal, retry
            return -1;
        }
        if (received == 0)
            return -1; // Connection closed by peer
        total_received += received;
    }

    return total_received;
}

void getActualIP(char *ip_buffer)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    if (getifaddrs(&ifaddr) == -1)
    {
        log_NM_error(9);
        exit(EXIT_FAILURE);
    }

    // Iterate through the list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        // Check for IPv4 addresses
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            // Skip loopback addresses
            if (strcmp(host, "127.0.0.1") != 0 && strcmp(host, "127.0.1.1") != 0)
            {
                strncpy(ip_buffer, host, NI_MAXHOST);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

int get_local_port(int sockfd)
{
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);

    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) == -1)
    {
        printf("%sError: Failed to get local port%s\n", ERROR_COLOR, COLOR_RESET);
        log_NM_error(30);
        return -1;
    }

    // Get port number
    return ntohs(local_addr.sin_port);
}

void clear_socket_buffer(int sockfd)
{
    char buffer[1024];
    while (recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT) > 0)
        ;
}
