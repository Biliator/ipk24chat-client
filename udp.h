#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int confirm(char **content, size_t *length, uint8_t *lsb, uint8_t *msb);
int auth(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *username, char *display_name, char *secret);
int join(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *channel_id, char *display_name);
int msg(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *display_name, char *message_contents);
int err(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *display_name, char *message_contents);
int bye(char **content, size_t *length, uint8_t *lsb, uint8_t *msb);
void message_id_increase(uint8_t *lsb, uint8_t *msb);
int udp_message_next(char input[], char **output, int start, size_t input_size);