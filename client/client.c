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
#include "clientUtility.h"

#define MAX_INPUT 10
#define printStars write(STDOUT_FILENO, "*****************************************************************************************\n", 90);
#define printNewLine write(STDOUT_FILENO, "\n", 1);

int getServerMessages(int fd);

int main(int argc, char* argv[]){

    if(argc!=3){
        write(STDOUT_FILENO, "Numero di parametri in input errato\n", 36);
        exit(1);
    }

    //1.Creo il socket
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd<0){
        perror("Socket");
        exit(1);
    }
    //puntatore alla struttura degli indirizzi
    struct sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    int port = atoi(argv[2]);
    serverAddress.sin_port = htons(port);

    inet_aton(argv[1], &serverAddress.sin_addr);
    memset(&(serverAddress.sin_zero), '\0', 8);

    //2.Connetto al server
    int connection = connect(fd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(connection<0){
        perror("Connect");
        exit(EXIT_FAILURE);
    }

    //3.Interazione
    printStars
    write(STDOUT_FILENO, "Benvenuto in questo gioco, ti verranno poste una serie di domande a risposta multipla\n", 86);
    write(STDOUT_FILENO, "- per ogni risposta corretta avrai +1 punto\n", 44);
    write(STDOUT_FILENO, "- per ogni risposta errata/non data ti verrà decurtato 1 punto\n", 64);
    write(STDOUT_FILENO, "Il gioco avrà inizio quando ci saranno almeno due sfidanti connessi\n", 69);
    printStars

    char response[MAX_INPUT] = {'\0'};
    do {

        write(STDOUT_FILENO, "se sei pronto digita un numero e premi invio\n", 45);
        int length = read(STDIN_FILENO, &response, MAX_INPUT);
        response[length-1]='\0';
    } while(!strIsNumber(response));
    int isThePlayerReady = atoi(response);
    sendInt(fd, isThePlayerReady);
    getServerMessages(fd);
}

int getServerMessages(int fd) {

    int questionLen=-1;
    do {
        if (receiveInt(fd, &questionLen) < 0) {
            perror("read question len\n");
            exit(1);
        }

        if (questionLen == -1) {
            break;
        }

        char *buff = NULL;
        if (receiveString(fd, &buff, questionLen) < 0) {
            perror("Read Question");
            exit(1);
        }

        printNewLine
        printStars
        write(STDOUT_FILENO, buff, questionLen - 1);
        printNewLine

        //Procedo a stampare all'utente la classifica di gioco
        int playerNum = -1;
        if (receiveInt(fd, &playerNum) < 0) {
            perror("Read Players Num");
            exit(1);
        }
        char *ip;
        int iplen = 0;
        int points = -1;
        char printer[128];
        write(STDOUT_FILENO, "                       CLASSIFICA ATTUALE\n", 42);
        for (int i = 0; i < playerNum; i++) {
            receiveInt(fd, &iplen);
            receiveString(fd, &ip, iplen);
            receiveInt(fd, &points);

            sprintf(printer, "Posizione %d - Ip giocatore: %s - punteggio: %d\n", i + 1, ip, points);
            write(STDOUT_FILENO, printer, strlen(printer));
            free(ip);
        }
        printStars
        write(STDOUT_FILENO, "Inserisci risposta\n", 19);
        int userAns = getAnswerWithTimeout();
        sendInt(fd, userAns);

    }while(questionLen!=-1);

    printStars;
    write(STDOUT_FILENO, "Fine del gioco\n", 15);
    printStars;
    close(fd);
    return 0;
}