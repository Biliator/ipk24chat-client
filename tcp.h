#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int content_auth(char **content, char *username, char *display_name, char *secret);
int content_join(char **content, char *display_name, char *channel_id);
int content_message(char **content, char *username, char *message);
int content_bye(char **content);
int content_err(char **content, char *username, char *message);
int tcp_check_msg(char *reply, char **display_name, char **message);
int tcp_check_reply(char *reply, char **res_succ, char **message);
int tcp_check_err(char *reply, char **display_name, char **message);
int tcp_check_bye(char *reply);