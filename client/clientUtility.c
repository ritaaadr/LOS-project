#include "clientUtility.h"

/**
 * Verifica che la stringa in ingresso sia costituita da soli numeri
 * @param string la stringa da controllare
 * @return true se formata da soli numeri, false altrimenti
 */
bool strIsNumber(const char* string) {
    int i = 0;
    while(string[i]) {
        // Se il singolo carattere della stringa non è compreso tra 48 e 57 allora non è un numero
        if(!((int)string[i] >= 48 && (int)string[i] <= 57)) {
            return false;
        }
        i++;
    }
    return true;
}
/**
 * Riceve la risposta dell'utente entro un limite di tempo
 * ritorna la risposta dell'utente, -1 altrimenti
 */
int getAnswerWithTimeout() {
//Inizializzo le proprietà del file descriptor
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = MAX_TIME-1;
    timeout.tv_usec = 0;
    int givenAns = -1;

//Aspetto l'input dell'utente o il timeout; il primo parametro è
// uno in più del più grande file descriptor
    if (select(STDIN_FILENO + 1, &read_fds, &write_fds, &except_fds, &timeout) == 1) {
        char buff[256] = { '\0' };
        int length = read(STDIN_FILENO, buff, 4);
        buff[length - 1] = '\0'; // removes the '\n'
        if(strIsNumber(buff)) {
            givenAns = atoi(buff);
        } else {
            write(STDOUT_FILENO, "La risposta inserita non e' un numero, ti verrà decurtato un punto\n", 68);
        }
    }
    else {
        write(STDOUT_FILENO, "\nATTENZIONE: Il tempo a disposizione per rispondere e' scaduto, ti verrà decurtato un punto\n\b", 94);
    }
    return givenAns;
}

/**
 * Prende un intero dal socket
 * e della lettura dal socket che può essere minore della grandezza dell'intero
 * @param fd il file descriptor del socket su cui leggere
 * @param num il valore numerico che si intende leggere dal socket (è un parametro di ritorno)
 * @return 0 se ha successo, -1 altrimenti
 */
int receiveInt(int fd, int* num){
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
 * Scrive un intero sul socket tenendo conto della differenza di rappresentazione di un int
 * @param fd il file descriptor del socket su cui scrivere
 * @param num il valore numerico che si intende scrivere sul socket
 * @return 0 se ha successo, -1 altrimenti
 */
int sendInt(int fd, int num){
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
 * Prende una stringa dal socket
 * @param fd il file descriptor del socket su cui leggere
 * @param str la stringa che si intende leggere dal socket (è un parametro di ritorno)
 * @param strLength la lunghezza della stringa
 * @return 0 se ha successo, -1 altrimenti
 */
int receiveString(int fd, char** str, int strLength){
    char* uncompleteStr = malloc(sizeof(char) * strLength);
    *str = calloc(strLength, sizeof(char));    // Alloco la stringa
    int bytesToReceive = strLength;
    int bytesReceived;

    // Ciclo finché non ricevo tutti i bytes necessari
    do {
        bytesReceived = read(fd, uncompleteStr, bytesToReceive);
        if(bytesReceived < 0) {
            free(uncompleteStr);
            return -1;
        }

        // Copio quello che ho letto finora nella stringa completeStr
        uncompleteStr[bytesReceived ] = '\0';
        strcat(*str, uncompleteStr);

        // Decremento il numero di byte che mi mancano da ricevere
        bytesToReceive -= bytesReceived;
    } while(bytesToReceive > 0);

    free(uncompleteStr);
    return 0;
}