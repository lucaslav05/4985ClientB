//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

#define MAX_HEADER_SIZE 6
#define ENCODING_OVERHEAD 20

void decode_header(const uint8_t buf[], struct Message *msg)
{
    uint16_t copy;
    int      pos = 1;

    msg->packet_type = buf[0];
    log_msg("Packet Type: %u\n", msg->packet_type);

    msg->protocol_version = buf[pos++];
    log_msg("Protocol Version: %u\n", msg->protocol_version);

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->sender_id = ntohs(copy);
    log_msg("Sender ID: %u\n", msg->sender_id);
    pos += (int)sizeof(uint16_t);

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->payload_length = ntohs(copy);
    log_msg("Payload Length: %u\n", msg->payload_length);
}

void encode_header(uint8_t buf[], const struct Message *msg)
{
    uint16_t copy;
    int      pos   = 1;
    size_t   u16sz = sizeof(uint16_t);

    // Copy individual header fields into the buffer
    memcpy(buf, &msg->packet_type, 1);    // 1 byte packet type
    log_msg("Packet Type: %u\n", msg->packet_type);

    memcpy(buf + pos, &msg->protocol_version, 1);    // 1 byte version
    log_msg("Protocol Version: %u\n", msg->protocol_version);
    pos++;

    // Convert values to network byte order (big-endian)
    copy = htons(msg->sender_id);
    memcpy(buf + pos, &copy, u16sz);
    log_msg("Sender ID: %u\n", msg->sender_id);
    pos += (int)u16sz;

    copy = htons(msg->payload_length);
    memcpy(buf + pos, &copy, u16sz);
    log_msg("Payload Length: %u\n", msg->payload_length);
}

void encode_payload(uint8_t *buffer, Tags tag, const void *data, size_t *payload_size)
{
    uint32_t net_value;
    uint8_t *ptr = buffer;
    size_t   src_len;
    size_t   offset   = sizeof(uint32_t);
    uint32_t tag_line = htonl(tag);
    memcpy(buffer, &tag_line, sizeof(uint32_t));

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
    switch(tag)
    {
        case UTF8STRING:
        {
            const struct ACC_Login *acc          = (const struct ACC_Login *)data;
            size_t                  username_len = strlen(acc->username);
            size_t                  password_len = strlen(acc->password);
            size_t                  total_len    = username_len + password_len + 1;    // +1 for separator

            log_msg("Encoding UTF8STRING payload...\n");
            log_msg("Username: %s, Password: %s\n", acc->username, acc->password);

            // Encode tag and length
            *ptr++ = (uint8_t)UTF8STRING;
            *ptr++ = (uint8_t)total_len;

            // Copy username and password with a separator (e.g., ':')
            memcpy(ptr, acc->username, username_len);
            ptr += username_len;
            *ptr++ = ':';    // Separator
            memcpy(ptr, acc->password, password_len);
            ptr += password_len;

            *payload_size = (size_t)(ptr - buffer);
            break;
        }
        case BOOLEAN:
            buffer[offset] = (*(const uint8_t *)data) ? 1 : 0;
            *payload_size  = offset + 1;
            log_msg("Encoding BOOLEAN payload: %u\n", buffer[offset]);
            break;

        case INTEGER:
            net_value = htonl(*(const uint32_t *)data);
            memcpy(buffer + offset, &net_value, sizeof(uint32_t));
            *payload_size = offset + sizeof(uint32_t);
            log_msg("Encoding INTEGER payload: %u\n", *(const uint32_t *)data);
            break;

        case SEQUENCE:
        case PRINTABLESTRING:
            src_len = strlen((const char *)data);
            strlcpy((char *)(buffer + offset), (const char *)data, src_len + 1);
            *payload_size = offset + src_len + 1;
            log_msg("Encoding STRING payload: %s\n", (const char *)data);
            break;

        case NUL:
        case ENUMERATED:
        case UTCTIME:
        case GENERALIZEDTIME:
        default:
            log_error("Unexpected tag type in switch: %u\n", tag);
            *payload_size = 0;
            break;
    }
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
}

/*void decode_payload(uint8_t *buffer, Tags *tag, void *data)
{
    size_t   offset;
    uint32_t temp;
    uint32_t tag_line;
    if(sizeof(buffer) < sizeof(uint32_t))
    {
        log_error("Buffer too small to decode payload\n");
        return;
    }
    memcpy(&tag_line, buffer, sizeof(uint32_t));
    *tag = ntohl(tag_line);

    offset = sizeof(uint32_t);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcovered-switch-default"
    switch(*tag)
    {
        case BOOLEAN:
            *(bool *)data = buffer[offset] ? true : false;
            break;

        case INTEGER:
            memcpy(&temp, buffer + offset, sizeof(uint32_t));    // Safe copy with proper alignment
            *(int32_t *)data = (int32_t)ntohl(temp);             // Convert and assign
            break;

        case UTCTIME:
        case GENERALIZEDTIME:
        case NUL:           // Unused but explicitly handled
        case ENUMERATED:    // Unused but explicitly handled
            log_error("Unexpected tag type in switch: %u\n", *tag);
            break;

        case UTF8STRING:
        case PRINTABLESTRING:
        case SEQUENCE:
        {
            size_t data_len = strlen((char *)(buffer + offset));
            if(data_len >= BUFSIZE)
            {
                log_error("String too long for buffer\n");
                return;
            }
            strlcpy((char *)data, (char *)(buffer + offset), BUFSIZE);
            break;
        }
        default:
            log_error("Unknown tag type: %u\n", *tag);
            break;
    }
#pragma clang diagnostic pop
}*/

int create_socket(int *sockfd)
{
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        log_error("Socket creation failed...\n");
        return -1;
    }
    log_msg("Socket successfully created..\n");
    return 0;
}

int bind_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr(ipv4);
    serveraddr->sin_port        = htons(port);

    if(connect(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        log_error("Connection with the server failed...\n");
        return -1;
    }

    log_msg("Connected to the server..\n");
    return 0;
}

void write_to_socket(int sockfd, struct Message *msg, const void *payload, size_t payload_size)
{
    ssize_t  sent_bytes;
    size_t   total_size;
    Tags     tag;
    size_t   header_size          = MAX_HEADER_SIZE;    // Fixed header size (1 byte type, 1 byte version, 2 bytes sender_id, 2 bytes payload_length)
    size_t   encoded_payload_size = 0;
    size_t   encoding_buffer_size = payload_size + ENCODING_OVERHEAD;
    uint8_t *buffer               = NULL;
    uint8_t *encoded_payload      = (uint8_t *)malloc(encoding_buffer_size);
    if(!encoded_payload)
    {
        log_error("Memory allocation failed for encoded payload\n");
        goto cleanup;
    }

    log_msg("Determining the tag for encoding...\n");
    // Determine the tag for encoding.
    // For example, if the message type is ACC_LOGIN, we use SEQUENCE.
    switch(msg->packet_type)
    {
        case ACC_LOGIN:
        case SYS_SUCCESS:
        case SYS_ERROR:
        case ACC_LOGIN_SUCCESS:
        case ACC_LOGOUT:
        case ACC_CREATE:
        case ACC_EDIT:
            tag = UTF8STRING;
            break;
        default:
            tag = UTF8STRING;
    }

    log_msg("Encoding the payload...\n");
    // Call encode_payload() to encode the payload into our temporary buffer.
    encode_payload(encoded_payload, tag, payload, &encoded_payload_size);

    log_msg("Updating the total size to include the header and the encoded payload...\n");
    // Now update the total size to be the header plus the encoded payload.
    total_size = header_size + encoded_payload_size;

    log_msg("Allocating memory for the complete message buffer...\n");
    // Allocate the complete message buffer.
    buffer = (uint8_t *)malloc(total_size);
    if(!buffer)
    {
        log_error("Memory allocation failed for message buffer\n");
        goto cleanup;
    }

    msg->payload_length = (uint16_t)encoded_payload_size;

    log_msg("Encoding the header...\n");
    // Encode the header into the buffer.
    encode_header(buffer, msg);

    log_msg("Copying the encoded payload into the buffer after the header...\n");
    // Copy the encoded payload into the buffer after the header.
    memcpy(buffer + header_size, encoded_payload, encoded_payload_size);

    // Log the original and encoded payloads.
    log_msg("Original payload: %s\n", (const char *)payload);
    log_msg("Encoded  payload: %.*s\n", (int)encoded_payload_size, encoded_payload);

    log_msg("Sending the complete message over the socket...\n");
    // Send the complete message over the socket.
    sent_bytes = send(sockfd, buffer, total_size, 0);
    if(sent_bytes < 0)
    {
        log_error("Failed to send message\n");
        goto cleanup;
    }

    log_msg("Message sent (size: %zu bytes)\n", total_size);

    // Clean up allocated memory.
cleanup:
    log_msg("Cleaning up allocated memory...\n");
    free(buffer);
    free(encoded_payload);
}

struct Message *read_from_socket(int sockfd, char *buffer)
{
    struct Message *msg = (struct Message *)malloc(sizeof(struct Message));
    uint8_t         header_buf[MAX_HEADER_SIZE];
    ssize_t         bytes_read;

    if(msg == NULL)
    {
        log_error("Failed to allocate memory for Message\n");
        return NULL;
    }

    log_msg("Reading the message header...\n");
    // Read the message header
    bytes_read = read(sockfd, header_buf, sizeof(header_buf));
    if(bytes_read < 0)
    {
        log_error("Failed to read message header\n");
        msg->packet_type = SYS_ERROR;
        return msg;
    }
    if((size_t)bytes_read < sizeof(header_buf))
    {
        log_error("Incomplete message header received\n");
        msg->packet_type = SYS_ERROR;
        return msg;
    }

    log_msg("Decoding the header...\n");
    // Decode the header
    decode_header(header_buf, msg);

    log_msg("Ensuring the payload fits in the buffer...\n");
    // Ensure the payload fits in the buffer
    if(msg->payload_length >= BUFSIZE)
    {
        log_error("Payload too large to fit in buffer\n");
        buffer[0]        = '\0';
        msg->packet_type = SYS_ERROR;
        return msg;
    }

    log_msg("Reading the dynamic payload...\n");
    // Read the dynamic payload
    bytes_read = read(sockfd, buffer, msg->payload_length);
    if(bytes_read < 0)
    {
        log_error("Failed to read payload\n");
        msg->packet_type = SYS_ERROR;
        return msg;
    }

    buffer[msg->payload_length] = '\0';    // Null-terminate

    log_msg("Message received (size: %d bytes):\n", msg->payload_length);
    log_msg("Packet Type: %d\n", msg->packet_type);
    log_msg("Protocol Version: %d\n", msg->protocol_version);
    log_msg("Sender ID: %d\n", msg->sender_id);
    log_msg("Payload Length: %d\n", msg->payload_length);
    log_msg("Payload: %s\n", buffer);

    return msg;
}
