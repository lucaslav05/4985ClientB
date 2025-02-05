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

#define P_BUFMAX 65541

void send_acc_login(void);
void send_acc_create(void);
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

    client.password = getpass("Enter password: ");

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

void send_acc_login(const int *sockfd,struct account *client)
{
    struct Message msg;
    struct ACC_Login lgn;

    lgn.username = client->username;
    lgn.password = *client->password;

    msg.packet_type = ACC_LOGIN;
    msg.protocol_version = 1;
    msg.sender_id = client->uid;
    msg.payload_length = sizeof(lgn);

    uint8_t buffer[sizeof(msg) + sizeof(lgn)];
    memcpy(buffer, &msg, sizeof(msg));
    memcpy(buffer + sizeof(msg), &lgn, sizeof(lgn));

    send(*sockfd, buffer, sizeof(buffer), 0);

}

void send_acc_create(void)
{
}
