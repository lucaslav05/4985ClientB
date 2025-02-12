//
// Created by lucas-laviolette on 1/30/25.
//

#include "account.h"
#include "clog.h"
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

#define PORT 8081
#define IPV4 "0.0.0.0"
#define TLV 4
// #define P_BUFMAX 65541

void send_acc_login(const int *sockfd, const struct account *client);
void send_acc_create(const int *sockfd, const struct account *client);

// int  setup_socket(int *sockfd, struct sockaddr_in *serveraddr);

int main(void)
{
    int                sockfd;
    struct sockaddr_in serveraddr;
    struct Message     response_msg;
    struct account     client;
    // const char        *server_r;
    char buffer[BUFSIZE];

    open_console();    // open external console, good for when we have n curses

    log_msg("Ip Address: %s\n", IPV4);
    log_msg("Port: %d\n", PORT);
    log_msg("Buffer Size: %d\n", BUFSIZE);

    if(create_socket(&sockfd) == -1)
    {
        exit(EXIT_FAILURE);
    }

    if(bind_socket(sockfd, &serveraddr, IPV4, PORT) == -1)
    {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Enter username: ");
    scanf("%49s", client.username);
    strncpy(client.password, getpass("Enter password: "), MAX_SIZE);

    send_acc_login(&sockfd, &client);

    response_msg = read_from_socket(sockfd, buffer);    // temp

    switch(response_msg.packet_type)
    {
        case ACC_LOGIN_SUCCESS:
            log_msg("Successfully logged in.\n");
            break;

        case SYS_ERROR:
            log_msg("Error logging in.\n");
            send_acc_create(&sockfd, &client);

            response_msg = read_from_socket(sockfd, buffer);    // temp

            if(response_msg.packet_type == SYS_SUCCESS)
            {
                log_msg("Successfully created account\n");
            }
            else
            {
                log_msg("Account creation failed\n");
            }
            break;

        default:
            log_msg("Unknown response from server: 0x%02X\n", response_msg.packet_type);
            break;
    }

    log_msg("\nUsername: %s\n", client.username);
    log_msg("Password: %s\n", client.password);

    close(sockfd);
}

void send_acc_login(const int *sockfd, const struct account *client)
{
    struct Message   msg;
    struct ACC_Login lgn;

    lgn.username = (char *)malloc(strlen(client->username) + 1);
    lgn.password = (char *)malloc(strlen(client->password) + 1);

    if(!lgn.username || !lgn.password)
    {
        log_error("Login info allocation failed\n");
        free(lgn.username);
        free(lgn.password);
        return;
    }

    memcpy(lgn.username, client->username, strlen(client->username));
    memcpy(lgn.password, client->password, strlen(client->password));

    lgn.username[strlen(client->username)] = '\0';
    lgn.password[strlen(client->password)] = '\0';

    msg.packet_type      = ACC_LOGIN;
    msg.protocol_version = 1;
    msg.sender_id        = 0;
    msg.payload_length   = htons((uint16_t)(TLV + strlen(lgn.username) + strlen(lgn.password)));

    write_to_socket(*sockfd, &msg, &lgn, (size_t)(TLV + strlen(lgn.username) + strlen(lgn.password)));

    // Free allocated memory before exiting
    free(lgn.username);
    free(lgn.password);
}

void send_acc_create(const int *sockfd, const struct account *client)
{
    struct Message    msg;
    struct ACC_Create crt;

    crt.username = malloc(strlen(client->username) + 1);
    crt.password = malloc(strlen(client->password) + 1);

    if(!crt.username || !crt.password)
    {
        log_error("Creation info allocation failed\n");
        free(crt.username);
        free(crt.password);
        return;
    }

    strlcpy(crt.username, client->username, MAX_SIZE);
    strlcpy(crt.password, client->password, MAX_SIZE);

    msg.packet_type      = ACC_CREATE;
    msg.protocol_version = 1;
    msg.sender_id        = 0;
    msg.payload_length   = (uint16_t)(strlen(crt.username) + strlen(crt.password));

    write_to_socket(*sockfd, &msg, &crt, sizeof(crt));

    // Free allocated memory before exiting
    free(crt.username);
    free(crt.password);
}

/*int setup_socket(int *sockfd, struct sockaddr_in *serveraddr)
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

    strncpy(lgn.username, client->username, MAX_SIZE);
    lgn.username[MAX_SIZE - 1] = '\0';

    strncpy(lgn.password, client->password, MAX_SIZE);
    lgn.password[MAX_SIZE - 1] = '\0';

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

    strncpy(crt.username, client->username, MAX_SIZE);
    crt.username[MAX_SIZE - 1] = '\0';    // Ensure null-termination

    strncpy(crt.password, client->password, MAX_SIZE);
    crt.password[MAX_SIZE - 1] = '\0';    // Ensure null-termination

    msg.packet_type      = ACC_CREATE;
    msg.protocol_version = 1;
    msg.sender_id        = 0;
    msg.payload_length   = sizeof(crt);

    memcpy(buffer, &msg, sizeof(msg));
    memcpy(buffer + sizeof(msg), &crt, sizeof(crt));

    send(*sockfd, buffer, sizeof(buffer), 0);
}*/
