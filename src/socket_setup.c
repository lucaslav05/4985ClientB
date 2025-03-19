//
// Created by jonathan on 2/6/25.
//
#include "socket_setup.h"
#include "clog.h"

int get_active_server_ip(char *ip_buffer, const char *ipv4, uint16_t port)
{
    int                sockfd;
    int                serverfd;
    struct sockaddr_in server_addr;
    unsigned char      request[MAX_IP_REQUEST_SIZE] = {CLIENT_GETIP, PROTOCOL_VERSION};
    unsigned char      response[MAX_SIZE];    // Buffer to handle variable length response
    ssize_t            bytes_received;

    // Create socket
    LOG_MSG("Creating socket for server manager...\n");
    if(create_socket(&sockfd) != 0)
    {
        return -1;
    }

    // Use bind_socket to establish the connection
    LOG_MSG("Binding socket for server manager...\n");
    if(bind_socket(sockfd, &server_addr, ipv4, port) != 0)
    {
        close(sockfd);
        return -1;
    }

    // Send request
    LOG_MSG("Sending IP request...\n");
    if(send(sockfd, request, sizeof(request), 0) != sizeof(request))
    {
        LOG_ERROR("Request Active IP failed...\n");
        close(sockfd);
        return -1;
    }

    // Receive response
    LOG_MSG("Receving response...\n");
    bytes_received = recv(sockfd, response, sizeof(response), 0);
    if(bytes_received <= 0)
    {
        LOG_ERROR("Receive response failed...\n");
        close(sockfd);
        return -1;
    }

    // Handle response
    serverfd = handle_response(response, bytes_received, ip_buffer, port);
    if(serverfd != 0)
    {
        return -1;
    }

    return serverfd;
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

int bind_socket(int sockfd, struct sockaddr_in *serveraddr, const char *ipv4, uint16_t port)
{
    memset(serveraddr, 0, sizeof(*serveraddr));
    serveraddr->sin_family      = AF_INET;
    serveraddr->sin_addr.s_addr = inet_addr(ipv4);
    serveraddr->sin_port        = htons(port);

    if(bind(sockfd, (struct sockaddr *)serveraddr, sizeof(*serveraddr)) != 0)
    {
        LOG_ERROR("Connection with the server failed...\n");
        return -1;
    }

    LOG_MSG("Connected to the server..\n");
    return 0;
}

int handle_response(const unsigned char *response, ssize_t bytes_received, char *ip_buffer, uint16_t port)
{
    int                serverfd;
    struct sockaddr_in server_addr;
    int                server_online;
    int                ip_length;
    int                port_length;
    char               port_str[MAX_HEADER_SIZE] = {0};
    int                offset                    = 0;

    // Check the message format

    if(bytes_received < offset + 3 || response[offset] != BOOLEAN)
    {
        LOG_ERROR("Invalid message format...\n");
        return -1;
    }

    // Check the active server
    offset += 2;    // Skip tag (1 byte) and length (1 byte)
    if(response[offset] != MAN_RETURNIP)
    {    // Expected TRUE for active server (0x01)
        LOG_MSG("Server is not active now...\n");
        return -1;
    }
    offset += 1;    // Move to next field

    // Check the protocol version format
    if(bytes_received < offset + 3 || response[offset] != INTEGER)
    {
        LOG_ERROR("Invalid message format...\n");
        return -1;
    }
    offset += 2;    // Skip tag (1 byte) and length (1 byte)

    // Check the protocol version value
    if(response[offset] != PROTOCOL_VERSION)
    {    // Check protocol version
        LOG_MSG("Unsupported protocol version...\n");
        return -1;
    }
    offset += 1;    // Move to next field

    // Check the serverOnline format
    if(bytes_received < offset + 3 || response[offset] != BOOLEAN)
    {
        LOG_ERROR("Invalid serverOnline format...\n");
        return -1;
    }
    offset += 2;    // Skip tag (1 byte) and length (1 byte)

    // Check serverOnline value (0x01 for online, 0x00 for offline)
    server_online = response[offset];
    offset += 1;    // Move to next field

    if(server_online == 0x00)
    {
        // No active server, set IP buffer and port to empty values
        ip_buffer[0] = '\0';
        LOG_MSG("No active IP and port was found...\n");
        return 0;
    }

    // Check activeServerIp (UTF8STRING)
    if(bytes_received < offset + 3 || response[offset] != UTF8STRING)
    {
        LOG_ERROR("Invalid activeServerIp format...\n");
        return -1;
    }
    offset += 2;    // Skip tag (1 byte) and length (1 byte)

    // Get the length of the IP address
    ip_length = response[offset];
    if(ip_length + offset + 1 > bytes_received)
    {
        LOG_ERROR("Invalid IP length...\n");
        return -1;
    }
    offset += 1;                                                        // Move to next byte after length
    memcpy(ip_buffer, &response[(size_t)offset], (size_t)ip_length);    // Copy the IP string into ip_buffer
    ip_buffer[ip_length] = '\0';                                        // Null-terminate the IP string
    offset += ip_length;                                                // Move past the IP string

    // Validate IP address
    if(!is_valid_ip(ip_buffer))
    {
        LOG_ERROR("Invalid IP address: %s...\n", ip_buffer);
        return -1;
    }

    // Check activeServerPort (UTF8STRING)
    if(bytes_received < offset + 3 || response[offset] != UTF8STRING)
    {
        LOG_ERROR("Invalid activeServerPort format...\n");
        return -1;
    }
    offset += 2;    // Skip tag (1 byte) and length (1 byte)

    // Get the length of the port string
    port_length = response[offset];
    if(port_length + offset + 1 > bytes_received)
    {
        LOG_ERROR("Invalid Port length...\n");
        return -1;
    }
    offset += 1;                                                         // Move to next byte after length
    memcpy(port_str, &response[(size_t)offset], (size_t)port_length);    // Copy the port string
    port = convert_port(port_str);                                       // Convert the port string to a port number

    // Validate Port number
    if(port == EXIT_FAILURE)
    {
        LOG_ERROR("Invalid port number: %s...\n", port_str);
        return -1;
    }

    // Create socket
    LOG_MSG("Creating socket for server manager...\n");
    if(create_socket(&serverfd) != 0)
    {
        return -1;
    }

    // Use bind_socket to establish the connection
    LOG_MSG("Binding socket for server manager...\n");
    if(bind_socket(serverfd, &server_addr, ip_buffer, port) != 0)
    {
        close(serverfd);
        return -1;
    }

    return serverfd;    // Successfully parsed the response
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
