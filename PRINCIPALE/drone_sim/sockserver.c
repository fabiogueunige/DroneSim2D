#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include<stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

# define MAX_MSG_LEN 1024

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}
void Receive(int sockfd, char *buffer, int *pipetowritefd) {
    FILE * debug = fopen("logfiles/debug.log", "a");
    if(recv(sockfd, buffer, MAX_MSG_LEN, 0) < 0) {
        writeToLog(stderr, "Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, buffer);
    //char msg[MAX_MSG_LEN] = buffer;

    if(write(*pipetowritefd, buffer, strlen(buffer)+1) < 0) {
        writeToLog(stderr, "Error writing to pipe the message information");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "Message sent to parent process");
    if(send(sockfd, buffer, strlen(buffer)+1, 0) < 0) {
        writeToLog(stderr, "Error sending message to client");
        exit(EXIT_FAILURE);
    }
    fclose(debug);
}
int main (int argc, char *argv[]) {

    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    
    writeToLog(debug, "Socket server started");
    char msg[MAX_MSG_LEN]; // for writing to log files
    
    int pipeSe[2];
    int sockfd;
    int identifier; // to know which is the child process (0 = TA, 1 = OBS)
    char stop[] = "STOP";
    char ge[] = "GE";
    bool stopReceived = false;


    sscanf(argv[4], "%d", &identifier);
    sprintf(msg, "logfiles/socketServer%d.log",identifier);

    FILE * sockdebug = fopen(msg, "w");
    writeToLog(sockdebug, "Socket server started");
    
    // strlen = numero caratteri di una stringa
    // sizeof = numero di byte di un tipo di dato (non sicuro -> sostituidci)

    sscanf(argv[1], "%d", &sockfd);
    sscanf(argv[2], "%d", &pipeSe[0]);
    sscanf(argv[3], "%d", &pipeSe[1]);
    writeToLog(sockdebug, "Socket server pipe created");
    
    memset(msg, '\0', MAX_MSG_LEN);
    if ((recv(sockfd, msg, MAX_MSG_LEN, 0)) < 0) {
        writeToLog(errors, "Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(sockdebug, "Message received from client");
    writeToLog(sockdebug, msg);
    
    if ((write(pipeSe[1], msg, strlen (msg) + 1)) < 0) {
        writeToLog(errors, "Error writing to pipe the message information");
        exit(EXIT_FAILURE);
    }
    writeToLog(sockdebug, "Message sent to parent process");
    writeToLog(sockdebug, msg);
    
    // Sending rows and cols to the clients
    if ((send(sockfd, argv[5], strlen(argv[5]) + 1, 0)) < 0) {
        perror("Error sending rows and cols to client");
        writeToLog(errors, "Error sending rows and cols to client");
        writeToLog(sockdebug, "Error sending rows and cols to client");
        exit(EXIT_FAILURE);
    }

    memset(msg, '\0', MAX_MSG_LEN);
    Receive(sockfd, msg, &pipeSe[1]);
    /*if(recv(sockfd, msg, MAX_MSG_LEN, 0)==-1){
        perror("Error receiving message from client");
        writeToLog(errors, "Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(sockdebug,msg);*/


    /*while (!stopReceived) 
    {
        memset(msg, '\0', MAX_MSG_LEN);
        if ((recv(sockfd, msg, strlen(msg), 0)) < 0) {
            writeToLog(errors, "Error receiving message from client");
            exit(EXIT_FAILURE);
        }
        writeToLog(sockdebug, "Message received from client");
        writeToLog(sockdebug, msg);
        
        if ((write(pipeSe[1], msg, strlen(msg))) < 0) {
            writeToLog(errors, "Error writing to pipe the message information");
            exit(EXIT_FAILURE);
        }
        writeToLog(sockdebug, "Message sent to parent process");
        writeToLog(sockdebug, msg);
    }*/

    close(sockfd);
    close(pipeSe[0]);
    close(pipeSe[1]);

    return 0;

}