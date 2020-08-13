#ifndef UNTITLED_UTILITY_H
#define UNTITLED_UTILITY_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

/* *************************** Costanti ****************************/
#define MAX_TIME 60

/* *************************** Funzioni ****************************/

bool strIsNumber(const char*);

int getAnswerWithTimeout();

int receiveInt(int, int*);

int sendInt(int, int);

int sendInt(int, int);

int receiveString(int, char**, int);



#endif //UNTITLED_UTILITY_H
