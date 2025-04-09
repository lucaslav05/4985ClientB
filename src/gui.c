//
// Created by jonathan on 2/13/25.
//

#include "gui.h"
#include "clog.h"

// Function to set up and draw the chat box
void draw_chat_box(const struct window *window_box, struct box *chat_box)
{
    if(chat_box->box)
    {
        delwin(chat_box->box);
    }

    // Calculate chat box dimensions
    chat_box->max_x = window_box->new_x;
    chat_box->max_y = window_box->new_y - 3;
    chat_box->min_x = 0;
    chat_box->min_y = 0;

    // Create chat box window
    chat_box->box = newwin(chat_box->max_y - chat_box->min_y, chat_box->max_x - chat_box->min_x, chat_box->min_y, chat_box->min_x);

    // Draw borders and mark for refresh
    box(chat_box->box, 0, 0);
    wnoutrefresh(chat_box->box);    // Optimized refreshing
}

// Function to set up and draw the text box
void draw_text_box(const struct window *window_box, struct box *text_box)
{
    if(text_box->box)
    {
        delwin(text_box->box);
    }

    // Calculate text box dimensions
    text_box->max_x = window_box->new_x;
    text_box->max_y = window_box->new_y;
    text_box->min_x = 0;
    text_box->min_y = window_box->new_y - 3;

    // Create text box window
    text_box->box = newwin(3, text_box->max_x - text_box->min_x, text_box->min_y, text_box->min_x);

    // Draw borders and mark for refresh
    box(text_box->box, 0, 0);
    wnoutrefresh(text_box->box);    // Optimized refreshing
}

// Main function to handle both boxes
void draw_boxes(struct window *window_box, struct box *chat_box, struct box *text_box)
{
    getmaxyx(stdscr, window_box->new_y, window_box->new_x);

    if(window_box->new_x != window_box->old_x || window_box->new_y != window_box->old_y)
    {
        LOG_MSG("Window size changed: %d:%d -> %d:%d\n", window_box->old_x, window_box->old_y, window_box->new_x, window_box->new_y);
        window_box->changed = true;
    }

    window_box->old_x = window_box->new_x;
    window_box->old_y = window_box->new_y;

    // Optimize refresh process
    if(window_box->changed)
    {
        LOG_MSG("Resizing window...\n");
        wclear(stdscr);    // Clear the screen more efficiently
        window_box->changed = false;
    }

    // Draw and prepare for efficient refresh updates
    draw_chat_box(window_box, chat_box);
    draw_text_box(window_box, text_box);

    doupdate();    // Batch updates for smoother rendering
}
