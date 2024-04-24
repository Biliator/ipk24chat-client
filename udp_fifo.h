#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ipk_list
{
    char *input;
    struct ipk_list *next;
} ipk_list;

ipk_list* create_node(char *input);
void insert_at_end(ipk_list **head, char *input);
ipk_list* remove_from_front(ipk_list **head);
void free_fifo(ipk_list *head);