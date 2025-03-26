//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

int create_socket(int *sockfd)
{
    int reuse = 1;
    *sockfd   = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        LOG_ERROR("Socket creation failed...\n");
        return -1;
    }
    LOG_MSG("Socket successfully created..\n");

    // Enable socket reuse
    // Set reuse to true (1)
    if(setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        LOG_ERROR("Failed to set SO_REUSEADDR on socket...\n");
        return -1;
    }
    LOG_MSG("SO_REUSEADDR enabled for socket..\n");

    return 0;
}

int bind_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr(ipv4);
    serveraddr->sin_port        = htons(port);

    // Attempt connection to the server
    if(connect(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        LOG_ERROR("Connection with the server failed...\n");
        return -1;
    }

    LOG_MSG("Connected to the server..\n");
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
        LOG_ERROR("Memory allocation failed for encoded payload\n");
        goto cleanup;
    }

    LOG_MSG("Determining the tag for encoding...\n");
    // Determine the tag for encoding.
    // For example, if the message type is ACC_LOGIN, we use SEQUENCE.
    switch(msg->packet_type)
    {
        case ACC_LOGOUT:
            tag = NUL;
            break;
        case ACC_LOGIN:
        case SYS_SUCCESS:
        case SYS_ERROR:
        case ACC_LOGIN_SUCCESS:
        case ACC_CREATE:
        case ACC_EDIT:
            tag = UTF8STRING;
            break;
        default:
            tag = UTF8STRING;
    }

    LOG_MSG("Encoding the payload...\n");
    // Call encode_payload() to encode the payload into our temporary buffer.
    encode_payload(encoded_payload, tag, payload, &encoded_payload_size);

    if(tag == NUL)
    {
        encoded_payload_size = 0;
    }

    LOG_MSG("Updating the total size to include the header and the encoded payload...\n");
    // Now update the total size to be the header plus the encoded payload.
    total_size = header_size + encoded_payload_size;

    LOG_MSG("Allocating memory for the complete message buffer...\n");
    // Allocate the complete message buffer.
    buffer = (uint8_t *)malloc(total_size);
    if(!buffer)
    {
        LOG_ERROR("Memory allocation failed for message buffer\n");
        goto cleanup;
    }

    msg->payload_length = (uint16_t)encoded_payload_size;

    LOG_MSG("Encoding the header...\n");
    // Encode the header into the buffer.
    encode_header(buffer, msg);

    LOG_MSG("Copying the encoded payload into the buffer after the header...\n");
    // Copy the encoded payload into the buffer after the header.
    if(encoded_payload_size != 0)
    {
        memcpy(buffer + header_size, encoded_payload, encoded_payload_size);
    }

    // Log the original and encoded payloads.
    if(tag != NUL)
    {
        LOG_MSG("Payload: %.*s\n", (int)encoded_payload_size, encoded_payload);
        LOG_PAYLOAD_HEX(encoded_payload, encoded_payload_size);
    }

    LOG_MSG("Sending the complete message over the socket...\n");
    // Send the complete message over the socket.
    sent_bytes = send(sockfd, buffer, total_size, 0);
    if(sent_bytes < 0)
    {
        LOG_ERROR("Failed to send message\n");
        goto cleanup;
    }

    LOG_MSG("Message sent (size: %zu bytes)\n", total_size);

    // Clean up allocated memory.
cleanup:
    LOG_MSG("Cleaning up allocated memory...\n");
    free(buffer);
    free(encoded_payload);
}

struct Message *read_from_socket(int sockfd, char *buffer)
{
    struct Message *msg = (struct Message *)malloc(sizeof(struct Message));
    uint8_t         header_buf[MAX_HEADER_SIZE];
    ssize_t         bytes_read;
    size_t          total_read = 0;

    if(msg == NULL)
    {
        LOG_ERROR("Failed to allocate memory for Message\n");
        return NULL;
    }

    LOG_MSG("Reading the message header...\n");
    // Read the message header, handling partial reads
    while(total_read < sizeof(header_buf))
    {
        bytes_read = read(sockfd, header_buf + total_read, sizeof(header_buf) - total_read);
        if(bytes_read < 0)
        {
            LOG_ERROR("Failed to read message header\n");
            msg->packet_type = SYS_ERROR;
            goto cleanup;
        }
        else if(bytes_read == 0)
        {
            LOG_ERROR("Connection closed by server\n");
            msg->packet_type = SYS_ERROR;
            goto cleanup;
        }
        total_read += (size_t)bytes_read;
    }

    LOG_MSG("Decoding the header...\n");
    // Decode the header
    decode_header(header_buf, msg);

    LOG_MSG("Ensuring the payload fits in the buffer...\n");
    // Ensure the payload fits in the buffer
    if(msg->payload_length >= BUFFER)
    {
        LOG_ERROR("Payload too large to fit in buffer\n");
        buffer[0]        = '\0';
        msg->packet_type = SYS_ERROR;
        goto cleanup;
    }

    LOG_MSG("Reading the dynamic payload...\n");
    // Read the dynamic payload
    bytes_read = read(sockfd, buffer, msg->payload_length);
    if(bytes_read < 0)
    {
        LOG_ERROR("Failed to read payload\n");
        msg->packet_type = SYS_ERROR;
        goto cleanup;
    }

    buffer[msg->payload_length] = '\0';    // Null-terminate

    LOG_MSG("Message received (size: %d bytes):\n", msg->payload_length);
    LOG_MSG("Packet Type: %d\n", msg->packet_type);
    LOG_MSG("Protocol Version: %d\n", msg->protocol_version);
    LOG_MSG("Sender ID: %d\n", msg->sender_id);
    LOG_MSG("Payload Length: %d\n", msg->payload_length);
    LOG_MSG("Payload: %s\n", buffer);

    return msg;

cleanup:
    free(msg);
    return NULL;
}
