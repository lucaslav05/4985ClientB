//
// Created by jonathan on 3/18/25.
//
#include "gui.h"
#include "clog.h"
#include "globals.h"
#include <arpa/inet.h>
#include <curses.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MESSAGES 100
#define BACKSPACE 127
#define LOWERBOUND 32
#define UPPERBOUND 126

int main(void)
{
    struct box      chat_box;
    struct box      text_box;
    struct window   window_box;
    struct timespec ts;
    char            messages[MAX_MESSAGES][BUFFER];    // List to store messages
    char            input_buffer[BUFFER];
    int             input_index   = 0;
    int             message_count = 0;

    ts.tv_sec  = FIXED_UPDATE / NANO;
    ts.tv_nsec = FIXED_UPDATE % NANO;

    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);    // Enable non-blocking input
    keypad(stdscr, TRUE);     // Allow special key input (like KEY_RESIZE)

    open_console();    // Open external console, good for debugging
    LOG_MSG("GUI TEST STARTED\n");

    memset(input_buffer, 0, sizeof(input_buffer));    // Initialize the input buffer
    memset(messages, 0, sizeof(messages));

    while(1)
    {
        int ch;    // For non-blocking character input
        draw_boxes(&window_box, &chat_box, &text_box);

        for(int i = 0; i < message_count; i++)
        {
            if(message_count > chat_box.max_y - 2)
            {
                // Remove the oldest message by shifting all messages up
                for(int j = 0; j < message_count - 1; j++)
                {
                    strncpy(messages[j], messages[j + 1], sizeof(messages[j]));
                }
                message_count--;    // Decrease the message count

                if(i > 0)
                {
                    i--;    // Correctly adjust `i` for the outer loop
                }
            }

            mvprintw(chat_box.min_y + 1 + i, chat_box.min_x + 2, "%s", messages[i]);
        }

        // Display the input prompt in the text box
        mvprintw(text_box.max_y - 2, text_box.min_x + 2, "Type here: %s", input_buffer);
        curs_set(1);    // Enable cursor visibility

        // Read input without blocking
        ch = getch();
        if(ch != ERR)    // Check if a key was pressed
        {
            if(ch == KEY_RESIZE)    // Handle resize events
            {
                LOG_MSG("Resize detected\n");
                // clear();
                // memset(input_buffer, 0, sizeof(input_buffer));    // Optional: Clear buffer on resize
                // input_index = 0;
                draw_boxes(&window_box, &chat_box, &text_box);
            }
            else if(ch == '\n')    // Enter key pressed
            {
                if(message_count < MAX_MESSAGES)
                {    // Store message if there's room
                    strncpy(messages[message_count++], input_buffer, sizeof(input_buffer));
                    LOG_MSG("User input received: %s\n", input_buffer);
                }
                memset(input_buffer, 0, sizeof(input_buffer));    // Clear the buffer
                input_index = 0;
            }
            else if((ch == KEY_BACKSPACE || ch == BACKSPACE) && input_index > 0)    // Handle backspace
            {
                input_buffer[--input_index] = '\0';
            }
            else if((ch >= LOWERBOUND && ch <= UPPERBOUND) && input_index < (int)sizeof(input_buffer) - 1)    // Ignore escape sequences or invalid input
            {
                input_buffer[input_index++] = (char)ch;
            }
        }

        // Refresh the screen and continue updating dynamically
        refresh();
        nanosleep(&ts, NULL);    // Sleep to reduce CPU usage
    }
}
