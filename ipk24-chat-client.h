#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <wait.h>
#include <poll.h>
#include <regex.h>
#include <time.h>
#include <stdint.h>
#include "udp_fifo.h"
#include "udp.h"
#include "tcp.h"
#include "udp_id_history.h"

#define DEFAULT_CONF_TIMEOUT 250
#define DEFAULT_MAX_RETRANSMISSIONS 3
#define MAX_MESSAGE_SIZE 1500
#define DEFAULT_CHANNEL "channel1"
#define DEFAULT_SERVER_PORT "4567"
//11559478-9b5c-4b74-935b-13070e18d768

volatile sig_atomic_t received_signal = 0;

enum State
{
    START = 0,
    AUTH_SEND,
    AUTH_CONF,
    AUTH_REPLY,
    JOIN_SEND,
    JOIN_CONF,
    MSG_SEND,
    MSG_CONF,
    ERR_SEND,
    ERR_CONF,
    BYE_SEND,
    BYE_CONF
};

enum Response
{
    INT_ERR,
    ERR,
    OK,
    NOK,
    MSG,
    BYE,
    UKNOWN
};

void handle_interrupt(int signum);
void opt_arg_check(char *transfer_protocol, char *ip_addr);
void print_help();
int check_input(char *input);
enum Response check_response(char *response, char **resp, char **resp_succ, char **message);
int recv_next_state(char *response, char **display_name, char **buff, int state, int *proccessing, int client_socket);
void udp_exit(char **buff, char **display_name, char **buff_confirm, struct Node **head, struct ipk_list **fifo, int client_socket, struct addrinfo **server_info, int exit_code);
void udp_conf(int *id_conf, char ** buff, int *retransmition, int max_num_retransmissions, int *timeout, enum State *current_state, enum State next_state);
void tcp(char *host, char *port);
void udp(char *host, char *port, int conf_timeout, int max_num_retransmissions);