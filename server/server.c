#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/un.h>
#include <stdbool.h>
#include "serverUtility.h"

/* *********************************************|   COSTANTI   |********************************************* */
/*dichiaro delle costanti che rappresentano rispettivamente il numero massimo di client supportati*/
#define MAX_CLIENTS 15
#define READ_TIME 0

/* **************************** Variabili Globali e Mutex ******************************/

//identifica se il gioco è iniziato o meno per far partire la prima sveglia
bool gameHasStarted = false;
pthread_mutex_t startGameMutex = PTHREAD_MUTEX_INITIALIZER;

//servono per l'attesa dei giocatori e l'inizio di gioco
int readyPlayers = 0;
pthread_mutex_t waitGameStartMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitGameStartCondition = PTHREAD_COND_INITIALIZER;

//servono per l'attesa della risposta del singolo giocatore
threadVariables* threadWakeups[MAX_CLIENTS] = { NULL };
int threadWakeupsIndex = 0;
pthread_mutex_t threadWakeupMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t responsePlayerCondition = PTHREAD_COND_INITIALIZER;

// Rappresentano il numero di giocatori e la leaderboard
numberOfPlayers* playersTotal;
List* listOfPlayers;

/* *************************************|   FUNZIONI   |************************************** */

// Funzione del singolo thread
void* start(void*);

// Funzioni relative all'array globale threadWakeups
bool addToThreadWakeups(threadVariables*);
bool removeFromThreadWakeups(threadVariables*);

/* *********************************************| MAIN |********************************************* */

int main(void){

    //Creo la lista che mi codifica la classifica
    listOfPlayers = createLeaderboard();
    signal(SIGALRM, signalHandler);

    playersTotal = init_struct();
    //1.Creo socket e controllo che la creazione sia avvenuta con successo
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0){
        perror("Socket\n");
        exit(1);
    }

    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(49154);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //imposto a zero tutto il resto
    memset(&(sockAddr.sin_zero), '\0', 8);

    //2.Assegnamento di un indirizzo
    int addr = bind(serverSocket, (struct sockaddr*)&sockAddr, sizeof(struct sockaddr));
    if(addr < 0){
        perror("Unable to bind\n");
        close(serverSocket);
        exit(1);
    }

    //3.Si mette in ascolto
    int lst = listen(serverSocket, MAX_CLIENTS);
    if(lst < 0){
        perror("Listen\n");
        close(serverSocket);
        exit(1);
    }

    //Accetto le connessioni dei client
    int clientSocket;
    struct sockaddr_in clientAddress;
    socklen_t clientLength;
    pthread_t tid[MAX_CLIENTS];
    int i = 0;
    while(1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLength);
        if(clientSocket < 0) {
            perror("Accept\n");
            exit(1);
        }

        //mi ritorno l'indirizzo del client in formato char e lo inserisco nella lista
        char *clientAddr = inet_ntoa(clientAddress.sin_addr);
        if(addUserToLeaderboard(listOfPlayers, clientAddr, 0)){
            write(STDOUT_FILENO, "Client inserito con successo\n", 29);
        } else {
            write(STDOUT_FILENO, "Non e' stato possibile inserire il client/client già presente\n", 62);
        }

        if(clientSocket < 0) {
            perror("Connection refused\n");
            close(serverSocket);
            exit(1);
        }

        pthread_mutex_lock(playersTotal->mutex);
        playersTotal->numPlayers = playersTotal->numPlayers + 1;
        pthread_mutex_unlock(playersTotal->mutex);

        threadVariables* tVars = malloc(sizeof(threadVariables));
        tVars->clientSocket = clientSocket;
        strcpy(tVars->clientIp, clientAddr);
        tVars->threadIsWaiting = false;

        addToThreadWakeups(tVars);

        if(pthread_create(&tid[i % MAX_CLIENTS], NULL, start, tVars)){
            write(STDOUT_FILENO, "Errore creazione di thread\n", 27);
            exit(1);
        }
        i++;
    }
}

/* *************************************|   FUNZIONE THREAD   |************************************** */

void* start(void* sockPtr){

    threadVariables* tVars = (threadVariables*)sockPtr;
    int sockAddr = tVars->clientSocket;

    // Se ricevo questo intero allora il client è pronto a giocare
    int isThePlayerReady = 0;
    receiveInt(sockAddr, &isThePlayerReady);

    pthread_mutex_lock(&waitGameStartMutex);
    readyPlayers++;
    pthread_cond_broadcast(&waitGameStartCondition);
    while(gameCannotStart()){
        pthread_cond_wait(&waitGameStartCondition, &waitGameStartMutex);
    }
    pthread_mutex_unlock(&waitGameStartMutex);

    startFirstAlarm();
    setTimeoutOnSocket(sockAddr, READ_TIME);

    // Apro il file
    int fd = open("questions.txt", O_RDONLY);
    if(fd < 0){
        perror("Open\n");
        exit(1);
    }

    // Leggo le info associate al file in input
    struct stat fileStat;
    if(stat("questions.txt", &fileStat) < 0) {
        perror("stat");
        exit(1);
    }

    // Controllo che la dimensione non sia 0
    int fileDim = fileStat.st_size;
    if(fileDim == 0) {
        write(STDERR_FILENO, "Errore: File vuoto", 18);
        exit(1);
    }

    char buff[512] = {'\0'};
    char ans = '\0';
    int nbytes;
    int readChars = 0, posInFile = 0, correctAns = 0;

    while(posInFile < fileDim) {
        readChars = 0;
        correctAns = 0;
        memset(buff, '\0', sizeof(char) * 512);
        /*Mediante questo do-while la domanda viene inviata al client
         * e salvo la risposta corretta in una variabile con cui farò il confronto*/
        do {
            nbytes = read(fd, &buff[readChars], 1);
            if(nbytes <= 0){
                break;
            }
            if (buff[readChars] == '~') {
                posInFile++;
                buff[readChars]='\0';
                read(fd, &ans, 1);
                correctAns = atoi(&ans);
                posInFile++;
            }
            posInFile++; readChars++;
        } while (correctAns == 0);
        lseek(fd, posInFile, SEEK_SET);

        if(readChars == 0){
            break;
        }

        if(sendInt(sockAddr, readChars) < 0){
            perror("Send readChars");
            exit(1);
        }
        if(sendString(sockAddr, buff, readChars) < 0){
            perror("send question");
            exit(1);
        }

        sortLeaderboard(&(listOfPlayers->head), listOfPlayers->dim);
        printLeaderboard(listOfPlayers, sockAddr);

        // Imposto il thread in attesa finché la sveglia non parte
        tVars->threadIsWaiting = true;

        /* Metto in attesa il thread finché non riceve la broadcast dalla sveglia, nel caso
         * di spurious wakeup tornerà in attesa */
        pthread_mutex_lock(&threadWakeupMutex);
        while(tVars->threadIsWaiting) {
            pthread_cond_wait(&responsePlayerCondition, &threadWakeupMutex);
        }
        pthread_mutex_unlock(&threadWakeupMutex);

        /*leggo la risposta del client mediante la funzione receiveInt. Inizializzo la variabile receivedAns a -1 in
         * modo che in caso di risposta non data essa contenga già il valore corretto*/
        int receivedAns = -1;
        receiveInt(sockAddr, &receivedAns);
        if(receivedAns < 0 || receivedAns != correctAns){
            setPlayerScore(listOfPlayers, tVars->clientIp, 0);
        } else {
            setPlayerScore(listOfPlayers, tVars->clientIp, 1);
        }
        sleep(1);
    }
    sendInt(sockAddr, -1);
    removeFromThreadWakeups(tVars);
    free(tVars);
    return NULL;
}

/* *************************************|   FUNZIONE DI SUPPORTO   |************************************** */

/**
 * Fa partire la sveglia all'inizio della partita
 */
void startFirstAlarm(){
    pthread_mutex_lock(&startGameMutex);
    if(!gameHasStarted){
        alarm(ALARM_TIME);
        gameHasStarted = true;
    }
    pthread_mutex_unlock(&startGameMutex);
}

/**
 * Funzione utilizzata per stabilire quando il gioco è pronto a partire, e cioè quando ho almeno due giocatori
 * */
int gameCannotStart() {
    pthread_mutex_lock(playersTotal->mutex);
    bool status = playersTotal->numPlayers < 2 || playersTotal->numPlayers != readyPlayers;
    pthread_mutex_unlock(playersTotal->mutex);
    return status;
}

/**
 * Imposta la variabile threadIsSleeping dei singoli thread a false
 */
void setAllThreadsToWakeup() {
    pthread_mutex_lock(&threadWakeupMutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if(threadWakeups[i] != NULL) {
            threadWakeups[i]->threadIsWaiting = false;
        }
    }

    pthread_mutex_unlock(&threadWakeupMutex);
}

bool addToThreadWakeups(threadVariables* tvar) {
    pthread_mutex_lock(&threadWakeupMutex);

    int index = threadWakeupsIndex % MAX_CLIENTS;
    bool isAdded;
    if(threadWakeups[index] == NULL) {
        threadWakeups[index] = tvar;
        threadWakeupsIndex++;
        isAdded = true;
    }
    else {
        isAdded = false;
    }

    pthread_mutex_unlock(&threadWakeupMutex);
    return isAdded;
}

bool removeFromThreadWakeups(threadVariables* tvar) {
    pthread_mutex_lock(&threadWakeupMutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if(threadWakeups[i] != NULL && strcmp(threadWakeups[i]->clientIp, tvar->clientIp) == 0) {
            threadWakeups[i] = NULL;
            pthread_mutex_unlock(&threadWakeupMutex);
            return true;
        }
    }

    pthread_mutex_unlock(&threadWakeupMutex);
    return false;
}

/**
 * Handler di SIGALRM, sveglia i thread in attesa e imposta la prossima sveglia
 * */
void signalHandler(int receivedSignal){
    if(receivedSignal == SIGALRM) {
        setAllThreadsToWakeup();
        pthread_cond_broadcast(&responsePlayerCondition);
        alarm(ALARM_TIME);
    }
}