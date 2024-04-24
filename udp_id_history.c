#include "udp_id_history.h"

/**
 * @brief add new ID to node
 * 
 * @param head 
 * @param id 
 */
void add_node(Node **head, int id)
{
    Node *newNode = (Node *) malloc(sizeof(Node));
    if (newNode == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed\n");
        exit(1);
    }
    newNode->data.id = id;
    newNode->next = *head;
    *head = newNode;
}

/**
 * @brief search ID in Node list
 * 
 * @param head 
 * @param id 
 * @return Node* if exists
 */
Node *search_node(Node *head, int id)
{
    Node *current = head;
    while (current != NULL)
    {
        if (current->data.id == id) 
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief delete Node with ID
 * 
 * @param head 
 * @param id 
 */
void delete_node(Node **head, int id)
{
    Node *current = *head;
    Node *prev = NULL;

    while (current != NULL && current->data.id != id)
    {
        prev = current;
        current = current->next;
    }

    if (current != NULL)
    {
        if (prev == NULL)
        {
            *head = current->next;
        }
        else
        {
            prev->next = current->next;
        }
        free(current);
    }
}

/**
 * @brief Free memmory
 * 
 * @param head 
 */
void free_list(Node *head)
{
    Node *current = head;
    while (current != NULL)
    {
        Node *temp = current;
        current = current->next;
        free(temp);
    }
}
