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
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>    // Required for atomic operations
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __linux__
    #include <unistd.h>
#endif
static volatile sig_atomic_t logout_flag   = 0;                            // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static int                   message_count = 0;                            // NOLINT cppcoreguidelines-avoid-non-const-global-variables
static char                  messages[MAX_MESSAGES][BUFFER];               // NOLINT cppcoreguidelines-avoid-non-const-global-variables
static pthread_mutex_t       message_mutex = PTHREAD_MUTEX_INITIALIZER;    // NOLINT cppcoreguidelines-avoid-non-const-global-variables

void format_message(const uint8_t *buffer, char *output, size_t out_size);
void handle_sigint(int sig);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void handle_sigint(int sig)
{
    (void)sig;
    logout_flag = 1;
}

#pragma GCC diagnostic pop

void format_message(const uint8_t *buffer, char *output, size_t out_size)
{
    uint8_t     ts_len;
    uint8_t     content_len;
    uint8_t     username_len;
    char        content[CONTENT_SIZE];
    char        username[NAME_SIZE];
    char        time_str[TIME_SIZE];
    char        time_buf[TIME_BUFFER];    // Buffer to hold the formatted time string
    const char *format = NULL;
    struct tm   tm_info;
    // time_t      timestamp;
    int pos = 0;

    // --- Process timestamp TLV ---
    pos++;
    ts_len = buffer[pos++];    // Read timestamp length
    if(ts_len >= TIME_SIZE)
    {
        LOG_ERROR("Timestamp length too long: %u\n", ts_len);
        return;
    }
    memcpy(time_str, &buffer[pos], ts_len);
    time_str[ts_len] = '\0';
    pos += ts_len;

    // Choose format based on length.
    // For example:
    // 15 characters: "YYYYMMDDhhmmssZ" (GeneralizedTime)
    // 13 characters: "YYMMDDhhmmssZ" (UTCTime)
    if(ts_len == GEN_TIM)
    {
        format = "%Y%m%d%H%M%SZ";
    }
    else if(ts_len == UTC_TIM)
    {
        format = "%y%m%d%H%M%SZ";
    }
    else
    {
        LOG_ERROR("Unexpected timestamp length: %u\n", ts_len);
        snprintf(output, out_size, "Error: unexpected timestamp length");
        return;
    }

    memset(&tm_info, 0, sizeof(tm_info));
    if(strptime(time_str, format, &tm_info) == NULL)
    {
        LOG_ERROR("Failed to parse timestamp: %s with format %s\n", time_str, format);
        snprintf(output, out_size, "Error: invalid timestamp");
        return;
    }
    // timestamp = mktime(&tm_info);
    // Format the time into a human-readable string (e.g., "2025-01-30 12:34:56")
    if(strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info) == 0)
    {
        LOG_ERROR("Failed to format timestamp\n");
        snprintf(output, out_size, "Error: failed to format timestamp");
        return;
    }

    // --- Process content TLV ---
    pos++;                          // Skip content tag (expected tag: 0x0C)
    content_len = buffer[pos++];    // Read content length
    memcpy(content, &buffer[pos], content_len);
    content[content_len] = '\0';    // Null-terminate the content string
    pos += content_len;

    if(content_len == 0 || strlen(content) == 0)
    {
        strcpy(content, "null");
    }

    pos++;
    // --- Process username TLV ---
    // Skip username tag (expected tag: 0x0C)
    username_len = buffer[pos++];    // Read username length
    memcpy(username, &buffer[pos], username_len);
    username[username_len] = '\0';    // Null-terminate the username string

    // Format the final string as: [timestamp] username: content
    snprintf(output, out_size, "[%s] %s: %s", time_buf, username, content);
}

// Thread function for receiving messages
static void *receive_messages(void *arg)
{
    int     sockfd = *(int *)arg;
    uint8_t payload_buffer[BUFFER];

    while(1)
    {
        struct Message *received_msg;

        if(logout_flag)
        {
            break;
        }

        received_msg = read_from_socket(sockfd, (char *)payload_buffer);

        if(received_msg == NULL)
        {
            LOG_ERROR("Received null message, likely due to socket closure or error\n");
            handle_sigint(logout_flag);
            break;
        }

        if(received_msg->packet_type == CHT_SEND)
        {
            pthread_mutex_lock(&message_mutex);

            // Add the payload to the messages array for display
            if(message_count < MAX_MESSAGES)
            {
                if(received_msg->payload_length > 0 && received_msg->payload_length < BUFFER)
                {
                    strncpy(messages[message_count++], (char *)payload_buffer, sizeof(payload_buffer));
                }
                else
                {
                    LOG_ERROR("Received message too large: %d\n", received_msg->payload_length);
                }
            }

            pthread_mutex_unlock(&message_mutex);
            free(received_msg);    // Free the allocated memory for the message
        }
        else
        {
            LOG_ERROR("Error reading message from socket\n");
            break;
        }
    }

    return NULL;
}

int main(void)
{
    int              sockfd;
    struct Message  *response_msg;
    struct account   client;
    char             buffer[BUFFER];
    struct sigaction sa;
    // char               chat_input[BUFFER];
    struct timespec ts;
    struct box      chat_box;
    struct box      text_box;
    struct window   window_box;
    // char            messages[MAX_MESSAGES][BUFFER];    // List to store messages
    char input_buffer[BUFFER];
    int  input_index = 0;
    // int             message_count = 0;
    pthread_t recv_thread    = 0;
    int       payload_offset = 0;

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = handle_sigint;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        LOG_ERROR("Error: cannot handle SIGINT\n");
        exit(EXIT_FAILURE);
    }

    ts.tv_sec  = FIXED_UPDATE / NANO;
    ts.tv_nsec = FIXED_UPDATE % NANO;

    open_console();    // open external console, good for when we have n curses

    LOG_MSG("Ip Address: %s\n", IPV4);
    LOG_MSG("Port: %d\n", PORT);
    LOG_MSG("Buffer Size: %d\n", BUFFER);

    sockfd = get_active_server_ip(buffer, IPV4, PORT);
    if(sockfd <= 0)
    {
        exit(EXIT_FAILURE);
    }

    LOG_MSG("Prompting login...!\n");
    printf("Enter username: ");
    scanf("%49s", client.username);
    printf("Enter password: ");
    scanf("%49s", client.password);

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

            if(buffer[payload_offset] == 0x02)
            {
                uint8_t uid_len = (uint8_t)buffer[payload_offset + 1];
                if(uid_len == 2)
                {
                    // Read the user id from the following 2 bytes.
                    uint16_t uid_network_order;
                    memcpy(&uid_network_order, &buffer[payload_offset + 2], sizeof(uid_network_order));
                    client.uid = ntohs(uid_network_order);
                    LOG_MSG("Stored user id: %d\n", client.uid);
                }
                else
                {
                    LOG_ERROR("Unexpected user id length: %u\n", uid_len);
                }
            }
            else
            {
                LOG_ERROR("Unexpected payload tag: 0x%02X\n", (uint16_t)buffer[payload_offset]);
            }

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

                response_msg = read_from_socket(sockfd, buffer);

                while(!response_msg)
                {
                    nanosleep(&ts, NULL);
                    response_msg = read_from_socket(sockfd, buffer);
                }

                if(pthread_create(&recv_thread, NULL, receive_messages, &sockfd) != 0)
                {
                    LOG_ERROR("Failed to create receiving thread\n");
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                break;
            }
            if(response_msg->packet_type == SYS_ERROR)
            {
                endwin();
                LOG_ERROR("Error logging in.\n");
                handle_sigint(logout_flag);
            }
            LOG_ERROR("Account creation failed\n");
            break;

        default:
            LOG_ERROR("Unknown response from server: 0x%02X\n", response_msg->packet_type);
            break;
    }

    // --- Chat Loop ---
    // Allow the user to type messages until they type "logout".

    // printf("Enter message (or type 'logout' to exit): ");

    if(logout_flag == 0)
    {
        initscr();
        noecho();
        cbreak();
        nodelay(stdscr, TRUE);    // Enable non-blocking input
        keypad(stdscr, TRUE);     // Allow special key input (like KEY_RESIZE)
    }

    memset(input_buffer, 0, sizeof(input_buffer));    // Initialize the input buffer
    memset(messages, 0, sizeof(messages));

    while(1)
    {
        int             ch;    // For non-blocking character input
        struct Message  chat_msg;
        struct CHT_Send chat;

        if(logout_flag)
        {
            // Build and send logout message
            struct Message logout_msg;
            logout_msg.packet_type      = ACC_LOGOUT;
            logout_msg.protocol_version = 3;
            logout_msg.sender_id        = (uint16_t)client.uid;
            logout_msg.payload_length   = 0;

            LOG_MSG("Sending logout message due to SIGINT...\n");
            write_to_socket(sockfd, &logout_msg, NULL, 0);

            shutdown(sockfd, SHUT_RDWR);
            // Break out of loop to perform cleanup
            break;
        }

        draw_boxes(&window_box, &chat_box, &text_box);

        for(int i = 0; i < message_count; i++)
        {
            char formatted[BUFFER];
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

            if(logout_flag)
            {
                break;
            }

            format_message((const uint8_t *)messages[i], formatted, sizeof(formatted));
            // mvprintw(chat_box.min_y + 1 + i, chat_box.min_x + 2, "%s", messages[i]);
            mvprintw(chat_box.min_y + 1 + i, chat_box.min_x + 2, "%s", formatted);
            refresh();
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
                    // strncpy(messages[message_count++], input_buffer, sizeof(input_buffer));
                    LOG_MSG("User input received: %s\n", input_buffer);

                    // Build and send chat message to the server

                    chat_msg.packet_type      = CHT_SEND;
                    chat_msg.protocol_version = 3;
                    chat_msg.sender_id        = (uint16_t)client.uid;    // Replace with actual user ID

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
    }

    LOG_MSG("Username: %s\n", client.username);
    LOG_MSG("Password: %s\n", client.password);

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

    LOG_MSG("Cleaning up and exiting...\n");

    if(recv_thread)
    {
        pthread_join(recv_thread, NULL);
    }
    pthread_mutex_destroy(&message_mutex);
    clear();
    refresh();
    endwin();    // End `ncurses` mode
    close(sockfd);
    free(response_msg);
    return 0;
}
