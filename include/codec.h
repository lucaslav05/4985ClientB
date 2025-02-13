//
// Created by jonathan on 2/13/25.
//

#ifndef CLIENT_CODEC_H
#define CLIENT_CODEC_H

#include "clog.h"
#include "globals.h"
#include "message.h"
#include <netinet/in.h>

void decode_header(const uint8_t buf[], struct Message *msg);
void encode_header(uint8_t buf[], const struct Message *msg);
void encode_payload(uint8_t *buffer, Tags tag, const void *data, size_t *payload_size);
void decode_payload(uint8_t *buffer, Tags *tag, void *data);

#endif    // CLIENT_CODEC_H
