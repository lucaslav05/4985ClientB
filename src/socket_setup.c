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
    int      pos          = 1;
    msg->packet_type      = buf[0];
    msg->protocol_version = buf[pos++];

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->sender_id = ntohs(copy);
    pos += sizeof(uint16_t);

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->payload_length = ntohs(copy);
}

void encode_header(uint8_t buf[], const struct Message *msg)
{
    uint16_t copy;
    int      pos   = 1;
    size_t   u16sz = sizeof(uint16_t);

    // Copy individual header fields into the buffer
    memcpy(buf, &msg->packet_type, 1);               // 1 byte packet type
    memcpy(buf + pos, &msg->protocol_version, 1);    // 1 byte version
    pos++;

    // Convert values to network byte order (big-endian)
    copy = htons(msg->sender_id);
    memcpy(buf + pos, &copy, u16sz);
    pos += (int)u16sz;

    copy = htons(msg->payload_length);
    memcpy(buf + pos, &copy, u16sz);
}

void encode_payload(uint8_t *buffer, Tags tag, const void *data, size_t *payload_size)
{
    uint32_t net_value;
    uint8_t *ptr = buffer;

    if(tag == SEQUENCE)
    {
        const struct ACC_Login *acc = (const struct ACC_Login *)data;

        // SEQUENCE header
        *ptr++ = (uint8_t)SEQUENCE;
        *ptr++ = (uint8_t)(strlen(acc->username) + strlen(acc->password) + 4);    // Total length

        // Encode Username as UTF8STRING
        *ptr++ = (uint8_t)UTF8STRING;
        *ptr++ = (uint8_t)strlen(acc->username);
        memcpy(ptr, acc->username, strlen(acc->username));
        ptr += strlen(acc->username);

        // Encode Password as UTF8STRING
        *ptr++ = (uint8_t)UTF8STRING;
        *ptr++ = (uint8_t)strlen(acc->password);
        memcpy(ptr, acc->password, strlen(acc->password));
        ptr += strlen(acc->password);

        *payload_size = (size_t)(ptr - buffer);
    }
    else
    {
        size_t  src_len;
        int32_t value;
        // Default encoding for other tags
        size_t         offset    = sizeof(uint32_t);
        uint32_t       tag_line  = htonl(tag);
        const uint8_t *byte_data = (const uint8_t *)data;
        memcpy(buffer, &tag_line, sizeof(uint32_t));

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcovered-switch-default"
        switch(tag)
        {
            case BOOLEAN:
                buffer[offset] = (*byte_data) ? 1 : 0;
                *payload_size  = offset + 1;
                break;

            case INTEGER:
                value     = *(const int32_t *)data;
                net_value = htonl((uint32_t)value);                       // Convert to unsigned before htonl
                memcpy(buffer + offset, &net_value, sizeof(uint32_t));
                *payload_size = offset + sizeof(uint32_t);
                break;

            case UTF8STRING:
            case PRINTABLESTRING:
                src_len = strlen((const char *)data);
                strlcpy((char *)(buffer + offset), (const char *)data, src_len + 1);
                *payload_size = offset + src_len + 1;
                break;

            case UTCTIME:
            case GENERALIZEDTIME:
            case NUL:           // Unused but explicitly handled
            case ENUMERATED:    // Unused but explicitly handled
            case SEQUENCE:      // Handled in the outer condition
                log_error("Unexpected tag type in switch: %u\n", tag);
                *payload_size = 0;
                break;
            default:
                *payload_size = 0;
                break;
        }
#pragma clang diagnostic pop
    }
}

void decode_payload(uint8_t *buffer, Tags *tag, void *data)
{
    size_t   offset;
    uint32_t temp;
    uint32_t tag_line;
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
            strlcpy((char *)data, (char *)(buffer + offset), BUFSIZE);
            break;

        default:
            log_error("Unknown tag type: %u\n", *tag);
            break;
    }
#pragma clang diagnostic pop
}

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
    uint8_t *buffer;
    size_t   total_size;
    Tags     tag;
    size_t   header_size          = MAX_HEADER_SIZE;    // Fixed header size (1 byte type, 1 byte version, 2 bytes sender_id, 2 bytes payload_length)
    size_t   encoded_payload_size = 0;
    size_t   encoding_buffer_size = payload_size + ENCODING_OVERHEAD;
    uint8_t *encoded_payload      = (uint8_t *)malloc(encoding_buffer_size);
    if(!encoded_payload)
    {
        log_error("Memory allocation failed for encoded payload\n");
        return;
    }

    // Determine the tag for encoding.
    // For example, if the message type is ACC_LOGIN, we use SEQUENCE.

    switch(msg->packet_type)
    {
        case ACC_LOGIN:
            tag = SEQUENCE;
            break;
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

    // Call encode_payload() to encode the payload into our temporary buffer.
    encode_payload(encoded_payload, tag, payload, &encoded_payload_size);

    // Now update the total size to be the header plus the encoded payload.
    total_size = header_size + encoded_payload_size;

    // Allocate the complete message buffer.
    buffer = (uint8_t *)malloc(total_size);
    if(!buffer)
    {
        log_error("Memory allocation failed for message buffer\n");
        free(encoded_payload);
        return;
    }

    msg->payload_length = (uint16_t)encoded_payload_size;

    // Encode the header into the buffer.
    encode_header(buffer, msg);

    // Copy the encoded payload into the buffer after the header.
    memcpy(buffer + header_size, encoded_payload, encoded_payload_size);

    // Send the complete message over the socket.
    sent_bytes = send(sockfd, buffer, total_size, 0);
    if(sent_bytes < 0)
    {
        log_error("Failed to send message\n");
    }
    else
    {
        log_msg("Message sent (size: %zu bytes)\n", total_size);
    }

    // Clean up allocated memory.
    free(buffer);
    free(encoded_payload);
}

struct Message read_from_socket(int sockfd, char *buffer)
{
    struct Message msg = {0};
    uint8_t        header_buf[MAX_HEADER_SIZE];

    // Read the message header
    ssize_t bytes_read = read(sockfd, header_buf, sizeof(header_buf));
    if(bytes_read < 0)
    {
        log_error("Failed to read message header\n");
        msg.packet_type = SYS_ERROR;
        return msg;
    }
    if((size_t)bytes_read < sizeof(header_buf))
    {
        log_error("Incomplete message header received\n");
        msg.packet_type = SYS_ERROR;
        return msg;
    }

    // Decode the header
    decode_header(header_buf, &msg);

    // Ensure the payload fits in the buffer
    if(msg.payload_length >= BUFSIZE)
    {
        log_error("Payload too large to fit in buffer\n");
        buffer[0]       = '\0';
        msg.packet_type = SYS_ERROR;
        return msg;
    }

    // Read the dynamic payload
    bytes_read = read(sockfd, buffer, msg.payload_length);
    if(bytes_read < 0)
    {
        log_error("Failed to read payload\n");
        msg.packet_type = SYS_ERROR;
        return msg;
    }

    buffer[msg.payload_length] = '\0';    // Null-terminate

    log_msg("Message received (size: %d bytes):\n", msg.payload_length);
    log_msg("Packet Type: %d\n", msg.packet_type);
    log_msg("Protocol Version: %d\n", msg.protocol_version);
    log_msg("Sender ID: %d\n", msg.sender_id);
    log_msg("Payload Length: %d\n", msg.payload_length);
    log_msg("Payload: %s\n", buffer);

    return msg;
}
