//
// Created by jonathan on 2/6/25.
//

#ifndef CLIENT_CLOG_H
#define CLIENT_CLOG_H

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 1024
#define TIMEBUFSIZE 64
#define DIVIDEND 1000

void open_console(void);
void log_msg(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif    // CLIENT_CLOG_H
