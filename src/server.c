#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define SERVER_PORT 7777
#define BUFFER_SIZE 1024

void p_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int init_tcp()
{
    int sockfd;
    // Creating TCP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("ERROR: TCP Socket creation failed\n");
    }
    return sockfd;
}

int init_udp()
{
    int sockfd;
    // Creating UDP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        p_error("ERROR: UDP Socket creation failed\n");
    }
    return sockfd;
}

int process_config(int connfd)
{
    // Receive file size
    uint64_t file_size;
    if (recv(connfd, &file_size, BUFFER_SIZE, 0) > 0)
    {
        printf("File size: %ld\n", file_size);
    }

    FILE *fp = fopen("rec_config.json", "w");

    // Hold incoming data
    char buffer[BUFFER_SIZE];

    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;
    while (total_bytes_received < file_size)
    {
        bytes_received = recv(connfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1)
        {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        else if (bytes_received == 0)
        {
            printf("Client closed connection unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        fwrite(buffer, 1, bytes_received, fp);
        total_bytes_received += bytes_received;
    }

    // Send confirmation message
    char confirm[] = "Recieved";
    send(connfd, confirm, strlen(confirm), 0);
    close(connfd);
    fclose(fp);
    return 0;
}

int main()
{
    /** Create sokect connection */
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;

    /** Create TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("Socket creation failed");
    }

    /** Set server address */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    /** Bind socket */
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Bind failed");
    }

    /** Listen for connections */
    if (listen(sockfd, 5) < 0)
    {
        p_error("Listen failed");
    }

    /** Accept incoming connection */
    len = sizeof(cliaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
    {
        p_error("Accept failed");
    }

    /** Pre-Probing Phase: Process config file */
    int process = process_config(connfd);
    if (process < 0)
    {
        p_error("ERROR: Processing config file\n");
    }

    /** Probing Phase: Receive packet trains */
    // (Here you would receive and process the UDP packets)
    // For the sake of simplicity, let's assume it's done elsewhere

    /** Calculate time differences */
    // low_entropy_time = (end_time_low.tv_sec - start_time_low.tv_sec) * 1000.0 + (end_time_low.tv_usec - start_time_low.tv_usec) / 1000.0;
    // high_entropy_time = (end_time_high.tv_sec - start_time_high.tv_sec) * 1000.0 + (end_time_high.tv_usec - start_time_high.tv_usec) / 1000.0;

    /** Post-Probing Phase: Send findings */
    // detect_compression();
    // send(connfd, "Compression detected!", strlen("Compression detected!"), 0);

    /** Close TCP connection */
    close(sockfd);

    return 0;
}
