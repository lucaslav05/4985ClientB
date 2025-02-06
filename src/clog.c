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
    char  log_buf[BUFSIZE];
    FILE *log_file;

    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);    // NOLINT(clang-analyzer-valist.Uninitialized)
    va_end(args);

    log_file = fopen("/tmp/latest.log", "ae");
    if(log_file != NULL)
    {
        fprintf(log_file, "[LOG] %s", log_buf);
        fclose(log_file);
    }
}

void log_error(const char *format, ...)
{
    char  log_buf[BUFSIZE];
    FILE *log_file;

    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);    // NOLINT(clang-analyzer-valist.Uninitialized)
    va_end(args);

    log_file = fopen("/tmp/latest.log", "ae");
    if(log_file != NULL)
    {
        fprintf(log_file, "[ERROR] %s", log_buf);
        fclose(log_file);
    }
}
