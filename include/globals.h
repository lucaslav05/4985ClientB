//
// Created by jonathan on 2/13/25.
//

#ifndef CLIENT_GLOBALS_H
#define CLIENT_GLOBALS_H

#define PORT 44233
#define IPV4 "192.168.0.137"
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
#define TIME_SIZE 16
#define NAME_SIZE 256
#define CONTENT_SIZE 1024
#define GEN_TIM 15
#define UTC_TIM 13

// Assume MAX_HEADER_SIZE is defined as 6 (1 byte packet type, 1 byte protocol version,
// 2 bytes sender_id, 2 bytes payload length)
#ifndef MAX_HEADER_SIZE
    #define MAX_HEADER_SIZE 6
#endif

#endif    // CLIENT_GLOBALS_H
