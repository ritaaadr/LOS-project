#ifndef PROGETTO_LSO_UTILITY_H
#define PROGETTO_LSO_UTILITY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/un.h>
#include <stdbool.h>
#include "list.h"

/* *************************** Costanti ****************************/

#define ALARM_TIME 60


/* **************************** Struct *****************************/

typedef struct numberOfPlayers{
    int numPlayers;
    pthread_mutex_t* mutex;
} numberOfPlayers;

typedef struct threadVariables{
    int clientSocket;
    bool threadIsWaiting;
    char clientIp[17];
} threadVariables;

/* ************************** Funzioni ****************************/

void startFirstAlarm();

int gameCannotStart();

void signalHandler(int);

int sendInt(int, int);

int sendString(int, char*, int);

void setTimeoutOnSocket(int, int);

int receiveInt(int, int*);

numberOfPlayers* init_struct();

void setPlayerScore(List*, char*, bool);

void printLeaderboard(List*, int);

#endif //PROGETTO_LSO_UTILITY_H
