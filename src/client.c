//
// Created by lucas-laviolette on 1/30/25.
//

#include "account.h"
#include "message.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080

// #define P_BUFMAX 65541

void send_acc_login(const int *sockfd, const struct account *client);
void send_acc_create(const int *sockfd, const struct account *client);
int  setup_socket(int *sockfd, struct sockaddr_in *serveraddr);

int main(void)
{
    int                sockfd;
    struct sockaddr_in serveraddr;
    struct account     client;

    if(setup_socket(&sockfd, &serveraddr) != 0)
    {
        printf("Could not establish connection. Exiting...\n");
        return 1;
    }

    printf("Enter username: ");
    scanf("%49s", client.username);
    strncpy(client.password, getpass("Enter password: "), MAX_PASS);
    client.password[MAX_PASS - 1] = '\0';

    send_acc_login(&sockfd, &client);

    // listen for response

    printf("\nUsername: %s\n", client.username);
    printf("Password: %s\n", client.password);

    close(sockfd);
}

int setup_socket(int *sockfd, struct sockaddr_in *serveraddr)
{
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        perror("socket creation failed...\n");
        return -1;
    }
    printf("socket successfully created..\n");

    memset(serveraddr, 0, sizeof(*serveraddr));

    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr("127.0.0.1");    // Temporary IP
    serveraddr->sin_port        = htons(PORT);

    if(connect(*sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        perror("connection with the server failed");
        return -1;
    }

    printf("connected to the server..\n");
    return 0;
}

void send_acc_login(const int *sockfd, const struct account *client)
{
    struct Message   msg;
    struct ACC_Login lgn;
    uint8_t          buffer[sizeof(msg) + sizeof(lgn)];

    strncpy(lgn.username, client->username, MAX_USER);
    lgn.username[MAX_USER - 1] = '\0';

    strncpy(lgn.password, client->password, MAX_PASS);
    lgn.password[MAX_PASS - 1] = '\0';

    msg.packet_type      = ACC_LOGIN;
    msg.protocol_version = 1;
    msg.sender_id        = 0;
    msg.payload_length   = sizeof(lgn);

    memcpy((void *)buffer, &msg, sizeof(msg));
    memcpy((void *)(buffer + sizeof(msg)), &lgn, sizeof(lgn));

    send(*sockfd, buffer, sizeof(buffer), 0);
}

void send_acc_create(const int *sockfd, const struct account *client)
{
    struct Message    msg;
    struct ACC_Create crt;
    uint8_t           buffer[sizeof(msg) + sizeof(crt)];

    strncpy(crt.username, client->username, MAX_USER);
    crt.username[MAX_USER - 1] = '\0';    // Ensure null-termination

    strncpy(crt.password, client->password, MAX_PASS);
    crt.password[MAX_PASS - 1] = '\0';    // Ensure null-termination

    msg.packet_type      = ACC_CREATE;
    msg.protocol_version = 1;
    msg.sender_id        = 0;
    msg.payload_length   = sizeof(crt);

    memcpy(buffer, &msg, sizeof(msg));
    memcpy(buffer + sizeof(msg), &crt, sizeof(crt));

    send(*sockfd, buffer, sizeof(buffer), 0);
}
