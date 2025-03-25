//
// Created by jonathan on 2/13/25.
//

#include "gui.h"
#include "clog.h"

// Function to set up and draw the chat box
void draw_chat_box(const struct window *window_box, struct box *chat_box)
{
    // If a previous chat box exists, delete it
    if(chat_box->box)
    {
        delwin(chat_box->box);
    }

    // Calculate dimensions for the chat box
    chat_box->max_x = window_box->new_x;        // Full width of the screen
    chat_box->max_y = window_box->new_y - 3;    // Use remaining height
    chat_box->min_x = 0;                        // Start at the left edge
    chat_box->min_y = 0;                        // Start at the top edge

    // Create the chat box window
    chat_box->box = newwin(chat_box->max_y - chat_box->min_y,    // Height of the chat box
                           chat_box->max_x - chat_box->min_x,    // Width of the chat box
                           chat_box->min_y,                      // Start row
                           chat_box->min_x                       // Start column
    );

    // Draw borders and refresh
    box(chat_box->box, 0, 0);
    wrefresh(chat_box->box);
}

// Function to set up and draw the text box
void draw_text_box(const struct window *window_box, struct box *text_box)
{
    // If a previous text box exists, delete it
    if(text_box->box)
    {
        delwin(text_box->box);
    }

    // Calculate dimensions for the text box
    text_box->max_x = window_box->new_x;        // Full width of the screen
    text_box->max_y = window_box->new_y;        // Bottom 3 lines height
    text_box->min_x = 0;                        // Start at the left edge
    text_box->min_y = window_box->new_y - 3;    // Start 3 lines from the bottom

    // Create the text box window
    text_box->box = newwin(3,                                    // Height of the text box
                           text_box->max_x - text_box->min_x,    // Width of the text box
                           text_box->min_y,                      // Start row
                           text_box->min_x                       // Start column
    );

    // Draw borders and refresh
    box(text_box->box, 0, 0);
    wrefresh(text_box->box);
}

// Main function to handle both boxes
void draw_boxes(struct window *window_box, struct box *chat_box, struct box *text_box)
{
    // Get current terminal size
    getmaxyx(stdscr, window_box->new_y, window_box->new_x);

    // Check if the terminal size has changed
    if(window_box->new_x != window_box->old_x || window_box->new_y != window_box->old_y)
    {
        LOG_MSG("window size has changed from %d:%d to %d:%d \n", window_box->old_x, window_box->old_y, window_box->new_x, window_box->new_y);
        erase();
    }

    // Update the old dimensions to track changes
    window_box->old_x = window_box->new_x;
    window_box->old_y = window_box->new_y;

    // Draw boxes
    draw_chat_box(window_box, chat_box);
    draw_text_box(window_box, text_box);

    refresh();
}
