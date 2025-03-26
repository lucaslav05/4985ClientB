//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

int get_active_server_ip(char *buffer, const char *ipv4, uint16_t port)
{
    int                sockfd;
    struct sockaddr_in server_addr;
    unsigned char      request[MAX_IP_REQUEST_SIZE] = {CLIENT_GETIP, PROTOCOL_VERSION};
    unsigned char      response[MAX_SIZE];    // Buffer to handle variable length response
    ssize_t            bytes_received;

    // Create socket for server manager
    LOG_MSG("Creating socket for server manager...\n");
    if(create_socket(&sockfd) != 0)
    {
        return -1;
    }

    // Establish the connection to the server manager
    LOG_MSG("Connecting to the server manager...\n");
    if(connect_socket(sockfd, &server_addr, ipv4, port) != 0)
    {
        close(sockfd);
        return -1;
    }

    // Send request
    LOG_MSG("Sending IP request to the server manager...\n");
    if(send(sockfd, request, sizeof(request), 0) != sizeof(request))
    {
        LOG_ERROR("Request Active IP failed...\n");
        close(sockfd);
        return -1;
    }

    // Receive response
    LOG_MSG("Receving response from the server manager...\n");
    bytes_received = recv(sockfd, response, sizeof(response), 0);
    if(bytes_received <= 0)
    {
        LOG_ERROR("Receive response failed...\n");
        close(sockfd);
        return -1;
    }

    // Print the response
    print_response(response, bytes_received);

    // Handle response - return fd of new active server
    sockfd = handle_response(response, bytes_received, buffer, port);
    return sockfd;
}

void print_response(const unsigned char *response, ssize_t bytes_received)
{
    printf("Server Response (%zd bytes): ", bytes_received);

    for(ssize_t i = 0; i < bytes_received; i++)
    {
        printf("%02X ", response[i]);
        if((i + 1) % IP_MAX_CHARS == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

int create_socket(int *sockfd)
{
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        LOG_ERROR("Socket creation failed...\n");
        return -1;
    }
    LOG_MSG("Socket successfully created..\n");
    return 0;
}

int connect_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr(ipv4);
    serveraddr->sin_port        = htons(port);

    if(connect(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        LOG_ERROR("Connecting failed...\n");
        return -1;
    }

    LOG_MSG("Connecting successfully...\n");
    return 0;
}

int handle_response(const unsigned char *response, ssize_t bytes_received, char *ip_buffer, uint16_t port)
{
    int                sockfd;
    int                server_online;
    struct sockaddr_in server_addr;
    int                offset = 0;
    int                ip_length;
    int                port_length;
    char               port_str[MAX_HEADER_SIZE] = {0};

    // // Check the response size
    // if(bytes_received < offset + 3)
    // {
    //     LOG_ERROR("Invalid response size...\n");
    //     return -1;
    // }

    // Check the active of server manager
    if(response[offset] != MAN_RETURNIP)
    {
        LOG_ERROR("Server manager is not active now...\n");
        return -1;
    }
    offset += 1;

    // Check protocol version
    if(response[offset] != PROTOCOL_VERSION)
    {
        LOG_ERROR("Unsupported protocol version: 0x%02X...\n", response[offset]);
        return -1;
    }
    offset += 1;

    // Check server status
    server_online = response[offset];
    offset += 1;

    // If there's no active server
    if(server_online == 0x00)
    {
        ip_buffer[0] = '\0';
        LOG_MSG("No active IP and port was found...\n");
        return 0;
    }

    // If there's active server, get the IP address length
    ip_length = response[offset];
    offset += 1;

    // Check the length of IP
    if(ip_length + offset > bytes_received)
    {
        LOG_ERROR("Invalid IP length...\n");
        return -1;
    }

    memcpy(ip_buffer, &response[offset], (size_t)ip_length);
    ip_buffer[ip_length] = '\0';
    offset += ip_length;

    if(!is_valid_ip(ip_buffer))
    {
        LOG_ERROR("Invalid IP address: %s...\n", ip_buffer);
        return -1;
    }
    offset += 1;

    // Get the port length
    port_length = response[offset];
    offset += 1;

    // Check the length of port
    if(port_length + offset > bytes_received)
    {
        LOG_ERROR("Invalid Port length...\n");
        return -1;
    }

    memcpy(port_str, &response[offset], (size_t)port_length);
    port = convert_port(port_str);

    if(port == EXIT_FAILURE)
    {
        LOG_ERROR("Invalid port number: %s...\n", port_str);
        return -1;
    }

    // Create socket for server manager
    LOG_MSG("Creating socket for active manager...\n");
    if(create_socket(&sockfd) != 0)
    {
        return -1;
    }

    // Establish the connection to the server manager
    LOG_MSG("Connecting to the active server...\n");
    if(connect_socket(sockfd, &server_addr, ip_buffer, port) != 0)
    {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

uint16_t convert_port(const char *port_str)
{
    char         *endptr;
    unsigned long port_ulong;    // The port as an unsigned long

    if(port_str == NULL)
    {
        LOG_ERROR("Port is null...\n");
        exit(EXIT_FAILURE);
    }

    port_ulong = strtoul(port_str, &endptr, PORT_BASE);

    // TODO: error checking
    // see client version

    if(*endptr != '\0')
    {
        LOG_ERROR("Invalid Port format...\n");
        return EXIT_FAILURE;
    }

    if(port_ulong == UINT16_MAX)
    {
        LOG_ERROR("Port number is out of range...\n");
        return EXIT_FAILURE;
    }

    return (uint16_t)port_ulong;
}

bool is_valid_ip(const char *ip)
{
    const char *token;
    char       *saveptr;
    char        ip_copy[IP_MAX_CHARS];
    int         count;

    // Spit the IP into four parts.
    strlcpy(ip_copy, ip, IP_MAX_CHARS);
    count = 0;
    token = strtok_r(ip_copy, ".", &saveptr);
    while(token != NULL)
    {
        char *endptr;
        long  num;
        count++;
        num = strtol(token, &endptr, IP_BASE);

        if(*endptr != '\0' || num < 0 || num > IP_MAX)
        {
            return false;
        }

        token = strtok_r(NULL, ".", &saveptr);
    }

    // Return if the address has exactly four parts
    if(count != 4)
    {
        return false;
    }

    return true;
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
