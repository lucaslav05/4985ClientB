//
// Created by lucas-laviolette on 1/30/25.
//

#include "account.h"
#include "chat.h"
#include "clog.h"
#include "globals.h"
#include "gui.h"
#include "message.h"
#include "socket_setup.h"
#include <arpa/inet.h>
#include <curses.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
    int                sockfd;
    struct sockaddr_in serveraddr;
    struct Message    *response_msg;
    struct account     client;
    char               buffer[BUFFER];
    struct timespec    ts;
    struct box         chat_box;
    struct box         text_box;
    struct window      window_box;
    char               input_buffer[BUFFER];
    int                input_index = 0;
    pthread_t          recv_thread;

    ts.tv_sec  = FIXED_UPDATE / NANO;
    ts.tv_nsec = FIXED_UPDATE % NANO;

    open_console();    // open external console, good for when we have n curses

    LOG_MSG("Ip Address: %s\n", IPV4);
    LOG_MSG("Port: %d\n", PORT);
    LOG_MSG("Buffer Size: %d\n", BUFFER);

    LOG_MSG("Creating socket...\n");
    if(create_socket(&sockfd) == -1)
    {
        LOG_ERROR("Creating socket failed\n");
        exit(EXIT_FAILURE);
    }

    LOG_MSG("Binding socket...\n");
    if(bind_socket(sockfd, &serveraddr, IPV4, PORT) == -1)
    {
        LOG_ERROR("Binding socket failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    LOG_MSG("Prompting login...!\n");
    printf("Enter username: ");
    scanf("%49s", client.username);
    strncpy(client.password, getpass("Enter password: "), MAX_SIZE);

    while(getchar() != '\n')
    {
    }

    LOG_MSG("Sending account login...\n");
    send_acc_login(&sockfd, &client);

    LOG_MSG("Reading response...\n");
    response_msg = read_from_socket(sockfd, buffer);

    while(!response_msg)
    {
        nanosleep(&ts, NULL);
        response_msg = read_from_socket(sockfd, buffer);
    }

    switch(response_msg->packet_type)
    {
        case ACC_LOGIN_SUCCESS:
            LOG_MSG("Successfully logged in.\n");
            // make new thread
            if(pthread_create(&recv_thread, NULL, receive_messages, &sockfd) != 0)
            {
                LOG_ERROR("Failed to create receiving thread\n");
                close(sockfd);
                return EXIT_FAILURE;
            }

            break;

        case SYS_ERROR:
            LOG_MSG("Error logging in.\n");
            LOG_MSG("Creating account...\n");
            send_acc_create(&sockfd, &client);
            LOG_MSG("Reading response...\n");
            response_msg = read_from_socket(sockfd, buffer);

            while(!response_msg)
            {
                nanosleep(&ts, NULL);
                response_msg = read_from_socket(sockfd, buffer);
            }
            if(response_msg->packet_type == SYS_SUCCESS)
            {
                LOG_MSG("Successfully created account\n");
                send_acc_login(&sockfd, &client);
                break;
            }
            LOG_ERROR("Account creation failed\n");
            break;

        default:
            LOG_ERROR("Unknown response from server: 0x%02X\n", response_msg->packet_type);
            break;
    }

    LOG_MSG("Username: %s\n", client.username);
    LOG_MSG("Password: %s\n", client.password);

    // --- Chat Loop ---
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);    // Enable non-blocking input
    keypad(stdscr, TRUE);     // Allow special key input (like KEY_RESIZE)

    memset(input_buffer, 0, sizeof(input_buffer));    // Initialize the input buffer
    memset(messages, 0, sizeof(messages));

    while(1)
    {
        int             ch;    // For non-blocking character input
        struct Message  chat_msg;
        struct CHT_Send chat;

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
            if(ch == KEY_RESIZE)
            {    // Handle resize events
                LOG_MSG("Resize detected\n");

                // Clear the screen
                clear();

                // Recalculate and redraw the boxes
                memset(input_buffer, 0, sizeof(input_buffer));    // Optional: Clear buffer on resize
                input_index = 0;
                draw_boxes(&window_box, &chat_box, &text_box);

                // Redraw existing messages
                pthread_mutex_lock(&message_mutex);    // Lock mutex before accessing messages[]
                for(int i = 0; i < message_count; i++)
                {
                    if(message_count > chat_box.max_y - 2)
                    {
                        // Adjust message count if overflow due to new dimensions
                        for(int j = 0; j < message_count - 1; j++)
                        {
                            strncpy(messages[j], messages[j + 1], sizeof(messages[j]));
                        }
                        message_count--;
                        if(i > 0)
                        {
                            i--;    // Correctly adjust `i` for the outer loop
                        }
                    }

                    mvprintw(chat_box.min_y + 1 + i, chat_box.min_x + 2, "%s", messages[i]);    // Reprint each message
                }
                pthread_mutex_unlock(&message_mutex);    // Unlock mutex after accessing messages[]

                refresh();    // Refresh the screen
            }

            else if(ch == '\n')
            {    // When the user presses Enter
                if(message_count < MAX_MESSAGES)
                {
                    strncpy(messages[message_count++], input_buffer, sizeof(input_buffer));
                    LOG_MSG("User input received: %s\n", input_buffer);

                    // Build and send chat message to the server

                    chat_msg.packet_type      = CHT_SEND;
                    chat_msg.protocol_version = 3;
                    chat_msg.sender_id        = 0;    // Replace with actual user ID

                    chat.timestamp = time(NULL);
                    chat.content   = input_buffer;
                    chat.username  = client.username;

                    send_chat_message(sockfd, &chat_msg, &chat);
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

        pthread_mutex_lock(&message_mutex);
        // Render messages in the chat box (read `messages[]`)
        pthread_mutex_unlock(&message_mutex);

        // Refresh the screen and continue updating dynamically
        refresh();
        nanosleep(&ts, NULL);
    }

    LOG_MSG("Cleaning up and exiting...\n");
    pthread_join(recv_thread, NULL);
    pthread_mutex_destroy(&message_mutex);
    endwin();    // End `ncurses` mode
    close(sockfd);
    free(response_msg);
}

/*while(1)
{
    struct Message  chat_msg;
    struct CHT_Send chat;
    fd_set          read_fds;
    int             max_fd;

FD_ZERO(&read_fds);
FD_SET(STDIN_FILENO, &read_fds);
FD_SET(sockfd, &read_fds);

max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

if(select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
{
    LOG_ERROR("Select failed\n");
    exit(EXIT_FAILURE);
}

if(FD_ISSET(STDIN_FILENO, &read_fds))
{
    printf("Enter message (or type 'logout' to exit): ");
    if(!fgets(chat_input, sizeof(chat_input), stdin))
    {
        break;    // Exit on input error or EOF
    }
    // Remove trailing newline
    chat_input[strcspn(chat_input, "\n")] = '\0';

    if(strcmp(chat_input, "logout") == 0)
    {
        // Build logout message.
        struct Message logout_msg;
        logout_msg.packet_type      = ACC_LOGOUT;
        logout_msg.protocol_version = 3;                       // protocol version used for logout (or update as needed)
        logout_msg.sender_id        = (uint16_t)client.uid;    // Replace with actual assigned ID if available
        logout_msg.payload_length   = 0;

        LOG_MSG("Sending logout message...\n");
        write_to_socket(sockfd, &logout_msg, NULL, 0);
        LOG_MSG("Logged out.\n");
        break;
    }
    // Build and send chat message.
    chat_msg.packet_type      = CHT_SEND;    // Use packet type defined for chat messages
    chat_msg.protocol_version = 3;
    chat_msg.sender_id        = 0;    // Replace with assigned user ID if available
    // payload_length will be set inside send_chat_message

    chat.timestamp = time(NULL);
    chat.content   = chat_input;
    chat.username  = client.username;

    send_chat_message(sockfd, &chat_msg, &chat);
}

if(FD_ISSET(sockfd, &read_fds))
{
    uint8_t readBuffer[BUFFER];
    ssize_t bytes_received;

    // --- Call read_chat_message to display received message ---
    bytes_received = recv(sockfd, readBuffer, sizeof(readBuffer), 0);

    if(bytes_received > 0)
    {
        read_chat_message(readBuffer);
    }
    else if(bytes_received == 0)
    {
        LOG_MSG("Server disconnected.\n");
        break;
    }
    else
    {
        LOG_ERROR("Failed to receive message from server\n");
        break;
    }
}
}*/
