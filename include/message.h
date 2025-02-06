//
// Created by lucas-laviolette on 2/5/25.
//
#include <stdint.h>

#ifndef MESSAGE_H
#define MESSAGE_H

struct Message {
    uint8_t packet_type;
    uint8_t protocol_version;
    uint16_t sender_id;
    uint16_t payload_length;
};

struct ACC_Login {
    char username[50];
    char password[50];
};

struct ACC_Create {
    char username[50];
    char password[50];
};

typedef enum {
    BOOLEAN = 1,
    INTEGER = 2,
    NULL_VAL = 5,
    ENUMERATED = 10,
    UTF8STRING = 12,
    SEQUENCE = 16,
    PRINTABLESTRING = 19,
    UTCTIME = 23,
    GENERALIZEDTIME = 24,
} Tag;

typedef enum {
    SYS_SUCCESS = 0,
    SYS_ERROR = 1,
    ACC_LOGIN = 10,
    ACC_LOGIN_SUCCESS = 11,
    ACC_LOGOUT = 12,
    ACC_CREATE = 13,
    ACC_EDIT = 14
} PacketType;


#endif //MESSAGE_H
