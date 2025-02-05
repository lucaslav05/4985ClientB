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


#endif //MESSAGE_H
