//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

int create_socket(int *sockfd)
{
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        log_error("Socket creation failed...\n");
        return -1;
    }
    log_msg("Socket successfully created..\n");
    return 0;
}

int bind_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr(ipv4);
    serveraddr->sin_port        = htons(port);

    if(connect(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        log_error("Connection with the server failed...\n");
        return -1;
    }

    log_msg("Connected to the server..\n");
    return 0;
}

void write_to_socket(int sockfd, const struct Message *msg, const void *payload, size_t payload_size)
{
    uint8_t buffer[sizeof(*msg) + sizeof(payload_size)];
    memcpy(buffer, msg, sizeof(*msg));
    memcpy(buffer + sizeof(*msg), payload, payload_size);

    send(sockfd, buffer, sizeof(buffer), 0);
    log_msg("Message sent\n");
}

char *read_from_socket(int sockfd)
{
    static char    buffer[BUFSIZE];
    struct Message msg;

    // Read the message header
    read(sockfd, &msg, sizeof(struct Message));

    // Ensure the payload fits in the buffer
    if(msg.payload_length >= BUFSIZE)
    {
        log_error("Payload too large to fit in buffer\n");
        buffer[0] = '\0';    // Set buffer to empty string in case of error
        return buffer;
    }

    // Read the payload
    read(sockfd, buffer, msg.payload_length);
    buffer[msg.payload_length] = '\0';    // Null-terminate the payload

    log_msg("Message received:\n");
    log_msg("Packet Type: %d\n", msg.packet_type);
    log_msg("Protocol Version: %d\n", msg.protocol_version);
    log_msg("Sender ID: %d\n", msg.sender_id);
    log_msg("Payload Length: %d\n", msg.payload_length);
    log_msg("Payload: %s\n", buffer);

    return buffer;
}
