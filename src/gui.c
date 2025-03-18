//
// Created by jonathan on 2/13/25.
//

#include "gui.h"
#include "clog.h"

void draw_boxes(struct window *window_box, struct box *chat_box, struct box *text_box)
{
    // Get current terminal size
    getmaxyx(stdscr, window_box->new_y, window_box->new_x);
    // LOG_MSG("window size %d:%d\n", window_box->new_x, window_box->new_y);

    // Check if the terminal size has changed
    if(window_box->new_x != window_box->old_x || window_box->new_y != window_box->old_y)
    {
        LOG_MSG("window size has changed from %d:%d to %d:%d \n", window_box->old_x, window_box->old_y, window_box->new_x, window_box->new_y);
        window_box->changed = true;
    }

    // Update the old dimensions to track changes
    window_box->old_x = window_box->new_x;
    window_box->old_y = window_box->new_y;

    if(chat_box->box)
    {
        delwin(chat_box->box);
    }
    if(text_box->box)
    {
        delwin(text_box->box);
    }

    // Calculate dimensions for the chat box (top half)
    chat_box->max_x = window_box->new_x - 1;    // Full width of the screen
    chat_box->max_y = window_box->new_y / 2;    // Top half height
    chat_box->min_x = 1;                        // Start at the left edge
    chat_box->min_y = 1;                        // Start at the top edge

    // Create the chat box window
    chat_box->box = newwin(chat_box->max_y - chat_box->min_y,    // Height of the chat box
                           chat_box->max_x - chat_box->min_x,    // Width of the chat box
                           chat_box->min_y,                      // Start row
                           chat_box->min_x                       // Start column
    );

    // Calculate dimensions for the text box (bottom half)
    text_box->max_x = window_box->new_x - 1;    // Full width of the screen
    text_box->max_y = window_box->new_y - 1;    // Bottom half height
    text_box->min_x = 1;                        // Start at the left edge
    text_box->min_y = window_box->new_y / 2;    // Start just below the chat box

    // Create the text box window
    text_box->box = newwin(text_box->max_y - text_box->min_y,    // Height of the text box
                           text_box->max_x - text_box->min_x,    // Width of the text box
                           text_box->min_y,                      // Start row
                           text_box->min_x                       // Start column
    );

    box(text_box->box, 0, 0);
    box(chat_box->box, 0, 0);
    wrefresh(text_box->box);
    wrefresh(chat_box->box);
    refresh();
    //  Clear the screen and redraw if the window has changed
    if(window_box->changed)
    {
        LOG_MSG("resizing window...\n");
        erase();
        window_box->changed = false;
    }
}
