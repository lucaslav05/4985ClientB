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
    uint32_t username;
    uint32_t password;
};

struct ACC_Create {
    uint32_t username;
    uint32_t password;
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


#endif //MESSAGE_H
