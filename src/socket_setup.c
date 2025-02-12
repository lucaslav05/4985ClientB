//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

#define MAX_HEADER_SIZE 6

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
    size_t   header_size = MAX_HEADER_SIZE;    // Fixed header size (1 byte type, 1 byte version, 2 bytes sender_id, 2 bytes payload_len)
    size_t   total_size  = header_size + payload_size;
    uint8_t *buffer      = (uint8_t *)malloc(total_size);
    ssize_t  sent_bytes;

    if(!buffer)
    {
        log_error("Memory allocation failed\n");
        return;
    }

    // Set the correct payload length dynamically
    msg->payload_length = (uint16_t)payload_size;

    // Encode the header
    encode_header(buffer, msg);

    /* change the void pointer payload to something real */

    // Copy the payload after the header
    memcpy(buffer + header_size, payload, payload_size);

    // Send the encoded message
    sent_bytes = send(sockfd, buffer, total_size, 0);
    if(sent_bytes < 0)
    {
        log_error("Failed to send message\n");
    }
    else
    {
        log_msg("Message sent (size: %zu bytes)\n", total_size);
    }

    free(buffer);
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
