#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int id;
} Data;

typedef struct Node
{
    Data data;
    struct Node *next;
} Node;

void add_node(Node **head, int id);
Node *search_node(Node *head, int id);
void delete_node(Node **head, int id);
void free_list(Node *head);
