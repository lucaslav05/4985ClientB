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
        log_msg("Socket creation failed...\n");
        return -1;
    }
    log_msg("Socket successfully created..\n");
    return 0;
}

int bind_socket(int sockfd, struct sockaddr_in *serveraddr, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr("127.0.0.1");    // Temporary IP
    serveraddr->sin_port        = htons(port);

    if(connect(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        log_msg("Connection with the server failed...\n");
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
    static char buffer[BUFSIZE] = {0};
    read(sockfd, buffer, BUFSIZE);
    printf("Message received: %s\n", buffer);    // log_msg doesnt work with %s stuff, we would need to bypass the compiler flags
    return buffer;
}
