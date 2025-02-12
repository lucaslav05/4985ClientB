//
// Created by lucas-laviolette on 2/5/25.
//
#include <stdint.h>

#ifndef MESSAGE_H
    #define MESSAGE_H

    #define MAX_USER 256

struct Message
{
    uint8_t  packet_type;
    uint8_t  protocol_version;
    uint16_t sender_id;
    uint16_t payload_length;
};

struct ACC_Login
{
    char *username;
    char *password;
};

struct ACC_Create
{
    char *username;
    char *password;
};

typedef enum
{
    SYS_SUCCESS       = 0x00,
    SYS_ERROR         = 0x01,
    ACC_LOGIN         = 0x0A,
    ACC_LOGIN_SUCCESS = 0x0B,
    ACC_LOGOUT        = 0x0C,
    ACC_CREATE        = 0x0D,
    ACC_EDIT          = 0x0E
} PacketType;

#endif    // MESSAGE_H
