#include "list.h"

/* ************************************************************************ */

/**
 * Funzione che crea e inizializza la lista
 * */
List* createLeaderboard() {
    List* newList = malloc(sizeof(List));
    newList->dim = 0;
    newList->head = NULL;
    newList->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(newList->mutex, NULL);

    return newList;
}

/**
 * Verifica se il parametro 'key' è nella lista, se non c'è aggiunge la coppia (key, value)
 * @param list la lista
 * @param key l'ip del giocatore
 * @param value il valore corrispondente alla chiave
 * @return true se la coppia è stata aggiunta, false altrimenti
 */
bool addUserToLeaderboard(List* list, char* key, int value) {
    pthread_mutex_lock(list->mutex);
    Node* head = list->head;
    Node* prev = NULL;
    while(head != NULL) {
        if(strcmp(head->ip, key) == 0) {
            pthread_mutex_unlock(list->mutex);
            return false;
        }
        prev = head;
        head = head->next;
    }

    Node* newNode = malloc(sizeof(Node));
    strcpy(newNode->ip, key);

    newNode->points = value;
    newNode->next = NULL;

    if(prev == NULL) {
        list->head = newNode;
    }
    else {
        prev->next = newNode;
    }

    list->dim = list->dim + 1;

    pthread_mutex_unlock(list->mutex);
    return true;
}

/**
 * Funzione di supporto che fa lo swap dei nodi
 * */
Node* swap(Node* ptr1, Node* ptr2) {
    Node* tmp = ptr2->next;
    ptr2->next = ptr1;
    ptr1->next = tmp;
    return ptr2;
}

/**
 * Funzione che ordina la lista
 * @param head nodo della lista
 * @count count dimensione della lista
 * */
void sortLeaderboard(Node** head, int count) {
    struct Node** h;
    int i, j, swapped;
  
    for (i = 0; i <= count; i++) {
  
        h = head;
        swapped = 0;
  
        for (j = 0; j < count - i - 1; j++) {
  
            Node* p1 = *h;
            Node* p2 = p1->next;
  
            if (p1->points < p2->points) {
  
                /*aggiorna link dopo lo scambio*/
                *h = swap(p1, p2);
                swapped = 1;
            }
  
            h = &(*h)->next;
        }
  
        /* termina se il ciclo finisce senza aver effettuato scambi*/
        if (swapped == 0)
            break;
    }
}
