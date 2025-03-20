//
// Created by jonathan on 2/13/25.
//

#ifndef CLIENT_GUI_H
#define CLIENT_GUI_H

#include <curses.h>

struct window
{
    int old_x;
    int old_y;
    int new_x;
    int new_y;
};

struct box
{
    WINDOW *box;
    int     max_x;
    int     min_x;
    int     max_y;
    int     min_y;
};

void draw_chat_box(const struct window *window_box, struct box *chat_box);
void draw_text_box(const struct window *window_box, struct box *text_box);
void draw_boxes(struct window *window_box, struct box *chat_box, struct box *text_box);

#endif    // CLIENT_GUI_H
