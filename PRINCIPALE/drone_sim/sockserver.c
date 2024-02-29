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

void Receive(int sockfd, char *buffer, int *pipetowritefd, FILE *sckfile) {
    /*
    Function for receiving a message from the client and echoing it back to the client. 
    It also writes to its parent the message received through the pipe given as input.
    */
    FILE *error = fopen("logfiles/errors.log", "a");
    if(recv(sockfd, buffer, MAX_MSG_LEN, 0) < 0) {
        writeToLog(error, "SOCKSERVER: Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(sckfile, buffer);
    //char msg[MAX_MSG_LEN] = buffer;

    if(write(*pipetowritefd, buffer, strlen(buffer)+1) < 0) {
        writeToLog(error, "SOCKSERVER: Error writing to pipe the message information");
        exit(EXIT_FAILURE);
    }
    if(send(sockfd, buffer, strlen(buffer)+1, 0) < 0) {
        writeToLog(error, "SOCKSERVER: Error sending message to client");
        exit(EXIT_FAILURE);
    }
    fclose(error);
}

void Send(int sock, char *msg, FILE *debug){
    /*
    Function to send a message and receive an echo.
    */
    FILE *error = fopen("logfiles/errors.log", "a");
    if (send(sock, msg, strlen(msg) + 1, 0) == -1) {
        perror("send");
        writeToLog(error, "SOCKSERVER: error in sending message to server");
        exit(EXIT_FAILURE);
    }
    char recvmsg[MAX_MSG_LEN];
    if (recv(sock, recvmsg, MAX_MSG_LEN, 0) < 0) {
        perror("recv");
        writeToLog(error, "SOCKSERVER: error in receiving message from server");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "Message echo:");
    writeToLog(debug, recvmsg);
    if(strcmp(recvmsg, msg) != 0){
        writeToLog(error, "SOCKSERVER: echo is not equal to the message sent");
        exit(EXIT_FAILURE);
    }
    fclose(error);
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

    sscanf(argv[4], "%d", &identifier); // the server identifies which child is receiving targets and which obs
    sprintf(msg, "logfiles/socketServer%d.log",identifier);

    FILE * sockdebug = fopen(msg, "w");
    writeToLog(sockdebug, "Socket server started");
    // imoprt the pipe and sock fd from the parent process
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
    
    fd_set readfds;
    FD_ZERO(&readfds);
    if ((write(pipeSe[1], msg, strlen (msg) + 1)) < 0) {
        writeToLog(errors, "Error writing to pipe the message information");
        exit(EXIT_FAILURE);
    }
    writeToLog(sockdebug, "Message sent to parent process");
    writeToLog(sockdebug, msg);

    // Sending rows and cols to the clients
    if ((send(sockfd, argv[5], strlen(argv[5]) + 1, 0)) < 0) {
        perror("Error sending rows and cols to client");
        writeToLog(sockdebug, "SOCKSERVER: Error sending rows and cols to client");
        exit(EXIT_FAILURE);
    }
    while (!stopReceived) {

        FD_SET(pipeSe[0], &readfds);
        FD_SET(sockfd, &readfds);
        int maxfd;
        if(pipeSe[0] > sockfd) {
            maxfd = pipeSe[0];
        }
        else {
            maxfd = sockfd;
        }
        int sel = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if(sel < 0) {
            writeToLog(errors, "Error in select");
            perror("Error in select");
            exit(EXIT_FAILURE);
        }
        else if(sel > 0) {
            if(FD_ISSET(sockfd, &readfds)){
                memset(msg, '\0', MAX_MSG_LEN);
                Receive(sockfd, msg, &pipeSe[1], sockdebug);
            }
            if(FD_ISSET(pipeSe[0], &readfds)) {
                memset(msg, '\0', MAX_MSG_LEN);
                if(read(pipeSe[0], msg, MAX_MSG_LEN) < 0) {
                    writeToLog(errors, "SOCKSERVER: Error reading from pipe");
                    perror("Error reading from pipe");
                    exit(EXIT_FAILURE);
                }
                writeToLog(sockdebug, "SOCKSERVER: Message received from server");
                writeToLog(sockdebug, msg);
                Send(sockfd, msg, sockdebug);
                writeToLog(sockdebug, "SOCKSERVER: Message sent to client");
            }
        }
        else {
            writeToLog(sockdebug, "SOCKSERVER: Timeout expired");
        }
        // IMPLEMENTA LA SELECT PER LA LETTURA DELLA PIPE con timeout
        if (strcmp(msg, stop) == 0) {
            stopReceived = true;
        }

    }
    writeToLog(sockdebug, "SOCKSERVER: Exiting with return value 0");


    close(sockfd);
    close(pipeSe[0]);
    close(pipeSe[1]);

    return 0;

}