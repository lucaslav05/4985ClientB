//
// Created by lucas-laviolette on 1/30/25.
//

#include "account.h"
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

int main(void)
{
    int sockfd;

    struct sockaddr_in serveraddr;
    struct account     client;

    printf("Enter username: ");
    scanf("%49s", client.username);

    client.password = getpass("Enter password: ");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
    {
        printf("socket successfully created..\n");
    }

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");    // Temporary IP
    serveraddr.sin_port        = htons(PORT);

    printf("\nUsername: %s\n", client.username);
    printf("Password: %s\n", client.password);

    if(connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
    {
        printf("connected to the server..\n");
    }

    close(sockfd);
}
