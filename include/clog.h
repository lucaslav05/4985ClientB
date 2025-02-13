//
// Created by jonathan on 2/6/25.
//

#ifndef CLIENT_CLOG_H
#define CLIENT_CLOG_H

#include "globals.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

void log_msg(const char *func, const char *format, ...) __attribute__((format(printf, 2, 3)));
void log_error(const char *func, const char *format, ...) __attribute__((format(printf, 2, 3)));
void log_payload_hex(const char *func, const uint8_t *payload, size_t size);
void open_console(void);

// Define macros for logging functions
#define LOG_MSG(...) log_msg(__func__, __VA_ARGS__)
#define LOG_ERROR(...) log_error(__func__, __VA_ARGS__)
#define LOG_PAYLOAD_HEX(payload, size) log_payload_hex(__func__, payload, size)

#endif    // CLIENT_CLOG_H
