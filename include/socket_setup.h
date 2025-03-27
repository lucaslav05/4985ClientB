//
// Created by jonathan on 2/6/25.
//

#ifndef CLIENT_SOCKET_SETUP_H
#define CLIENT_SOCKET_SETUP_H

#include "codec.h"
#include "globals.h"
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

int             get_active_server_ip(char *buffer, const char *ipv4, uint16_t port);
int             create_socket(int *sockfd);
int             connect_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port);
int             handle_response(const unsigned char *response, ssize_t bytes_received, char *ip_buffer, uint16_t port);
uint16_t        convert_port(const char *port_str);
bool            is_valid_ip(const char *ip);
void            write_to_socket(int sockfd, struct Message *msg, const void *payload, size_t payload_size);
struct Message *read_from_socket(int sockfd, char *buffer);

#endif    // CLIENT_SOCKET_SETUP_H
