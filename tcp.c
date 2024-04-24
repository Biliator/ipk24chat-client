#include "tcp.h"

/**
 * @brief Builds an AUTH message and stores it in the buff variable
 *
 * @param buff final message
 * @param username the login name
 * @param display_name the name to be represented by
 * @param secret secret password/key
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int content_auth(char **buff, char *username, char *display_name, char *secret)
{
    size_t length = strlen(username) + strlen(display_name) + strlen(secret) + 19;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "AUTH %s AS %s USING %s\r\n", username, display_name, secret);
    return 0;
}

/**
 * @brief Builds a JOIN message and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name the name to be represented by
 * @param channel_id the new channel
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int content_join(char **buff, char *display_name, char *channel_id)
{
    size_t length = strlen(display_name) + strlen(channel_id) + 12;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "JOIN %s AS %s\r\n", channel_id, display_name);
    return 0;
}

/**
 * @brief Builds an MSG message and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name the name to be represented by
 * @param message the message specified by the user
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int content_message(char **buff, char *display_name, char *message)
{
    size_t length = strlen(display_name) + strlen(message) + 16;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERROR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "MSG FROM %s IS %s\r\n", display_name, message);
    return 0;
}

/**
 * @brief Sestaví BYE zprávy a uloží jí do proměnné buff
 * 
 * @param buff finální zpráva
 * @return int 1 pokud nastala chyba při alokaci, 0 jinak
 */
int content_bye(char **buff)
{
    *buff = (char *) malloc(6);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, 6, "BYE\r\n");
    return 0;
}

/**
 * @brief Builds the ERR message and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name the name to be represented by
 * @param message the error message
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int content_err(char **buff, char *display_name, char *message)
{
    size_t length = strlen(display_name) + strlen(message) + 16;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "ERR FROM %s IS %s\r\n", display_name, message);
    return 0;
}

/**
 * @brief Checks if the MSG message from the server is in the correct format
 *
 * @param reply message from the server
 * @param display_name message author
 * @param message the message
 * @return int 1 if the shape is wrong, 0 otherwise
 */
int tcp_check_msg(char *reply, char **display_name, char **message)
{
    char *token;

    token = strtok(reply, " ");
    if (token == NULL || strcasecmp(token, "MSG") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "FROM") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *display_name = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "IS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;

    *message = strdup(token);
    
    return 0;
}

/**
 * @brief Checks whether the REPLY message from the server is in the correct form
 *
 * @param reply message from the server
 * @param res_succ result (OK|NOK)
 * @param message the message
 * @return int 1 if the shape is wrong, 0 otherwise
 */
int tcp_check_reply(char *reply, char **res_succ, char **message)
{
    char *token;

    token = strtok(reply, " ");
    if (token == NULL || strcasecmp(token, "REPLY") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL || (strcasecmp(token, "OK") != 0 && strcasecmp(token, "NOK") != 0)) return 1;
    
    *res_succ = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "IS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;

    *message = strdup(token);
    return 0;
}

/**
 * @brief Checks if the ERR message from the server is in the correct form
 *
 * @param reply message from the server
 * @param display_name message author
 * @param message the error message
 * @return int 1 if the shape is wrong, 0 otherwise
 */
int tcp_check_err(char *reply, char **display_name, char **message)
{
    char *token;

    token = strtok(reply, " ");
    if (token == NULL || strcasecmp(token, "ERR") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "FROM") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *display_name = token;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "IS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;

    *message = token;

    return 0;
}

/**
 * @brief Checks whether the BYE message from the server is in the correct form
 *
 * @param reply message from the server
 * @return int 1 if the shape is wrong, 0 otherwise
 */
int tcp_check_bye(char *reply)
{
    char *token;

    token = strtok(reply, "\r\n");
    if (token == NULL || strcasecmp(token, "BYE") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token != NULL && strlen(token) > 0) return 1;

    return 0;
}