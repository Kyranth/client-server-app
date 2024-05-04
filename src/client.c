#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/cJSON.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define THRESHOLD 100

#define PACKET_SIZE 1000
#define INTER_MEASUREMENT_TIME 15

// Structure definition
typedef struct
{
    char *server_ip_address;
    int tcp_pre_probing_port;
    int udp_source_port;
    int udp_destination_port;
    int tcp_head_syn_port;
    int tcp_tail_syn_port;
    int udp_payload_size;
    int inter_measurement_time;
    int num_udp_packets;
    int udp_packet_ttl;
} Config;

typedef struct
{
    uint16_t packet_id;
    char payload[PACKET_SIZE - sizeof(uint16_t)];
} UDP_Packet;

Config *createConfig()
{
    Config *config = malloc(sizeof(Config));
    if (config == NULL)
    {
        perror("Failed to allocate memory for Config");
        exit(1);
    }

    // Assuming IPv4 address, so 15 characters + null terminator
    config->server_ip_address = malloc(16 * sizeof(char));

    if (config->server_ip_address == NULL)
    {
        perror("Failed to allocate memory for server_ip_address");
        exit(1);
    }

    return config;
}

void p_error(const char *msg)
{
    perror(msg);
    exit(1);
}

/**
 * @brief Takes a filename and returns the cJSON object
 *
 * @param file pointer to a filename
 * @param config structure
 */
void setConfig(const char *file, Config *config)
{
    // open the file
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        p_error("Could not open config file\n");
    }

    // read the file contents into a string
    char buffer[1024];
    fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    // parse the JSON data
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("Error: %s\n", error_ptr);
        }
        exit(1);
    }
    else
    {
        strcpy(config->server_ip_address,
               cJSON_GetObjectItemCaseSensitive(json, "server_ip_address")->valuestring);
        config->tcp_pre_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valueint;
        config->udp_source_port = cJSON_GetObjectItemCaseSensitive(json, "udp_source_port")->valueint;
        config->udp_destination_port = cJSON_GetObjectItemCaseSensitive(json, "udp_destination_port")->valueint;
        config->tcp_head_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_head_syn_port")->valueint;
        config->tcp_tail_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_tail_syn_port")->valueint;
        config->udp_payload_size = cJSON_GetObjectItemCaseSensitive(json, "udp_payload_size")->valueint;
        config->inter_measurement_time = cJSON_GetObjectItemCaseSensitive(json, "inter_measurement_time")->valueint;
        config->num_udp_packets = cJSON_GetObjectItemCaseSensitive(json, "num_udp_packets")->valueint;
        config->udp_packet_ttl = cJSON_GetObjectItemCaseSensitive(json, "udp_packet_ttl")->valueint;
    }

    // delete the JSON object
    cJSON_Delete(json);
}

/**
 * Creates a TCP file descriptor and returns for use.
 *
 * @return sockfd file descriptor
 */
int init_tcp()
{
    int sockfd;
    // Creating TCP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: TCP Socket creation failed\n");
        return -1;
    }
    return sockfd;
}

/**
 * Creates a UDP file descriptor and returns for use.
 *
 * @return sockfd file descriptor or -1 if unsuccessful
 */
int init_udp()
{
    int sockfd;
    // Creating UDP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
    {
        printf("ERROR: UDP Socket creation failed\n");
        return -1;
    }
    return sockfd;
}

/**
 * @brief Takes sockfd and sends config file over connection
 *
 * @param sockfd socket file descriptor or -1 if unsuccessful
 */
int send_ConfigFile(int sockfd)
{
    /** Create file */
    FILE *config_file;

    /** Open and read file */
    config_file = fopen("config.json", "r");
    if (config_file == NULL)
    {
        printf("ERROR: Opening config file\n");
        return -1;
    }

    ssize_t bytes = 0;
    char buffer[BUFFER_SIZE] = {0};
    while (fgets(buffer, BUFFER_SIZE, config_file) != NULL)
    {
        bytes = send(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes < 0)
        {
            printf("ERROR: Couldn't send file\n");
            return -1;
        }
        memset(buffer, 0, BUFFER_SIZE); // reset buffer
    }
    // Close file
    fclose(config_file);
    return 0;
}

int main(int argc, char *argv[])
{
    /** Check for command line arguments */
    if (argc < 1)
    {
        printf("Error: Expected (1) argument config.json file\n");
        exit(0);
    }
    char *config_file = argv[1]; // Get config file from command line

    struct sockaddr_in servaddr, cliaddr;

    /** Initialize a config structure to NULL values */
    Config *config = createConfig();

    /** Read the config file for server IP and fill it with JSON data */
    setConfig(config_file, config);

    /** Init TCP Connection */
    int sockfd = init_tcp();

    // Enable DF flag
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &enable, sizeof(enable)) < 0)
    {
        p_error("setsockopt failed\n");
    }

    /** Set server address */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->tcp_pre_probing_port);
    printf("Server IP/Port: %s/%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    /** Connect to server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("TCP Connection Failed\n");
    }

    /** Pre-Probing Phase: Send config file */
    int send = send_ConfigFile(sockfd);
    if (send < 0)
    {
        p_error("Couldn't send config file\n");
    }
    close(sockfd); // TCP Connection
    printf("TCP Closed. Initiating UDP...\n");

    /** Probing Phase: Sending low and high entropy data */
    // [1] Initiate UDP Connection
    sockfd = init_udp();
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(config->udp_source_port);
    servaddr.sin_port = htons(config->udp_destination_port);
    printf("Server IP/Port: %s/%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    // [3] Generate low entropy data (all 0s)
    UDP_Packet packet[10];

    // Send low entropy data packet
    for (int i = 0; i < 10; i++)
    {
        // Prepare packet payload with packet ID
        packet[i].packet_id = htons(i + 1);
        memset(packet[i].payload, 0, sizeof(packet[i].payload));

        // Send
        sendto(sockfd, &packet[i], PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        printf("Sent packet with ID: %d\n", ntohs(packet[i].packet_id));

        // Prevent sending packets too fast
        sleep(1);
    }

    // [7] Wait before sending high entropy data
    // sleep(INTER_MEASUREMENT_TIME);

    // [8] Send high entropy data packet
    // Generate high entropy data (random numbers), rand to a file
    // srand(time(NULL));
    // char high_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    // for (int i = 0; i < sizeof(high_entropy_data); ++i)
    // {
    //     high_entropy_data[i] = rand() % 256;
    // }

    /** Post Probing Phase: Calculate compression; done on Server */
    close(sockfd);
    free(config); // Release config
    printf("Connection closed\n");
    return 0;
}
