//
// Created by lucas-laviolette on 3/4/25.
//

#include "chat.h"
#include "clog.h"     // for logging macros
#include "codec.h"    // for encode_header()
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

// Assume MAX_HEADER_SIZE is defined as 6 (1 byte packet type, 1 byte protocol version,
// 2 bytes sender_id, 2 bytes payload length)
#ifndef MAX_HEADER_SIZE
    #define MAX_HEADER_SIZE 6
#endif

#define TIME_SIZE 16
#define NAME_SIZE 256
#define CONTENT_SIZE 1024

// The send_chat_message function builds a chat payload manually according to the protocol:
// TLV for timestamp (GeneralizedTime), then TLV for content and TLV for username.
void send_chat_message(int sockfd, struct Message *msg, const struct CHT_Send *chat)
{
    char      time_str[TIME_SIZE];    // "YYYYMMDDhhmmssZ" is 15 characters plus a null terminator
    size_t    timestamp_len;
    size_t    content_len;
    size_t    username_len;
    size_t    payload_len;
    size_t    total_size;
    uint8_t  *buffer;
    uint8_t  *ptr;
    ssize_t   sent_bytes;
    struct tm tm_info;

    gmtime_r(&chat->timestamp, &tm_info);

    // Format time in GeneralizedTime: YYYYMMDDhhmmssZ
    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%SZ", &tm_info);
    timestamp_len = strlen(time_str);    // should be 15

    content_len  = strlen(chat->content);
    username_len = strlen(chat->username);

    // Each TLV has: 1 byte tag + 1 byte length + value bytes.
    payload_len = (1 + 1 + timestamp_len) + (1 + 1 + content_len) + (1 + 1 + username_len);

    total_size = MAX_HEADER_SIZE + payload_len;

    buffer = (uint8_t *)malloc(total_size);
    if(!buffer)
    {
        LOG_ERROR("Memory allocation failed for chat message buffer\n");
        return;
    }

    // Encode the header into the buffer.
    // (Header: 1 byte packet type, 1 byte protocol version, 2 bytes sender_id, 2 bytes payload length)

    LOG_MSG("Calculated Payload Length: %zu\n", payload_len);

    msg->payload_length = (uint16_t)payload_len;
    encode_header(buffer, msg);

    LOG_MSG("Encoding Payload Length: %u\n", msg->payload_length);

    ptr = buffer + MAX_HEADER_SIZE;

    // --- Encode timestamp TLV ---
    // Use the tag for GeneralizedTime. In our Tags enum, UTCTIME is defined as 23,
    // so GENERALIZEDTIME will be 24 (if not explicitly assigned, it is one more than UTCTIME).
    *ptr++ = (uint8_t)GENERALIZEDTIME;    // Tag for GeneralizedTime
    *ptr++ = (uint8_t)timestamp_len;      // Length of timestamp string
    memcpy(ptr, time_str, timestamp_len);
    ptr += timestamp_len;

    // --- Encode message content TLV ---
    *ptr++ = (uint8_t)UTF8STRING;    // Tag for UTF8STRING (defined as 12)
    *ptr++ = (uint8_t)content_len;
    memcpy(ptr, chat->content, content_len);
    ptr += content_len;

    // --- Encode username TLV ---
    *ptr++ = (uint8_t)UTF8STRING;    // Tag for UTF8STRING
    *ptr++ = (uint8_t)username_len;
    memcpy(ptr, chat->username, username_len);

    // Send the complete message over the socket.
    sent_bytes = send(sockfd, buffer, total_size, 0);
    if(sent_bytes < 0)
    {
        LOG_ERROR("Failed to send chat message\n");
    }
    else
    {
        LOG_MSG("Chat message sent (size: %zu bytes)\n", total_size);
    }
    free(buffer);
}

void read_chat_message(uint8_t *buffer)
{
    uint8_t  *ptr = buffer;
    uint8_t   tag;
    uint8_t   len;
    struct tm tm_info;
    time_t    timestamp;
    char      username[NAME_SIZE];      // Buffer to hold the username
    char      content[CONTENT_SIZE];    // Buffer to hold the chat content

    // The first part is the header, so we skip it (MAX_HEADER_SIZE is defined earlier).
    ptr += MAX_HEADER_SIZE;

    // Read timestamp (GeneralizedTime TLV)
    tag = *ptr++;
    len = *ptr++;
    if(tag == GENERALIZEDTIME)
    {
        char time_str[TIME_SIZE];    // Buffer to hold the formatted time string
        memcpy(time_str, ptr, len);
        time_str[len] = '\0';    // Null-terminate the string
        ptr += len;
        // Convert the timestamp string to time_t
        strptime(time_str, "%Y%m%d%H%M%SZ", &tm_info);
        timestamp = mktime(&tm_info);
    }
    else
    {
        LOG_ERROR("Expected GeneralizedTime tag for timestamp, got %d\n", tag);
        return;
    }

    // Read message content (UTF8STRING TLV)
    tag = *ptr++;
    len = *ptr++;
    if(tag == UTF8STRING)
    {
        memcpy(content, ptr, len);
        content[len] = '\0';    // Null-terminate the string
        ptr += len;
    }
    else
    {
        LOG_ERROR("Expected UTF8STRING tag for content, got %d\n", tag);
        return;
    }

    // Read username (UTF8STRING TLV)
    tag = *ptr++;
    len = *ptr++;
    if(tag == UTF8STRING)
    {
        memcpy(username, ptr, len);
        username[len] = '\0';    // Null-terminate the string
    }
    else
    {
        LOG_ERROR("Expected UTF8STRING tag for username, got %d\n", tag);
        return;
    }

    // Display the parsed message
    printf("[%s] %s: %s\n", ctime(&timestamp), username, content);
}
