//
// Created by jonathan on 2/6/25.
//

#ifndef CLIENT_CLOG_H
#define CLIENT_CLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#define BUFSIZE 1024

void open_console(void);
void log_msg(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif    // CLIENT_CLOG_H
