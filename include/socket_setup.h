//
// Created by jonathan on 2/6/25.
//

#ifndef CLIENT_SOCKET_SETUP_H
#define CLIENT_SOCKET_SETUP_H

#include "message.h"
#include <arpa/inet.h>
#include <curses.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

void           decode_header(const uint8_t buf[], struct Message *msg);
void           encode_header(uint8_t buf[], const struct Message *msg);
int            create_socket(int *sockfd);
int            bind_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port);
void           write_to_socket(int sockfd, struct Message *msg, const void *payload, size_t payload_size);
struct Message read_from_socket(int sockfd, char *buffer);

#endif    // CLIENT_SOCKET_SETUP_H
