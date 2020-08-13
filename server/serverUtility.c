#include "serverUtility.h"

/**
 * Inizializza la struct che contiene i campi numPlayers e mutex
 * @return la struttura inizializzata
 */
numberOfPlayers* init_struct(){
    numberOfPlayers* fp = malloc(sizeof(numberOfPlayers));
    if(fp == NULL){
        return NULL;
    }
    fp->numPlayers = 0;
    fp->mutex = malloc(sizeof(pthread_mutex_t));
    return fp;
}

/**
 * Scrive un intero sul socket tenendo conto della differenza di rappresentazione di un int
 * @param fd il file descriptor del socket su cui scrivere
 * @param num il valore numerico che si intende scrivere sul socket
 * @return 0 se ha successo, -1 altrimenti
 */
int sendInt(int fd, int num) {
    int32_t conv = htonl(num);
    char* data = (char*) &conv;
    int missingBytes = sizeof(conv);
    int bytesWritten;

    // Ciclo scrivendo i bytes finché non scrivo tutto l'intero
    do {
        bytesWritten = write(fd, data, missingBytes);
        if (bytesWritten < 0) {
            return -1;
        }

        // Sposto il puntatore per continuare a scrivere da quel byte
        data += bytesWritten;

        // Decremento il numero di bytes mancanti da scrivere
        missingBytes -= bytesWritten;
    } while (missingBytes > 0);

    return 0;
}

/**
 * Scrive una stringa sul socket, necessaria in quanto una write può scrivere meno byte di quanto dovrebbe
 * @param fd il file descriptor del socket su cui scrivere
 * @param str la stringa che si intende scrivere sul socket
 * @param strLength la grandezza della stringa
 * @return 0 se ha successo, -1 altrimenti
 */
int sendString(int fd, char* str, int strLength) {
    // Bytes mancanti, si potrebbe usare strLenght ma una nuova variabile ne aumenta la leggibilità
    int missingBytes = strLength;
    int bytesWritten;

    // Ciclo scrivendo i bytes finché non scrivo tutto la stringa
    do {
        bytesWritten = write(fd, str, missingBytes);
        if (bytesWritten < 0) {
            return -1;
        }

        // Sposto il puntatore per continuare a scrivere da quel byte
        str += bytesWritten;

        // Decremento il numero di bytes mancanti da scrivere
        missingBytes -= bytesWritten;
    } while (missingBytes > 0);

    return 0;
}

/**
 * Setta il timeout, cioè il tempo il client ha a disposizione
 * @param fd è il file descriptor
 * @param timeOutInSeconds tempo espresso in secondi
 */
void setTimeoutOnSocket(int fd, int timeOutInSeconds) {
    struct timeval tv;
    tv.tv_sec = timeOutInSeconds;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

/**
 * Prende un intero dal socket
 * e della lettura dal socket che può essere minore della grandezza dell'intero
 * @param fd il file descriptor del socket su cui leggere
 * @param num il valore numerico che si intende leggere dal socket (è un parametro di ritorno)
 * @return 0 se ha successo, -1 altrimenti
 */
int receiveInt(int fd, int* num) {
    int32_t ret;
    char* data = (char*) &ret;
    int missingBytes = sizeof(ret); // Bytes mancanti
    int bytesRead;

    // Ciclo leggendo i bytes finché non leggo tutto l'intero
    do {
        bytesRead = read(fd, data, missingBytes);
        if(bytesRead < 0) {
            return -1;
        }

        // Sposto il puntatore per continuare a leggere da quel byte
        data += bytesRead;

        // Decremento il numero di bytes mancanti da leggere
        missingBytes -= bytesRead;
    } while (missingBytes > 0);

    // Casto il risultato da network a valore host
    *num = ntohl(ret);
    return 0;
}

/**
 * Calcola il punteggio del giocatore in base alla risposta data
 * @param list: lista che contiene le informazioni associate al giocatore
 * @param clientIp: ip del client corrente
 * @param increment: flag che indica se aggiungere o decurtare punti. Se param è 1 allora verrà
 * aggiunto +1 al punteggio del giocatore, viceversa se è 0 verrà decurtato 1 punto
 */
void setPlayerScore(List* list, char* clientIp, bool increment){
    pthread_mutex_lock(list->mutex);
    Node* head = list->head;
    while(head != NULL) {
        if(strcmp(head->ip, clientIp) == 0) {
            if(increment) {
                head->points++;
            } else{
                head->points--;
            }
            pthread_mutex_unlock(list->mutex);
            return;
        }
        head = head->next;
    }
    pthread_mutex_unlock(list->mutex);
}

/**
 * Stampa la classifica attuale, inviando prima la dimensione della stessa al client
 * e poi procedendo alla stampa di Ip e punteggio
 * @param list: identifica la classifica da stampare
 * @param socketAddr: indirizzo del socket
 * */
void printLeaderboard(List* list, int socketAddr){
    pthread_mutex_lock(list->mutex);
    sendInt(socketAddr, list->dim);

    Node* travel = list->head;
    while(travel != NULL) {
        sendInt(socketAddr, (int) strlen(travel->ip));
        sendString(socketAddr, travel->ip, (int) strlen(travel->ip));
        sendInt(socketAddr, travel->points);
        travel = travel->next;
    }
    pthread_mutex_unlock(list->mutex);
}