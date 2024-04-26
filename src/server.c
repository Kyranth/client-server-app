#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/** Constant members */
#define SERVER_IP "127.0.0.1" // Update for server address
#define SERVER_PORT 8765
#define CLIENT_PORT 9876
#define PACKET_SIZE 1024

/**
 * @brief Represents the server side of the network compression detection application
 *
 */
typedef struct
{
    /** Socket object */
    int sockfd;

    /** IPv4 socket address */
    struct sockaddr_in servaddr, cliaddr;

    /** Message len */
    socklen_t clilen;

    /** Buffere told hold message */
    char buffer[PACKET_SIZE];

    /** Time variables */
    char start_time[20], end_time[20];
} Server;

/**
 * @brief Initialize the server
 *
 */
void start_server(Server *server)
{
    if (server == NULL)
    {
        perror("Error initializing Server");
    }

    /** Configure Server struct sockaddr_in */
    server->servaddr.sin_family = AF_INET;                // IPv4
    server->servaddr.sin_port = htons(SERVER_PORT);       // Server port
    server->servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Set to default interface

    /** Create UDP socket */
    server->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server->sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    /** Bind server address to socket descriptor */
    if (bind(server->sockfd, (struct sockaddr *)&server->servaddr, sizeof(server->servaddr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Receives two sets of UDP packets from the client.
 *
 * @param server this object pointer
 * @return int 0 if recieved successful, -1 otherwise
 */
int receive_packet(Server *server)
{
    while (1)
    {
        printf("Listening for pakcets...");
        socklen_t len = sizeof(server->cliaddr);
        ssize_t n = recvfrom(server->sockfd, (char *)server->buffer, PACKET_SIZE, 0, (struct sockaddr *)&server->cliaddr, &len);
        server->buffer[n] = '\0';
        printf("Client: %s\n", server->buffer);
    }

    // send the response
    sendto(server->sockfd, "msg recieved", PACKET_SIZE, 0, (struct sockaddr *)&server->cliaddr, sizeof(server->cliaddr));
    return 0;
}

void write_file(int sockfd)
{
    int n;
    FILE *fp;
    char *filename = "config.json";
    char buffer[PACKET_SIZE];

    fp = fopen(filename, "w");
    while (1)
    {
        n = recv(sockfd, buffer, PACKET_SIZE, 0);
        if (n <= 0)
        {
            break;
            return;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, PACKET_SIZE);
    }
    return;
}

int main()
{
    Server *server = (Server *)malloc(sizeof(Server));
    printf("Created Server");

    /** Initialize the server */
    start_server(server);

    /** Listen for the packets */
    printf("Listening for packets...");
    receive_packet(server);

    sleep(1);

    close(server->sockfd);
    free(server);

    return 0;
}