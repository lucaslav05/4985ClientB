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

int main(void)
{
    struct box      chat_box;
    struct box      text_box;
    struct window   window_box;
    struct timespec ts;

    ts.tv_sec  = FIXED_UPDATE / NANO;
    ts.tv_nsec = FIXED_UPDATE % NANO;
    initscr();
    noecho();
    cbreak();

    open_console();    // open external console, good for when we have n curses

    LOG_MSG("GUI TEST STARTED\n");

    while(1)
    {
        draw_boxes(&window_box, &chat_box, &text_box);
        mvprintw(text_box.max_y - 2, 2, "Test");
        nanosleep(&ts, NULL);
    }
}
