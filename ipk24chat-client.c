/**
 * ==========================================================
 * @author Valentyn Vorobec
 * login: xvorob02
 * email: xvorob02@stud.fit.vutbr.cz
 * date: 2024
 * ==========================================================
 */

#include "ipk24-chat-client.h"

/**
 * @brief Ctrl + C/Ctrl + D
 * 
 * @param signum 
 */
void handle_interrupt(int signum) 
{
    (void)signum;
    received_signal = 1;
}

/**
 * @brief Checks if the specified arguments have been specified
 * 
 * @param transfer_protocol udp/tcp
 * @param ip_addr IPv4
 * @param conf_timeout timeout for udp 
 * @param max_num_retransmissions maximum number of forwarding for udp
 */
void opt_arg_check(char *transfer_protocol, char *ip_addr) 
{
    if (transfer_protocol == NULL || ip_addr == NULL)
    {
        fprintf(stderr, "ERR: Use './ipk24-chat-client -h' for help!\n");
        exit(1);
    }
    if (strcmp(transfer_protocol, "udp") && strcmp(transfer_protocol, "tcp")) 
    {
        fprintf(stderr, "ERR: Unknown transport protocol: '%s'!\n", transfer_protocol);
        exit(1);
    }
}

/**
 * @brief prints help
 * 
 */
void print_help()
{
    printf("Usage: ./ipk24-chat-client -t <protocol> -s <IP address> -p <port> -d <number> -r <number> -h\n");
    printf("\n");
    printf("Argument    | Value         | Possible values	        | Meaning or expected program behaviour\n");
    printf("--------------------------------------------------------------------------------------------------\n");
    printf("-t          | User provided | tcp or udp                | Transport protocol used for connection\n");
    printf("-s          | User provided | IP address or hostname    | Server IP or hostname\n");
    printf("-p          | 4567          | uint16	                | Server port\n");
    printf("-d          | 250           | uint16	                | UDP confirmation timeout\n");
    printf("-r          | 3	            | uint8                     | Maximum number of UDP retransmissions\n");
    printf("-h          | 	            |                           | Prints program help output and exits\n\n");
}

/**
 * @brief Check if the client's input starts with a command.
 * 
 * @param input client input
 * @return int number, depending on how the input starts, 5 = invalid command
 */
int check_input(char *input)
{
    if (!strncmp(input, "/", strlen("/")))
    {
        if (!strncmp(input, "/auth", strlen("/auth"))) return 1;
        else if (!strncmp(input, "/join", strlen("/join"))) return 2;
        else if (!strncmp(input, "/rename", strlen("/rename"))) return 3;
        else if (!strncmp(input, "/help", strlen("/help"))) return 4;
        else return 5;
    }
    else return 6;
}

/**
 * @brief This function checks the response from the server and decides what came. 
 * It uses functions like tcp_check_reply and if any of them return 0, 
 * it returns a Response value. They always have to copy the response to resp and then work with it, 
 * because there happens to be deletion of the value using strtok.
 * 
 * @param response response from the server
 * @param resp auxiliary copy of the response to work with
 * @param resp_succ this value is either supplied with DisplayName or OK|NOK, depending on what the message is from the server
 * @param message message
 * @return enum Response represents what came from the server, INT_ERR represents an allocation error
 */
enum Response check_response(char *response, char **resp, char **resp_succ, char **message)
{
    enum Response resp_code = UKNOWN;

    *resp = strdup(response);
    if (*resp == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return INT_ERR;
    }

    if (!tcp_check_err(*resp, resp_succ, message)) 
        resp_code = ERR;
    else
    {
        if (*resp_succ != NULL) free(*resp_succ);
        if (*message != NULL) free(*message);
        *resp_succ = NULL;
        *message = NULL;
        free(*resp);
        *resp = strdup(response);
        if (*resp == NULL)
        {
            fprintf(stderr, "ERR: Memory allocation failed!\n");
            return INT_ERR;
        }
        if (!tcp_check_reply(*resp, resp_succ, message))
        {
            if (!strcasecmp(*resp_succ, "OK")) resp_code = OK;
            else resp_code = NOK;
        }
        else 
        {
            if (*resp_succ != NULL) free(*resp_succ);
            if (*message != NULL) free(*message);
            *resp_succ = NULL;
            *message = NULL;
            free(*resp);
            *resp = strdup(response);
            if (*resp == NULL)
            {
                fprintf(stderr, "ERR: Memory allocation failed!\n");
                return INT_ERR;
            }
            if (!tcp_check_msg(*resp, resp_succ, message))
                resp_code = MSG;
            else
            {
                if (*resp_succ != NULL) free(*resp_succ);
                if (*message != NULL) free(*message);
                *resp_succ = NULL;
                *message = NULL;
                free(*resp);
                *resp = strdup(response);
                if (*resp == NULL)
                {
                    fprintf(stderr, "ERR: Memory allocation failed!\n");
                    return INT_ERR;
                }
                if (!tcp_check_bye(*resp))
                {
                    resp_code = BYE;
                    if (*resp_succ != NULL) free(*resp_succ);
                    if (*message != NULL) free(*message);
                    if (*resp != NULL) free(*resp);
                    *resp_succ = NULL;
                    *message = NULL;
                    *resp = NULL;
                }
                else resp_code = UKNOWN;
            }
        }
    }

    return resp_code;
}

/**
 * @brief It checks the response from the server and decides the next state. 
 * In the event of an error, free all pointers, close the socket and terminate the program.
 * 
 * @param response the message from the server
 * @param display_name the client's display name
 * @param buff final client response
 * @param state current state
 * @param proccessing decides whether or not the client can currently send more messages
 * @param client_socket the socket
 * @return int next state
 */
int recv_next_state(char *response, char **display_name, char **buff, int state, int *proccessing, int client_socket)
{
    int current_state = state;
    char *message = NULL;
    char *resp_succ = NULL;
    char *resp = NULL;

    enum Response resp_code = check_response(response, &resp, &resp_succ, &message);
    
    if (resp_code == INT_ERR)
    {
        if (*buff != NULL) free(*buff);
        if (*display_name != NULL) free(*display_name);
        close(client_socket);
        exit(1);
    }

    switch (current_state)
    {
        // START - waiting for the REPLY response here
        case 1:
            if (resp_code == ERR)
            {
                current_state = 4;
                *proccessing = 1;
                fprintf(stderr, "ERR FROM %s: %s\n", resp_succ, message);
                if (content_bye(buff))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
            }
            else if (resp_code == OK)
            {
                current_state = 2;
                fprintf(stderr, "Success: %s\n", message);
            }
            else if (resp_code == NOK)
            {
                fprintf(stderr, "Failure: %s\n", message);
            }
            else if (resp_code == UKNOWN)
            {
                fprintf(stderr, "ERR: Unrecognized message from server!\n");
                if (content_err(buff, *display_name, "Unrecognized message from server!"))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
                *proccessing = 1;
                current_state = 3;
            }
            else
            {
                if (*buff != NULL) free(*buff);
                if (*display_name != NULL) free(*display_name);
                close(client_socket);
                exit(1);
            }
            break;
        // OPEN - messages can be received here
        case 2:
            if (resp_code == ERR)
            {
                current_state = 4;
                *proccessing = 1;
                fprintf(stderr, "ERR FROM %s: %s\n", resp_succ, message);
                if (content_bye(buff))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
            }
            else if (resp_code == MSG)
            {
                fprintf(stdout, "%s: %s\n", resp_succ, message);
                fflush(stdout);
            }
            else if (resp_code == BYE)
            {
                *proccessing = 1;
                current_state = 4;
            }
            else if (resp_code == UKNOWN)
            {
                fprintf(stderr, "ERR: Unrecognized message from server!\n");
                if (content_err(buff, *display_name, "Unrecognized message from server!"))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
                *proccessing = 1;
                current_state = 3;
            }
            break;
        case 5:
            if (resp_code == ERR)
            {
                current_state = 4;
                *proccessing = 1;
                fprintf(stderr, "ERR FROM %s: %s\n", resp_succ, message);
                if (content_bye(buff))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
            }
            else if (resp_code == OK)
            {
                current_state = 2;
                fprintf(stderr, "Success: %s\n", message);
            }
            else if (resp_code == NOK)
            {
                current_state = 2;
                fprintf(stderr, "Failure: %s\n", message);
            }
            else if (resp_code == MSG)
            {
                *proccessing = 1;
                fprintf(stdout, "%s: %s\n", resp_succ, message);
                fflush(stdout);
            }
            else if (resp_code == BYE)
            {
                *proccessing = 1;
                current_state = 4;
            }
            else if (resp_code == UKNOWN)
            {
                fprintf(stderr, "ERR: Unrecognized message from server!\n");
                if (content_err(buff, *display_name, "Unrecognized message from server!"))
                {
                    if (*buff != NULL) free(*buff);
                    if (*display_name != NULL) free(*display_name);
                    close(client_socket);
                    exit(1);
                }
                *proccessing = 1;
                current_state = 3;
            }
            else
            {
                if (*buff != NULL) free(*buff);
                if (*display_name != NULL) free(*display_name);
                close(client_socket);
                exit(1);
            }
            break;
        default:
            break; 
    }

    if (resp_code != ERR)
    {
        if (resp_succ != NULL) free(resp_succ);
        if (message != NULL) free(message);
    }
    if (resp != NULL) free(resp);
    return current_state;
}

/**
 * @brief Correctlly exit program, close socket and free memmory.
 * 
 * @param buff 
 * @param display_name 
 * @param buff_confirm 
 * @param head udp id history
 * @param fifo udp fifo
 * @param client_socket 
 * @param server_info IPv4
 * @param exit_code 
 */
void udp_exit(char **buff, char **display_name, char **buff_confirm, struct Node **head, struct ipk_list **fifo, int client_socket, struct addrinfo **server_info, int exit_code)
{
    if (*buff != NULL) free(*buff);
    if (*display_name != NULL) free(*display_name);
    if (*buff_confirm != NULL) free(*buff_confirm);
    *buff = NULL;
    *display_name = NULL;
    *buff_confirm = NULL;
    free_list(*head);
    free_fifo(*fifo);
    close(client_socket);
    freeaddrinfo(*server_info);
    exit(exit_code);
}

/**
 * @brief Resets variables after CONFIRM from server
 * 
 * @param id_conf 
 * @param buff 
 * @param retransmition 
 * @param max_num_retransmissions 
 * @param timeout 
 * @param current_state 
 * @param next_state 
 */
void udp_conf(int *id_conf, char ** buff, int *retransmition, int max_num_retransmissions, int *timeout, enum State *current_state, enum State next_state)
{
    *id_conf = -1;
    if (*buff != NULL) free(*buff);
    *buff = NULL;
    *retransmition = max_num_retransmissions;
    *timeout = -1;
    *current_state = next_state;
}

/**
 * @brief Connects to the server socket. Then in while, whichever is the current state,
 * thus decides what will be done with the input from the client or the response from the server
 * 
 * @param host ip or domain name
 * @param port the port
 */
void tcp(char *host, char *port) 
{
    struct addrinfo *server_info;
    struct addrinfo *p;
    struct addrinfo hints;
    int client_socket;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if (getaddrinfo(host, port, &hints, &server_info) != 0)
    {
        fprintf(stderr, "ERR: Failed to get address info for %s!\n", host);
        exit(1);
    }

    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((client_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            fprintf(stderr, "ERR: Socket creation!\n");
            continue;
        }

        if (connect(client_socket, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(client_socket);
            fprintf(stderr, "ERR: Socket connection!\n");
            continue;
        }

        struct timeval timeval = {.tv_sec = 5};

        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0)
        {
            fprintf(stderr, "ERR: Setsockopt!\n");
            freeaddrinfo(server_info);
            exit(1);
        }
        
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "ERR: Failed to connect to %s!\n", host);
        freeaddrinfo(server_info);
        exit(1);
    }

    freeaddrinfo(server_info);
    
    struct sigaction sa;
    sa.sa_handler = handle_interrupt;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "ERR: Setting up signal handler!\n");
        exit(1);
    }

    // poll setting
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client_socket;
    fds[1].events = POLLIN;

    int proccessing = 0;        // when 1, blocks the client from writing messages (currently being processed)
    int current_state = 1;      // a variable that represents the current state
    char *display_name = NULL;  // the name under which messages are written

    while(1)
    {
        char *buff = NULL;
        // a nonsense message came from the server and an ERR was sent, so just send BYE
        if (current_state == 3)
        {
            current_state = 4;
            proccessing = 1;
            if (content_bye(&buff))
            {
                if (buff != NULL) free(buff);
                close(client_socket);
                exit(1);
            }
        }
        else
        {
            int ret = poll(fds, 2, -1);

            // if true, then Ctrl + C was recorded, send BYE and go to exit state
            if (received_signal)
            {
                current_state = 4;
                proccessing = 1;
                if (content_bye(&buff))
                {
                    if (buff != NULL) free(buff);
                    if (display_name != NULL) free(display_name);
                    close(client_socket);
                    exit(1);
                }
            }
            else
            {
                if (ret < 0)    // poll error
                {
                    fprintf(stderr, "ERR: poll!\n");
                    if (buff != NULL) free(buff);
                    close(client_socket);
                    exit(1);
                }

                // messages have arrived from the server
                if (fds[1].revents & POLLIN) 
                {
                    char response[MAX_MESSAGE_SIZE] = {0};
                    ssize_t recv_result = recv(client_socket, response, sizeof(response), 0);
                    if (recv_result < 0) 
                    {
                        fprintf(stderr, "ERR: Can't receive message!\n");
                        if (buff != NULL) free(buff);
                        if (display_name != NULL) free(display_name);
                        close(client_socket);
                        exit(1);
                    } 
                    else if (recv_result == 0)
                    {
                        current_state = 4;
                        proccessing = 1;
                        if (content_bye(&buff))
                        {
                            if (buff != NULL) free(buff);
                            if (display_name != NULL) free(display_name);
                            close(client_socket);
                            exit(1);
                        }
                    }
                    proccessing = 0;
                    current_state = recv_next_state(response, &display_name, &buff, current_state, &proccessing, client_socket);
                }
            }

            // the user entered something into the console, it is allowed but only when something is not being processed
            if (!proccessing)
                if (fds[0].revents & POLLIN) 
                {
                    char input[1400];
                    if (fgets(input, sizeof(input), stdin) != NULL) 
                    {
                        if (input[strlen(input) - 1] == '\n') 
                            input[strlen(input) - 1] = '\0';
                        int input_code = check_input(input);
                        
                        switch (current_state)
                        {
                        case 1:
                            if (input_code == 1)    // AUTH
                            {
                                char *token = strtok(input, " ");
                                char *param1 = NULL;
                                char *param2 = NULL;
                                char *param3 = NULL;

                                if (token != NULL)
                                {
                                    param1 = strtok(NULL, " ");
                                    param2 = strtok(NULL, " ");
                                    param3 = strtok(NULL, " ");
                                }

                                if (param1 == NULL || param2 == NULL || param3 == NULL)
                                {
                                    fprintf(stderr, "ERR: Parameters do not match!\n");
                                    continue;
                                }

                                int message_code = content_auth(&buff, param1, param3, param2);
                                if (message_code)
                                {
                                    if (buff != NULL) free(buff);
                                    if (display_name != NULL) free(display_name);
                                    close(client_socket);
                                    exit(1);
                                }

                                if (display_name != NULL) free(display_name);
                                display_name = strdup(param3);
                                if (display_name == NULL)
                                {
                                    fprintf(stderr, "ERR: Memory allocation failed!\n");
                                    if (buff != NULL) free(buff);
                                    close(client_socket);
                                    exit(1);
                                }
                                proccessing = 1;
                            }
                            else if (input_code == 4)   // HELP
                            {
                                print_help();
                                continue;
                            }
                            else
                            {
                                fprintf(stderr, "ERR: You are not authenticated!\n");
                                continue;
                            }
                            break; 
                        case 2:
                            if (input_code == 1)    // AUTH - but already logged in
                            {
                                fprintf(stderr, "ERR: Already authenticated!\n");
                                continue;
                            }
                            else if (input_code == 2)   // JOIN
                            {
                                char *token = strtok(input, " ");
                                char *param1 = NULL;

                                if (token != NULL) param1 = strtok(NULL, " ");

                                if (param1 == NULL)
                                {
                                    fprintf(stderr, "ERR: Parameter's number does not match!\n");
                                    continue;
                                }
                                int message_code = content_join(&buff, display_name, param1);
                                proccessing = 1;
                                if (message_code)
                                {
                                    if (buff != NULL) free(buff);
                                    if (display_name != NULL) free(display_name);
                                    close(client_socket);
                                    exit(1);
                                }
                                current_state = 5;
                            }
                            else if (input_code == 3)   // RENAME
                            {
                                char *token = strtok(input, " ");
                                char *param1 = NULL;

                                if (token != NULL) param1 = strtok(NULL, " ");

                                if (param1 == NULL)
                                {
                                    fprintf(stderr, "ERR: Parameter's number does not match!\n");
                                    continue;
                                }

                                if (display_name != NULL) free(display_name);
                                display_name = strdup(param1);
                                if (display_name == NULL)
                                {
                                    fprintf(stderr, "ERR: Memory allocation failed!\n");
                                    if (buff != NULL) free(buff);
                                    close(client_socket);
                                    exit(1);
                                }
                                continue;
                            }
                            else if (input_code == 4)   // HELP
                            {
                                print_help();
                                continue;
                            }
                            else if (input_code == 6)   // MSG
                            {
                                int message_code = content_message(&buff, display_name, input);
                                if (message_code)
                                {
                                    if (buff != NULL) free(buff);
                                    if (display_name != NULL) free(display_name);
                                    close(client_socket);
                                    exit(1);
                                }
                            }
                            else
                            {
                                fprintf(stderr, "ERR: Unknown command!\n\b");
                                continue;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    if (feof(stdin))
                    {
                        current_state = 4;
                        proccessing = 1;
                        if (content_bye(&buff))
                        {
                            if (buff != NULL) free(buff);
                            if (display_name != NULL) free(display_name);
                            close(client_socket);
                            exit(1);
                        }
                    }
                }
        }

        // if there is something in the buff, send it and release it
        if (buff != NULL)
        {
            if (send(client_socket, buff, strlen(buff), 0) < 0) 
            {
                fprintf(stderr, "ERR: Can't send message!\n");
                free(buff);
                if (display_name != NULL) free(display_name);
                close(client_socket);
                exit(1);
            }

            free(buff);
            buff = NULL;
        }

        // came BYE, that's it
        if (current_state == 4)
        {
            if (buff != NULL) free(buff);
            if (display_name != NULL) free(display_name);
            close(client_socket);
            exit(0);
        }
    }
}

/**
 * @brief Connects to the server socket. Then in while, whichever is the current state,
 * thus decides what will be done with the input from the client or the response from the server
 *
 * @param host ip or domain name
 * @param port the port
 * @param conf_timeout time to wait for CONFIRM, default is 250ms
 * @param max_num_retransmissions maximum number of message retransmissions, default is 3 attempts
 */
void udp(char *host, char *port, int conf_timeout, int max_num_retransmissions)
{
    struct addrinfo *server_info;
    struct addrinfo hints;
    int client_socket;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;

    int status = getaddrinfo(host, port, &hints, &server_info);

    if (status != 0)
    {
        fprintf(stderr, "ERR: getaddrinfo: failed to resolve hostname!\n");
        exit(1);
    }

    if ((client_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) <= 0)
    {
        fprintf(stderr, "ERR: Socket creation!\n");
        freeaddrinfo(server_info);
        exit(1);
    }

    struct timeval timeval = {.tv_sec = 2};

    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0)
    {
        fprintf(stderr, "ERR: Setsockopt!\n");
        freeaddrinfo(server_info);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handle_interrupt;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "ERR: Setting up signal handler!\n");
        freeaddrinfo(server_info);
        exit(1);
    }

    // 32 bit ID
    uint8_t message_id_lsb = 0xFF;
    uint8_t message_id_msb = 0xFF;

    enum State current_state = START;       // current state
    int proccessing = 0;                    // allows/disallows klint to send messages
    char *display_name = NULL;
    Node *head = NULL;                      // pointer to the list of arrived IDs
    ipk_list *fifo = NULL;                  // pointer to the FIFO of messages/commands written by the client
    
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client_socket;
    fds[1].events = POLLIN;

    int timeout = -1;                               // if it is -1, then poll waits indefinitely
    int err_event = 0;                              // error happened
    int retransmition = max_num_retransmissions;    // maximum number of forwarding
    time_t send_time;                               // variable to measure time

    int id_conf = -1;                               // variable to check if the CONFIRM ID matches the ID of the message sent
    char *buff = NULL;                              // messages to be sent to the server
    size_t buff_len = 0;                            // buff length

    socklen_t addr_len = sizeof(struct sockaddr);
    struct sockaddr_storage server_addr;            // used to change the port

    while(1)
    {
        char *buff_confirm = NULL;
        size_t buff_confirm_len = 0;
        int not_conf = 0;

        if (current_state == ERR_CONF)
        {
            current_state = BYE_SEND;
            message_id_increase(&message_id_lsb, &message_id_msb);
            if (buff != NULL) free(buff);
            buff = NULL;
            bye(&buff, &buff_len, &message_id_lsb, &message_id_msb);
        }
        else
        {
            int ret = poll(fds, 2, timeout);

            if (timeout != -1 && ret == 0)
            {
                retransmition--;
                if (retransmition == 0)
                {
                    fprintf(stderr, "ERR: Timeout and retransmition failed!\n");
                    udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                }
            }
            else if (received_signal && current_state != BYE_SEND)
            {
                current_state = BYE_SEND;
                message_id_increase(&message_id_lsb, &message_id_msb);
                if (buff != NULL) free(buff);
                buff = NULL;
                bye(&buff, &buff_len, &message_id_lsb, &message_id_msb);
            }
            else
            {
                if (ret < 0) 
                {
                    fprintf(stderr, "ERR: poll!\n");
                    udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                }

                if (fds[1].revents & POLLIN) 
                {
                    char response[MAX_MESSAGE_SIZE] = {0};
                    ssize_t recv_result = recvfrom(client_socket, response, sizeof(response), 0, (struct sockaddr *) &server_addr, &addr_len);
                    if (recv_result < 0)
                    {
                        fprintf(stderr, "ERR: Can't receive message!\n");
                        udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                    }
                    else if (recv_result == 0)
                    {
                        current_state = ERR_CONF;
                        continue;
                    }
                    
                    // there is no timeout, the message arrived, but it is not CONFIRM to the message sent by the client, 
                    // so we detect the elapsed time and set it as a timeout
                    if (timeout != -1)
                    {
                        if (response[0] != 0)
                        {
                            double arrival_time = difftime(time(NULL), send_time);
                            timeout = timeout - (int) (arrival_time * 1000);
                            not_conf = 1;
                        }
                    }

                    // The ID of the message from the server
                    int message_id = (response[1] << 8) | response[2];

                    switch (current_state)
                    {
                    case BYE_SEND:
                        if (response[0] == 0)
                        {             
                            if (message_id == id_conf)
                            {
                                udp_conf(&id_conf, &buff, &retransmition, max_num_retransmissions, &timeout, &current_state, BYE_CONF);
                            }
                            else
                            {
                                // it is CONFIRM, but not on the message sent by the client, so we detect 
                                // the elapsed time and set it as a timeout
                                double arrival_time = difftime(time(NULL), send_time);
                                timeout = timeout - (int) (arrival_time * 1000);
                                not_conf = 1;
                            }
                        }
                        break;
                    case AUTH_SEND:
                        if (response[0] == 0)
                        {
                            if (message_id == id_conf)
                            {
                                udp_conf(&id_conf, &buff, &retransmition, max_num_retransmissions, &timeout, &current_state, AUTH_CONF);
                                id_conf = message_id;
                            }
                            else
                            {
                                // it is CONFIRM, but not on the message sent by the client, 
                                // so we detect the elapsed time and set it as a timeout
                                double arrival_time = difftime(time(NULL), send_time);
                                timeout = timeout - (int) (arrival_time * 1000);
                                not_conf = 1;
                            }
                        }
                        break;
                    case AUTH_CONF:
                        if (response[0] == 1)
                        {
                            int result = response[3];
                            int ref_message_id = (response[4] << 8) | response[5];
                            char *message = NULL;

                            if (ref_message_id == id_conf)
                            {
                                udp_message_next(response, &message, 6, sizeof(response));
                                add_node(&head, message_id);
                                
                                if (result == 1)
                                {
                                    current_state = MSG_SEND;
                                    fprintf(stderr, "Success: %s\n", message);
                                }
                                else
                                {
                                    current_state = START;
                                    fprintf(stderr, "Failure: %s\n", message);
                                }

                                free(message);
                                proccessing = 0;

                                // port change
                                uint16_t server_port = ntohs(((struct sockaddr_in *)& server_addr)->sin_port);
                                ((struct sockaddr_in *)server_info->ai_addr)->sin_port = htons(server_port);
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);
                            }
                        }
                        else if (response[0] == -2)
                        {
                            current_state = ERR_CONF;
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stderr, "ERR FROM %s: %s\n", display_name, message);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        break;
                    case MSG_CONF:
                        if (response[0] == 0)
                        {
                            if (message_id == id_conf)
                            {
                                udp_conf(&id_conf, &buff, &retransmition, max_num_retransmissions, &timeout, &current_state, MSG_SEND);
                                proccessing = 0;
                            }
                            else
                            {
                                // it is CONFIRM, but not on the message sent by the client, so we detect the elapsed time 
                                // and set it as a timeout
                                double arrival_time = difftime(time(NULL), send_time);
                                timeout = timeout - (int) (arrival_time * 1000);
                                not_conf = 1;
                            }
                        }
                        else if (response[0] == 1)
                        {
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);
                        }
                        else if (response[0] == 4)
                        {
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stdout, "%s: %s\n", display_name, message);
                                fflush(stdout);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -2)
                        {
                            current_state = ERR_CONF;
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stderr, "ERR FROM %s: %s\n", display_name, message);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -1)
                        {
                                current_state = BYE_CONF;
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]); 
                        }
                        break;
                    case MSG_SEND:
                        if (response[0] == 4)
                        {
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stdout, "%s: %s\n", display_name, message);
                                fflush(stdout);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -2)
                        {
                            current_state = ERR_CONF;
                            char *display_name = NULL;
                            char *message = NULL;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stderr, "ERR FROM %s: %s\n", display_name, message);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -1)
                        {
                                current_state = BYE_CONF;
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]); 
                        }
                        else if (response[0] != 0 && response[0] != 1 && response[0] != 2)
                        {
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);  
                            message_id_increase(&message_id_lsb, &message_id_msb);
                            if (buff != NULL) free(buff);
                            buff = NULL;
                            int message_code = err(&buff, &buff_len, &message_id_lsb, &message_id_msb, display_name, "ERR: Uknown message!\n");
                            if (message_code)
                            {
                                fprintf(stderr, "ERR: Can't send message!\n");
                                udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                            }
                            fprintf(stderr, "ERR: Uknown message!\n");
                            current_state = ERR_SEND;
                            err_event = 1;
                        }
                        break;
                    case JOIN_SEND:
                        if (response[0] == 0)
                        {
                            if (message_id == id_conf)
                            {
                                udp_conf(&id_conf, &buff, &retransmition, max_num_retransmissions, &timeout, &current_state, JOIN_CONF);
                                id_conf = message_id;
                            }
                            else
                            {
                                double arrival_time = difftime(time(NULL), send_time);
                                timeout = timeout - (int) (arrival_time * 1000);
                                not_conf = 1;
                            }
                        }
                        else if (response[0] == 4)
                        {
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stdout, "%s: %s\n", display_name, message);
                                fflush(stdout);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -2)
                        {
                            current_state = ERR_CONF;
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stderr, "ERR FROM %s: %s\n", display_name, message);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -1)
                        {
                                current_state = BYE_CONF;
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]); 
                        }
                        break;
                    case JOIN_CONF:
                        if (response[0] == 1)
                        {
                            int result = response[3];
                            int ref_message_id = (response[4] << 8) | response[5];
                            char *message;
                            if (ref_message_id == id_conf)
                            {
                                udp_message_next(response, &message, 6, sizeof(response));
                                add_node(&head, message_id);

                                if (result == 1)
                                {
                                    fprintf(stderr, "Success: %s\n", message);
                                }
                                else
                                {
                                    fprintf(stderr, "Failure: %s\n", message);
                                }
                                free(message);
                                proccessing = 0;
                                current_state = MSG_SEND;
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);
                            }
                        }
                        else if (response[0] == 4)
                        {
                            char *display_name;
                            char *message;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stdout, "%s: %s\n", display_name, message);
                                fflush(stdout);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -2)
                        {
                            current_state = ERR_CONF;
                            char *display_name = NULL;
                            char *message = NULL;

                            Node *search_res = search_node(head, message_id);
                            if (search_res == NULL)
                            {
                                add_node(&head, message_id);
                                udp_message_next(response, &display_name, 3, sizeof(response));
                                udp_message_next(response, &message, 4 + strlen(display_name), sizeof(response));
                                fprintf(stderr, "ERR FROM %s: %s\n", display_name, message);
                                if (message != NULL) free(message);
                                if (display_name != NULL) free(display_name);
                            }
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);    
                        }
                        else if (response[0] == -1)
                        {
                                current_state = BYE_CONF;
                                confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]); 
                        }
                        else if (response[0] != 0 && response[0] != 1 && response[0] != 2)
                        {
                            confirm(&buff_confirm, &buff_confirm_len, (uint8_t *) &response[2], (uint8_t *) &response[1]);  
                            message_id_increase(&message_id_lsb, &message_id_msb);
                            if (buff != NULL) free(buff);
                            buff = NULL;
                            int message_code = err(&buff, &buff_len, &message_id_lsb, &message_id_msb, display_name, "ERR: Uknown message!\n");
                            if (message_code)
                            {
                                fprintf(stderr, "ERR: Can't send message!\n");
                                udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                            }
                            fprintf(stderr, "ERR: Uknown message!\n");
                            current_state = ERR_SEND;
                            err_event = 1;
                        }
                        break;
                    case ERR_SEND:
                        if (response[0] == 0)
                        {
                            if (message_id == id_conf)
                            {
                                current_state = ERR_CONF;
                                continue;
                            }
                            else
                            {
                                double arrival_time = difftime(time(NULL), send_time);
                                timeout = timeout - (int) (arrival_time * 1000);
                                not_conf = 1;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }

                // message from server
                if (fds[0].revents & POLLIN) 
                {
                    char input[1400];
                    if (fgets(input, sizeof(input), stdin) != NULL) 
                    {
                        if (input[strlen(input) - 1] == '\n') 
                            input[strlen(input) - 1] = '\0';

                        // save into FIFO
                        insert_at_end(&fifo, input);
                    }
                    if (feof(stdin))
                    {
                        current_state = ERR_CONF;
                        continue;
                    }
                }

                // if the client is not blocking the sending of further messages 
                // (one is currently being processed), then the oldest input is 
                // taken from the FIFO, processed and sent
                if (!proccessing)
                {
                    if (!err_event)
                    {
                        // takes a record from the FIFO
                        ipk_list *removed_node = remove_from_front(&fifo);
                        if (removed_node != NULL)
                        {
                            int input_code = check_input(removed_node->input);

                            switch (current_state)
                            {
                            case START:
                                if (input_code == 1)
                                {
                                    char *token = strtok(removed_node->input, " ");
                                    char *param1 = NULL;
                                    char *param2 = NULL;
                                    char *param3 = NULL;

                                    if (token != NULL)
                                    {
                                        param1 = strtok(NULL, " ");
                                        param2 = strtok(NULL, " ");
                                        param3 = strtok(NULL, " ");
                                    }   

                                    if (param1 == NULL || param2 == NULL || param3 == NULL)
                                    {
                                        fprintf(stderr, "ERR: Parameters do not match!\n");
                                    }
                                    else
                                    {
                                        message_id_increase(&message_id_lsb, &message_id_msb);
                                        int message_code = auth(&buff, &buff_len, &message_id_lsb, &message_id_msb, param1, param3, param2);
                                        display_name = strdup(param3);
                                        if (display_name == NULL)
                                        {
                                            fprintf(stderr, "ERR: Memory allocation failed!\n");
                                            udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                                        }
                                        current_state = AUTH_SEND;

                                        if (message_code)
                                        {
                                            fprintf(stderr, "ERR: Can't send message!\n");
                                            udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                                        }
                                    }
                                }
                                else if (input_code == 4)
                                {
                                    print_help();
                                }
                                else
                                {
                                    fprintf(stderr, "ERR: You are not authorized!\n");
                                }
                                break;
                            case MSG_SEND:
                                if (input_code == 1)
                                {
                                    fprintf(stderr, "ERR: Alreade authorized!\n");
                                }
                                else if (input_code == 2)
                                {
                                    char *token = strtok(removed_node->input, " ");
                                    char *param1 = NULL;

                                    if (token != NULL) param1 = strtok(NULL, " ");

                                    if (param1 == NULL)
                                    {
                                        fprintf(stderr, "ERR: Parameter's number does not match!\n");
                                    }
                                    else
                                    {
                                        message_id_increase(&message_id_lsb, &message_id_msb);
                                        int message_code = join(&buff, &buff_len, &message_id_lsb, &message_id_msb, param1, display_name);
                                        if (message_code)
                                        {
                                            fprintf(stderr, "ERR: Can't send message!\n");
                                            udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                                        }
                                        current_state = JOIN_SEND;
                                    }
                                }
                                else if (input_code == 3)
                                {
                                    char *token = strtok(removed_node->input, " ");
                                    char *param1 = NULL;

                                    if (token != NULL) param1 = strtok(NULL, " ");

                                    if (param1 == NULL)
                                    {
                                        fprintf(stderr, "ERR: Parameter's number does not match!\n");
                                    }
                                    else
                                    {
                                        if (display_name != NULL) free(display_name);
                                        display_name = strdup(param1);
                                        if (display_name == NULL)
                                        {
                                            fprintf(stderr, "ERR: Memory allocation failed!\n");
                                            udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                                        }
                                    }
                                }
                                else if (input_code == 4)
                                {
                                    print_help();
                                }
                                else if (input_code == 6)
                                {
                                    message_id_increase(&message_id_lsb, &message_id_msb);
                                    int message_code = msg(&buff, &buff_len, &message_id_lsb, &message_id_msb, display_name, removed_node->input);
                                    if (message_code)
                                    {
                                        fprintf(stderr, "ERR: Can't send message!\n");
                                        udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
                                    }
                                    current_state = MSG_CONF;
                                }
                                break;
                            default:
                                break;
                            }
                            free(removed_node->input);
                            free(removed_node);
                        }
                    }
                }
                else
                {
                    not_conf = 1;
                }
            }
        }

        // sends CONFIRM to messages from the server
        if (buff_confirm != NULL)
        {
            if (sendto(client_socket, buff_confirm, buff_confirm_len, 0, server_info->ai_addr, addr_len) < 0)
            {
                fprintf(stderr, "ERR: Can't send message!\n");
                udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
            }

            free(buff_confirm);
            buff_confirm = NULL;
        }

        // sending messages to the server specified by the client
        if (buff != NULL && not_conf == 0)
        {
            id_conf = (message_id_msb << 8) | message_id_lsb;
            timeout = conf_timeout;
            send_time = time(NULL);
            proccessing = 1;
            if (sendto(client_socket, buff, buff_len, 0, server_info->ai_addr, addr_len) < 0)
            {
                fprintf(stderr, "ERR: Can't send message!\n");
                udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 1);
            }
        }

        // the connection was terminated correctly
        if (current_state == BYE_CONF)
        {
            udp_exit(&buff, &display_name, &buff_confirm, &head, &fifo, client_socket, &server_info, 0);
        }
    }
}

int main(int argc, char *argv[])
{
    int opt;
    int conf_timeout = DEFAULT_CONF_TIMEOUT;
    int max_num_retransmissions = DEFAULT_MAX_RETRANSMISSIONS;

    char *port = DEFAULT_SERVER_PORT;
    char *transfer_protocol = NULL;
    char *ip_addr = NULL;

    while ((opt = getopt(argc, argv, "t:s:p:d:r:h")) != -1) 
    {
        switch (opt)
        {
            case 't':
                transfer_protocol = optarg;
                break;
            case 's':
                ip_addr = optarg;
                break;
            case 'p':
                if (atoi(optarg) <= 0 || atoi(optarg) > 65535) 
                {
                    fprintf(stderr, "ERR: Invalid port number! Port must be between 1 and 65535!\n");
                    exit(1);
                }
                port = optarg;
                break;
            case 'd':
                conf_timeout = atoi(optarg);
                if (conf_timeout <= 0) 
                {
                    fprintf(stderr, "ERR: Invalid UDP confirmation timeout! Must be a positive integer!\n");
                    exit(1);
                }
                break;
            case 'r':
                max_num_retransmissions = atoi(optarg);
                break;
            case 'h':
                print_help();
                exit(0);
            default:
                fprintf(stderr, "ERR: Use './ipk24-chat-client -h' for help!\n");
                exit(1);
        }
    }

    opt_arg_check(transfer_protocol, ip_addr);
    
    if (!strcmp(transfer_protocol, "tcp")) tcp(ip_addr, port);
    else udp(ip_addr, port, conf_timeout, max_num_retransmissions);

    return 0;
}