//
// Created by lucas-laviolette on 1/30/25.
//

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "account.h"

#define PORT 8080



int main(void) {

    int sockfd;

    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        printf("socket successfully created..\n");
    }

    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Temporary IP
    serveraddr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else {
        printf("connected to the server..\n");
    }


    close(sockfd);
}

