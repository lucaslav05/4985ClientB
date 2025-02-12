//
// Created by jonathan on 2/6/25.
//
#include "clog.h"

void open_console(void)
{
    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid == 0)
    {
        // Child process: open a new terminal
        execlp("gnome-terminal", "gnome-terminal", "--", "sh", "-c", "tail -f /tmp/latest.log", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process: open a log file for writing
        FILE *log_file = fopen("/tmp/latest.log", "we");
        if(log_file == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        if(setvbuf(log_file, NULL, _IOLBF, 0) != 0)
        {
            perror("setvbuf");
            fclose(log_file);
            exit(EXIT_FAILURE);
        }

        fclose(log_file);    // Close the file pointer to avoid leaks
    }
}

void log_msg(const char *format, ...)
{
    char             log_buf[BUFSIZE];
    char             time_buf[TIMEBUFSIZE];
    FILE            *log_file;
    time_t           now;
    struct tm        timeinfo;
    const struct tm *timeinfoptr;
    struct timeval   tv;
    va_list          args;

    // Get the current time
    gettimeofday(&tv, NULL);
    now         = tv.tv_sec;
    timeinfoptr = localtime_r(&now, &timeinfo);

    // Format the time as a string
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", timeinfoptr);
    snprintf(time_buf + strlen(time_buf), sizeof(time_buf) - strlen(time_buf), "::%03ld:%03ld", tv.tv_usec / DIVIDEND, tv.tv_usec % DIVIDEND);

    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);    // NOLINT(clang-analyzer-valist.Uninitialized)
    va_end(args);

    log_file = fopen("/tmp/latest.log", "ae");
    if(log_file != NULL)
    {
        fprintf(log_file, "[%s] [LOG] %s", time_buf, log_buf);
        fclose(log_file);
    }
}

void log_error(const char *format, ...)
{
    char             log_buf[BUFSIZE];
    char             time_buf[TIMEBUFSIZE];
    FILE            *log_file;
    time_t           now;
    struct tm        timeinfo;
    const struct tm *timeinfoptr;
    struct timeval   tv;
    va_list          args;

    // Get the current time
    gettimeofday(&tv, NULL);
    now         = tv.tv_sec;
    timeinfoptr = localtime_r(&now, &timeinfo);

    // Format the time as a string
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", timeinfoptr);
    snprintf(time_buf + strlen(time_buf), sizeof(time_buf) - strlen(time_buf), "::%03ld:%03ld", tv.tv_usec / DIVIDEND, tv.tv_usec % DIVIDEND);

    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);    // NOLINT(clang-analyzer-valist.Uninitialized)
    va_end(args);

    log_file = fopen("/tmp/latest.log", "ae");
    if(log_file != NULL)
    {
        fprintf(log_file, "[%s] [ERROR] %s", time_buf, log_buf);
        fclose(log_file);
    }
}

void log_payload_hex(const uint8_t *payload, size_t size)
{
    // Each byte requires 3 characters: 2 for the hex representation and 1 for space
    size_t hex_buf_size = 3 * size + 1;
    char  *hex_buf      = (char *)malloc(hex_buf_size);
    if(hex_buf == NULL)
    {
        log_error("Memory allocation failed for hex buffer\n");
        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        snprintf(&hex_buf[i * 3], 4, "%02x ", payload[i]);
    }

    log_msg("Encoded payload: %s\n", hex_buf);

    free(hex_buf);
}
