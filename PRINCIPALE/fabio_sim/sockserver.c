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

int main (int argc, char *argv[]) {

    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    
    writeToLog(debug, "Socket server started");
    char msg[100]; // for writing to log files
    
    int pipeSe[2];
    int sockfd;
    int identifier; // to know which is the child process (0 = TA, 1 = OBS)


    sscanf(argv[4], "%d", &identifier);
    sprintf(msg, "logfiles/socketServer%d.log",identifier);

    FILE * sockdebug = fopen(msg, "w");
    writeToLog(sockdebug, "Socket server started");
    

    sscanf(argv[1], "%d", &sockfd);
    sscanf(argv[2], "%d", &pipeSe[0]);
    sscanf(argv[3], "%d", &pipeSe[1]);
    writeToLog(sockdebug, "Socket server pipe created");

    char buffer[MAX_MSG_LEN];
    if ((recv(sockfd, buffer, strlen(buffer), 0)) < 0) {
        writeToLog(errors, "Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(sockdebug, "Message received from client");
    writeToLog(sockdebug, buffer);
    
    if ((write(pipeSe[1], buffer, strlen(buffer))) < 0) {
        writeToLog(errors, "Error writing to pipe the message information");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    close(pipeSe[0]);
    close(pipeSe[1]);

    return 0;

}