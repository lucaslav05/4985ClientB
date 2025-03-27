//
// Created by lucas-laviolette on 3/4/25.
//

#include "chat.h"
#include "clog.h"     // for logging macros
#include "codec.h"    // for encode_header()
#include "globals.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

void *receive_messages(void *arg)
{
    int  sockfd = *(int *)arg;
    char payload_buffer[BUFFER];

    while(1)
    {
        struct Message *received_msg = read_from_socket(sockfd, payload_buffer);
        if(received_msg)
        {
            // Lock the mutex to safely access shared messages array
            const char *format = NULL;
            uint8_t     ts_len;
            uint16_t    content_len;
            uint8_t     username_len;
            time_t      timestamp;
            char        time_str[TIME_SIZE];
            char        content[CONTENT_SIZE];
            char        username[NAME_SIZE];
            struct tm   tm_info;
            int         pos = MAX_HEADER_SIZE;    // Skip header
            char        formatted_message[BUFFER];
            pthread_mutex_lock(&message_mutex);

            // --- Process timestamp TLV ---
            pos++;
            ts_len = (uint8_t)payload_buffer[pos++];
            if(ts_len >= TIME_SIZE)
            {
                LOG_ERROR("Timestamp length too long: %u\n", ts_len);
                pthread_mutex_unlock(&message_mutex);
                free(received_msg);    // Free the allocated memory for the message
                continue;
            }
            memcpy(time_str, &payload_buffer[pos], ts_len);
            time_str[ts_len] = '\0';
            pos += ts_len;

            // Determine timestamp format
            if(ts_len == GEN_TIM)
            {
                format = "%Y%m%d%H%M%SZ";
            }
            else if(ts_len == UTC_TIM)
            {
                format = "%y%m%d%H%M%SZ";
            }
            else
            {
                LOG_ERROR("Unexpected timestamp length: %u\n", ts_len);
                pthread_mutex_unlock(&message_mutex);
                free(received_msg);
                continue;
            }

            // Parse the timestamp
            memset(&tm_info, 0, sizeof(tm_info));
            if(strptime(time_str, format, &tm_info) == NULL)
            {
                LOG_ERROR("Failed to parse timestamp: %s with format %s\n", time_str, format);
                pthread_mutex_unlock(&message_mutex);
                free(received_msg);
                continue;
            }
            timestamp = mktime(&tm_info);

            // --- Process content TLV ---
            pos++;    // Skip content tag
            content_len = (uint8_t)payload_buffer[pos++];
            if(content_len >= CONTENT_SIZE)
            {
                LOG_ERROR("Content length too long: %u\n", content_len);
                pthread_mutex_unlock(&message_mutex);
                free(received_msg);
                continue;
            }
            memcpy(content, &payload_buffer[pos], content_len);
            content[content_len] = '\0';
            pos += content_len;

            // --- Process username TLV ---
            pos++;    // Skip username tag
            username_len = (uint8_t)payload_buffer[pos++];
            if(username_len >= NAME_SIZE - 1)
            {
                LOG_ERROR("Username length too long: %u\n", username_len);
                pthread_mutex_unlock(&message_mutex);
                free(received_msg);
                continue;
            }
            memcpy(username, &payload_buffer[pos], username_len);
            username[username_len] = '\0';

            // Format the final message
            snprintf(formatted_message, BUFFER, "[%s] %s: %s", ctime(&timestamp), username, content);

            // Add the formatted message to the messages array
            if(message_count < MAX_MESSAGES)
            {
                strncpy(messages[message_count++], formatted_message, sizeof(messages[0]));
            }
            else
            {
                // If the messages array is full, shift all messages up and add the new one
                for(int i = 0; i < MAX_MESSAGES - 1; i++)
                {
                    strncpy(messages[i], messages[i + 1], sizeof(messages[0]));
                }
                strncpy(messages[MAX_MESSAGES - 1], formatted_message, sizeof(messages[0]));
            }

            pthread_mutex_unlock(&message_mutex);
            free(received_msg);    // Free the allocated memory for the message
        }
        else
        {
            LOG_ERROR("Error reading message from socket\n");
            break;
        }
    }

    return NULL;
}

// Thread function for receiving messages
/*void *receive_messages(void *arg)
{
    int  sockfd = *(int *)arg;
    char payload_buffer[BUFFER];

    while(1)
    {
        struct Message *received_msg = read_from_socket(sockfd, payload_buffer);
        if(received_msg)
        {
            pthread_mutex_lock(&message_mutex);

            // Add the payload to the messages array for display
            if(message_count < MAX_MESSAGES)
            {
                strncpy(messages[message_count++], payload_buffer, sizeof(payload_buffer));
            }

            pthread_mutex_unlock(&message_mutex);
            free(received_msg);    // Free the allocated memory for the message
        }
        else
        {
            LOG_ERROR("Error reading message from socket\n");
            break;
        }
    }
    return NULL;
}*/

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

void read_chat_message(const uint8_t *buffer, char *formatted_message, size_t max_len)
{
    const char *format = NULL;
    uint8_t     ts_len;
    uint8_t     content_len;
    uint8_t     username_len;
    char        time_str[TIME_SIZE];
    char        content[CONTENT_SIZE];
    char        username[NAME_SIZE];
    struct tm   tm_info;
    int         pos = MAX_HEADER_SIZE;    // Skip header

    // --- Process timestamp TLV ---
    pos++;    // Skip timestamp tag
    ts_len = buffer[pos++];
    if(ts_len >= TIME_SIZE)
    {
        LOG_ERROR("Timestamp length too long: %u\n", ts_len);
        snprintf(formatted_message, max_len, "Error: Invalid timestamp length.");
        return;
    }
    memcpy(time_str, &buffer[pos], ts_len);
    time_str[ts_len] = '\0';
    pos += ts_len;

    // Determine the timestamp format
    if(ts_len == GEN_TIM)
    {
        format = "%Y%m%d%H%M%SZ";
    }
    else if(ts_len == UTC_TIM)
    {
        format = "%y%m%d%H%M%SZ";
    }
    else
    {
        LOG_ERROR("Unexpected timestamp length: %u\n", ts_len);
        snprintf(formatted_message, max_len, "Error: Unexpected timestamp length.");
        return;
    }

    // Parse the timestamp
    memset(&tm_info, 0, sizeof(tm_info));
    if(strptime(time_str, format, &tm_info) == NULL)
    {
        LOG_ERROR("Failed to parse timestamp: %s with format %s\n", time_str, format);
        snprintf(formatted_message, max_len, "Error: Failed to parse timestamp.");
        return;
    }

    // --- Process content TLV ---
    pos++;    // Skip content tag
    content_len = buffer[pos++];
    memcpy(content, &buffer[pos], content_len);
    content[content_len] = '\0';
    pos += content_len;

    // --- Process username TLV ---
    pos++;    // Skip username tag
    username_len = buffer[pos++];
    memcpy(username, &buffer[pos], username_len);
    username[username_len] = '\0';

    // Format the final message
    snprintf(formatted_message, max_len, "[%s] %s: %s", time_str, username, content);
}
