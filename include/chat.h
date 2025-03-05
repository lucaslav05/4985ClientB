//
// Created by lucas-laviolette on 3/4/25.
//

#ifndef CHAT_H
#define CHAT_H

#include "clog.h"
#include "globals.h"
#include "message.h"
#include "socket_setup.h"

void send_chat_message(int sockfd, const struct Message *msg, const struct CHT_Send *chat);

#endif    // CHAT_H
