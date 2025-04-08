//
// Created by jonathan on 2/13/25.
//

#ifndef CLIENT_GLOBALS_H
#define CLIENT_GLOBALS_H

#define PORT 8080 // Server manager port
#define IPV4 "192.168.0.46" // Server manager IP
#define MAX_IP_REQUEST_SIZE 2
#define IP_MAX_CHARS 16
#define IP_BASE 10
#define PORT_BASE 10
#define IP_MAX 255

#define MAX_HEADER_SIZE 6
#define ENCODING_OVERHEAD 20
#define BUFFER 1024
#define TIME_BUFFER 64
#define DIVIDEND 1000
#define MAX_SIZE 256
#define TLV 4
#define NANO 1000000000
#define FPS 120
#define FIXED_UPDATE (NANO / FPS)
#define MAX_MESSAGES 100
#define BACKSPACE 127
#define LOWERBOUND 32
#define UPPERBOUND 126
#define GEN_TIM 15
#define UTC_TIM 13
#define TIME_SIZE 16
#define NAME_SIZE 256
#define CONTENT_SIZE 1024
#define FIFTY_TIMES 50

#endif    // CLIENT_GLOBALS_H
