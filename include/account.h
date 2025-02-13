//
// Created by lucas-laviolette on 1/30/25.
//

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "clog.h"
#include "globals.h"
#include "message.h"
#include "socket_setup.h"
#include <netinet/in.h>

struct account
{
    char username[MAX_SIZE];
    char password[MAX_SIZE];
    int  uid;
};

void send_acc_login(const int *sockfd, const struct account *client);
void send_acc_create(const int *sockfd, const struct account *client);

#endif    // ACCOUNT_H
