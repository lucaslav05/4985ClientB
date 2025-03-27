//
// Created by lucas-laviolette on 3/4/25.
//

#ifndef CHAT_H
#define CHAT_H

#include "clog.h"
#include "globals.h"
#include "message.h"
#include "socket_setup.h"
#include <pthread.h>

static int             message_count = 0;                            // NOLINT cppcoreguidelines-avoid-non-const-global-variables
static char            messages[MAX_MESSAGES][BUFFER];               // NOLINT cppcoreguidelines-avoid-non-const-global-variables
static pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;    // NOLINT cppcoreguidelines-avoid-non-const-global-variables

void *receive_messages(void *arg);
void  send_chat_message(int sockfd, struct Message *msg, const struct CHT_Send *chat);
void  read_chat_message(const uint8_t *buffer, char *formatted_message, size_t max_len);

#endif    // CHAT_H
