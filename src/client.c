//
// Created by lucas-laviolette on 1/30/25.
//

#include "account.h"
#include "clog.h"
#include "globals.h"
#include "message.h"
#include "socket_setup.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
    int                sockfd;
    struct sockaddr_in serveraddr;
    struct Message    *response_msg;
    struct account     client;
    char               buffer[BUFFER];
    struct timespec    ts;

    ts.tv_sec  = FIXED_UPDATE / NANO;
    ts.tv_nsec = FIXED_UPDATE % NANO;

    open_console();    // open external console, good for when we have n curses

    LOG_MSG("Ip Address: %s\n", IPV4);
    LOG_MSG("Port: %d\n", PORT);
    LOG_MSG("Buffer Size: %d\n", BUFFER);

    LOG_MSG("Creating socket...\n");
    if(create_socket(&sockfd) == -1)
    {
        LOG_ERROR("Creating socket failed\n");
        exit(EXIT_FAILURE);
    }

    LOG_MSG("Binding socket...\n");
    if(bind_socket(sockfd, &serveraddr, IPV4, PORT) == -1)
    {
        LOG_ERROR("Binding socket failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    LOG_MSG("Prompting login...!\n");
    printf("Enter username: ");
    scanf("%49s", client.username);
    strncpy(client.password, getpass("Enter password: "), MAX_SIZE);

    LOG_MSG("Sending account login...\n");
    send_acc_login(&sockfd, &client);

    LOG_MSG("Reading response...\n");
    response_msg = read_from_socket(sockfd, buffer);

    while(!response_msg)
    {
        nanosleep(&ts, NULL);
        response_msg = read_from_socket(sockfd, buffer);
    }

    switch(response_msg->packet_type)
    {
        case ACC_LOGIN_SUCCESS:
            LOG_MSG("Successfully logged in.\n");
            break;

        case SYS_ERROR:
            LOG_MSG("Error logging in.\n");
            LOG_MSG("Creating account...\n");
            send_acc_create(&sockfd, &client);
            LOG_MSG("Reading response...\n");
            response_msg = read_from_socket(sockfd, buffer);

            while(!response_msg)
            {
                nanosleep(&ts, NULL);
                response_msg = read_from_socket(sockfd, buffer);
            }
            if(response_msg->packet_type == SYS_SUCCESS)
            {
                LOG_MSG("Successfully created account\n");
                break;
            }
            LOG_ERROR("Account creation failed\n");
            break;

        default:
            LOG_ERROR("Unknown response from server: 0x%02X\n", response_msg->packet_type);
            break;
    }

    LOG_MSG("Username: %s\n", client.username);
    LOG_MSG("Password: %s\n", client.password);

    LOG_MSG("Cleaning up and exiting...\n");
    close(sockfd);
    free(response_msg);
}
