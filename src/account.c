//
// Created by jonathan on 2/13/25.
//
#include "account.h"

void send_acc_login(const int *sockfd, const struct account *client)
{
    struct Message   msg;
    struct ACC_Login lgn;

    LOG_MSG("Allocating login info memory...\n");
    lgn.username = (char *)malloc(strlen(client->username) + 1);
    lgn.password = (char *)malloc(strlen(client->password) + 1);

    if(!lgn.username || !lgn.password)
    {
        LOG_ERROR("Login info allocation failed\n");
        goto cleanup;
    }

    LOG_MSG("Copying username and password to login info struct...\n");
    memcpy(lgn.username, client->username, strlen(client->username));
    memcpy(lgn.password, client->password, strlen(client->password));

    lgn.username[strlen(client->username)] = '\0';
    lgn.password[strlen(client->password)] = '\0';

    LOG_MSG("Setting up message structure for login...\n");
    msg.packet_type      = ACC_LOGIN;
    msg.protocol_version = 2;
    msg.sender_id        = 0;
    msg.payload_length   = htons((uint16_t)(TLV + strlen(lgn.username) + strlen(lgn.password)));

    LOG_MSG("Sending login message to socket...\n");
    write_to_socket(*sockfd, &msg, &lgn, (size_t)(TLV + strlen(lgn.username) + strlen(lgn.password)));

cleanup:
    // Free allocated memory before exiting
    LOG_MSG("Freeing allocated login info memory...\n");
    free(lgn.username);
    free(lgn.password);
}

void send_acc_create(const int *sockfd, const struct account *client)
{
    struct Message    msg;
    struct ACC_Create crt;

    LOG_MSG("Allocating account creation info memory...\n");
    crt.username = (char *)malloc(strlen(client->username) + 1);
    crt.password = (char *)malloc(strlen(client->password) + 1);

    if(!crt.username || !crt.password)
    {
        LOG_ERROR("Creation info allocation failed\n");
        goto cleanup;
    }

    LOG_MSG("Copying username and password to creation info struct...\n");
    strlcpy(crt.username, client->username, MAX_SIZE);
    strlcpy(crt.password, client->password, MAX_SIZE);

    LOG_MSG("Setting up message structure for account creation...\n");
    msg.packet_type      = ACC_CREATE;
    msg.protocol_version = 2;
    msg.sender_id        = 0;
    msg.payload_length   = (uint16_t)(strlen(crt.username) + strlen(crt.password));

    LOG_MSG("Sending account creation message to socket...\n");
    write_to_socket(*sockfd, &msg, &crt, sizeof(crt));

cleanup:
    // Free allocated memory before exiting
    LOG_MSG("Freeing allocated account creation info memory...\n");
    free(crt.username);
    free(crt.password);
}
