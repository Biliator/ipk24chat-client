#include "udp.h"

/**
  * @brief Constructs a CONFIRM message and executes it in the buff variable
  *
  * @param content message
  * @param length pointer to the total length of the string, which is initially 0
  * @param lsb least significant bit
  * @param msb the most significant bit
  * @return int 1 if an allocation error occurred, 0 otherwise
  */
int confirm(char **content, size_t *length, uint8_t *lsb, uint8_t *msb)
{
    *length = 3;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length + 1, "%c%c%c", 0x00, *msb, *lsb);
    return 0;
}

/**
 * @brief Builds an AUTH message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param username the login name
 * @param display_name the name to be represented by
 * @param secret secret password/key
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int auth(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *username, char *display_name, char *secret)
{
    *length = strlen(username) + strlen(display_name) + strlen(secret) + 6;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%s%c%s%c%s%c", 0x02, *msb, *lsb, username, '\0', display_name, '\0', secret, '\0');
    return 0;
}

/**
 * @brief Builds a JOIN message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param channel_id the new channel
 * @param display_name the name to be represented by
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int join(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *channel_id, char *display_name)
{
    *length = strlen(channel_id) + strlen(display_name) + 5;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length + 1, "%c%c%c%s%c%s%c", 0x03, *msb, *lsb, channel_id, 0x00, display_name, 0x00);
    return 0;
}

/**
 * @brief Builds an MSG message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param display_name the name to be represented by
 * @param message_contents the message specified by the user
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int msg(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *display_name, char *message_contents)
{
    *length = strlen(display_name) + strlen(message_contents) + 5;
    *content = (char *) malloc(*length);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%s%c%s%c", 0x04, *msb, *lsb, display_name, 0x00, message_contents, 0x00);
    return 0;
}

/**
 * @brief Builds an MSG message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param display_name the name to be represented by
 * @param message_contents the error message
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int err(char **content, size_t *length, uint8_t *lsb, uint8_t *msb, char *display_name, char *message_contents)
{
    *length = strlen(display_name) + strlen(message_contents) + 5;
    *content = (char *) malloc(*length);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%s%c%s%c", 0xFE, *msb, *lsb, display_name, 0x00, message_contents, 0x00);
    return 0;
}

/**
 * @brief Builds a BYE message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int bye(char **content, size_t *length, uint8_t *lsb, uint8_t *msb)
{
    *length = 3;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length + 1, "%c%c%c", 0xFF, *msb, *lsb);
    return 0;
}

/**
 * @brief Increments the ID by 1
 *
 * @param lsb least significant bit
 * @param msb most significant bit
 */
void message_id_increase(uint8_t *lsb, uint8_t *msb)
{
    if (*msb == 0xFF && *lsb == 0xFF)
        *msb = *lsb = 0x00;
    else if (*lsb == 0xFF)
    {
        *lsb = 0x00;
        (*msb)++;
    }
    else (*lsb)++;
}

/**
 * @brief Helper function to get a parameter like name, message, etc.
 * Loops through the entire string and splits it using 0x00
 *
 * @param input The input string
 * @param output Output is one string
 * @param start the beginning of the predicted string
 * @param input_size size of the input string
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int udp_message_next(char input[], char **output, int start, size_t input_size)
{
    int i = start;

    while (i < (int) input_size && input[i] != 0x00)
        i++;

    *output = (char *) malloc((i - start + 1) * sizeof(char));
    if (*output == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    i = start;
    while (i < (int) input_size && input[i] != 0x00)
    {
        (*output)[i - start] = input[i];
        i++;
    }

    (*output)[i - start] = '\0';
    return 0;
}
