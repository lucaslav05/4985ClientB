//
// Created by jonathan on 2/13/25.
//
#include "codec.h"

void decode_header(const uint8_t buf[], struct Message *msg)
{
    uint16_t copy;
    int      pos = 1;

    msg->packet_type = buf[0];
    LOG_MSG("Packet Type: %u\n", msg->packet_type);

    msg->protocol_version = buf[pos++];
    LOG_MSG("Protocol Version: %u\n", msg->protocol_version);

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->sender_id = ntohs(copy);
    LOG_MSG("Sender ID: %u\n", msg->sender_id);
    pos += (int)sizeof(uint16_t);

    memcpy(&copy, buf + pos, sizeof(uint16_t));
    msg->payload_length = ntohs(copy);
    LOG_MSG("Payload Length: %u\n", msg->payload_length);
}

void encode_header(uint8_t buf[], const struct Message *msg)
{
    uint16_t copy;
    int      pos   = 1;
    size_t   u16sz = sizeof(uint16_t);

    // Copy individual header fields into the buffer
    memcpy(buf, &msg->packet_type, 1);    // 1 byte packet type
    LOG_MSG("Packet Type: %u\n", msg->packet_type);

    memcpy(buf + pos, &msg->protocol_version, 1);    // 1 byte version
    LOG_MSG("Protocol Version: %u\n", msg->protocol_version);
    pos++;

    // Convert values to network byte order (big-endian)
    copy = htons(msg->sender_id);
    memcpy(buf + pos, &copy, u16sz);
    LOG_MSG("Sender ID: %u\n", msg->sender_id);
    pos += (int)u16sz;

    copy = htons(msg->payload_length);
    memcpy(buf + pos, &copy, u16sz);
    LOG_MSG("Payload Length: %u\n", msg->payload_length);
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

            LOG_MSG("Encoding UTF8STRING payload...\n");
            LOG_MSG("Username: %s, Password: %s\n", acc->username, acc->password);

            // Encode username field
            *ptr++ = (uint8_t)UTF8STRING;      // Tag
            *ptr++ = (uint8_t)username_len;    // Length
            memcpy(ptr, acc->username, username_len);
            ptr += username_len;

            // Encode password field
            *ptr++ = (uint8_t)UTF8STRING;      // Tag
            *ptr++ = (uint8_t)password_len;    // Length
            memcpy(ptr, acc->password, password_len);
            ptr += password_len;

            *payload_size = (size_t)(ptr - buffer);
            break;
        }
        case BOOLEAN:
            buffer[offset] = (*(const uint8_t *)data) ? 1 : 0;
            *payload_size  = offset + 1;
            LOG_MSG("Encoding BOOLEAN payload: %u\n", buffer[offset]);
            break;

        case INTEGER:
            net_value = htonl(*(const uint32_t *)data);
            memcpy(buffer + offset, &net_value, sizeof(uint32_t));
            *payload_size = offset + sizeof(uint32_t);
            LOG_MSG("Encoding INTEGER payload: %u\n", *(const uint32_t *)data);
            break;

        case SEQUENCE:
        case PRINTABLESTRING:
            src_len = strlen((const char *)data);
            strlcpy((char *)(buffer + offset), (const char *)data, src_len + 1);
            *payload_size = offset + src_len + 1;
            LOG_MSG("Encoding STRING payload: %s\n", (const char *)data);
            break;

        case NUL:
        case ENUMERATED:
        case UTCTIME:
        case GENERALIZEDTIME:
        default:
            LOG_ERROR("Unexpected tag type in switch: %u\n", tag);
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
        LOG_ERROR("Buffer too small to decode payload\n");
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
            LOG_ERROR("Unexpected tag type in switch: %u\n", *tag);
            break;

        case UTF8STRING:
        case PRINTABLESTRING:
        case SEQUENCE:
        {
            size_t data_len = strlen((char *)(buffer + offset));
            if(data_len >= BUFFER)
            {
                LOG_ERROR("String too long for buffer\n");
                return;
            }
            strlcpy((char *)data, (char *)(buffer + offset), BUFFER);
            break;
        }
        default:
            LOG_ERROR("Unknown tag type: %u\n", *tag);
            break;
    }
#pragma clang diagnostic pop
}*/
