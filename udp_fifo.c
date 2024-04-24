#include "udp_fifo.h"

/**
 * @brief Create a node object
 * 
 * @param input console message
 * @return ipk_list* 
 */
ipk_list* create_node(char *input)
{
    ipk_list *new_node = (ipk_list*)malloc(sizeof(ipk_list));
    if (new_node == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        exit(1);
    }
    new_node->input = strdup(input);
    new_node->next = NULL;
    return new_node;
}

/**
 * @brief insert new message at the end
 * 
 * @param head 
 * @param input console message
 */
void insert_at_end(ipk_list **head, char *input)
{
    ipk_list *new_node = create_node(input);
    if (*head == NULL)
    {
        *head = new_node;
        return;
    }
    ipk_list *temp = *head;
    while (temp->next != NULL)
        temp = temp->next;

    temp->next = new_node;
}

/**
 * @brief remove first message
 * 
 * @param head 
 * @return ipk_list* 
 */
ipk_list* remove_from_front(ipk_list **head)
{
    if (*head == NULL) return NULL;
    ipk_list *temp = *head;
    *head = (*head)->next;
    return temp;
}

/**
 * @brief free memmory
 * 
 * @param head 
 */
void free_fifo(ipk_list *head)
{
    while (head != NULL)
    {
        ipk_list *temp = head;
        head = head->next;
        free(temp->input);
        free(temp);
    }
}