#ifndef LIST_H
#define LIST_H

/* ************************************************************************ */

// Librerie
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

/* ************************************************************************ */

typedef struct Node Node;
struct Node {
    char ip[17];
    int points;
    Node *next;
};

typedef struct List {
    Node *head;
    int dim;
    pthread_mutex_t *mutex;
} List;

/* ************************************************************************ */

// Funzioni
List *createLeaderboard();

bool addUserToLeaderboard(List *list, char *key, int value);

void sortLeaderboard(Node** head, int count);

/* ************************************************************************ */

#endif
