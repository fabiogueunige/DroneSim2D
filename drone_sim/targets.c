#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_TARGETS 20
#define MAX_MSG_LEN 1024

pid_t wd_pid = -1;
bool sigint_rec = false;

typedef struct {
    float x;
    float y;
    bool taken;
} targets;

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}


void Receive(int sockfd, char *buffer, FILE *debug) {
    /*
    Function for receiving a message from the client and echoing it back to the client. 
    */
    FILE *error = fopen("logfiles/errors.log", "a");
    if(recv(sockfd, buffer, MAX_MSG_LEN, 0) < 0) {
        writeToLog(error, "SOCKSERVER: Error receiving message from client");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, buffer);
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
        writeToLog(error, "TARGETS: error in sending message to server");
        exit(EXIT_FAILURE);
    }
    char recvmsg[MAX_MSG_LEN];
    if (recv(sock, recvmsg, MAX_MSG_LEN, 0) < 0) {
        perror("recv");
        writeToLog(error, "TARGETS: error in receiving message from server");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "Message echo:");
    writeToLog(debug, recvmsg);
    if(strcmp(recvmsg, msg) != 0){
        writeToLog(error, "TARGETS: echo is not equal to the message sent");
        exit(EXIT_FAILURE);
    }
    fclose(error);
}


int main (int argc, char *argv[]) 
{
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    FILE * tardebug = fopen("logfiles/targets.log", "w");

    char msg[100]; // for writing to log files
    targets *target[MAX_TARGETS];
    int ntargets = 0;
    struct sockaddr_in server_address;

    struct hostent *server;
    int port = 40000; // default port 
    int sock;
    char sockmsgt[MAX_MSG_LEN];
    float r,c;
    int rows = 0, cols = 0;
    char stop[] = "STOP";
    char message [] = "TI";
    char ge[] = "GE";
    bool stopReceived = false;
    bool ge_flag = false;
    fd_set readfds;
    FD_ZERO(&readfds);
    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "TARGETS: process started");
    printf("TARGETS: process started\n");

    sscanf(argv[1], "%d", &port);
    sprintf(msg, "TARGETS: port = %d", port);
    writeToLog(tardebug, msg);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        writeToLog(errors, "TARGETS: error in creating socket");
        return 1;
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);  

    // convert the string in ip address
    if ((inet_pton(AF_INET, argv[2], &server_address.sin_addr)) < 0) {
        perror("inet_pton");
        writeToLog(errors, "TARGETS: error in inet_pton() in converting IP address");
        return 1;
    }
    
    // Connect to the server
    if ((connect(sock, (struct sockaddr*)&server_address, sizeof(server_address))) == -1) {
        perror("connect");
        writeToLog(errors, "TARGETS: error in connecting to server");
        return 1;
    }
    writeToLog(debug, "TARGETS: connected to serverSocket");

    
    writeToLog(tardebug, message);
    Send(sock, message, tardebug);
    // receiving rows and cols from server
    memset(sockmsgt, '\0', MAX_MSG_LEN);
    Receive(sock, sockmsgt, tardebug);
    // setting rows and cols
    char *format = "%f,%f";
    sscanf(sockmsgt, format, &r, &c);
    rows = (int)r;
    cols = (int)c;
    printf("TARGETS: rows = %d, cols = %d\n", rows, cols);
    sleep(2);
    while(!stopReceived){
        time_t t = time(NULL);
        ge_flag = false;
        srand(time(NULL)); // for change every time the seed of rand()
        ntargets = rand() % (MAX_TARGETS-1)+1;
        sprintf(msg, "TARGETS: ntargets = %d", ntargets);
        writeToLog(tardebug, msg);
        char pos_targets[ntargets][10];
        char targetStr[MAX_MSG_LEN];
        char temp[50];
        // add number of targets to the string
        sprintf(targetStr, "T[%d]", ntargets);
        if (ntargets == 0){
            strcat(targetStr, "|");
        }
        for(int i = 0; i<ntargets; i++){
            target[i] = malloc(sizeof(targets));
            // generates random coordinates for targets
            // i put targets away from edges because if they are too close to it coulb be a problem to take them due to repulsive force
            target[i]->x = rand() % (cols-4) + 2;
            target[i]->y = rand() % (rows-4) + 2;
            target[i]->taken = false;
            sprintf(temp, "%.3f,%.3f|", target[i]->x, target[i]->y);
            strcat(targetStr, temp);
            sprintf(msg,"TARGETS: target %d: x = %.3f, y = %.3f\n", i, target[i]->x, target[i]->y);
            writeToLog(tardebug, msg);
        }
        targetStr[strlen(targetStr)-1] = '\0'; // remove the last |
        writeToLog(tardebug, targetStr);

        // Send the targets to the socket server
        Send(sock, targetStr, tardebug);
        int sel;
        while(!ge_flag){
            FD_SET(sock, &readfds);
            do{
                sel = select(sock+1, &readfds, NULL, NULL, NULL);
            }
            while(sel<0 && errno==EINTR);
            if (sel<0){
                writeToLog(errors, "TARGETS: error in select");
                perror("select");
                exit(EXIT_FAILURE);
            }
            else if (sel>0){
                if(FD_ISSET(sock, &readfds)){
                    writeToLog(tardebug, "Reading message sent via socket");
                    char buffer[MAX_MSG_LEN];
                    Receive(sock, buffer, tardebug);
                    if(strcmp(buffer, stop) == 0){
                        writeToLog(tardebug, "TARGETS: STOP message received from server");
                        stopReceived = true;
                        ge_flag = true;
                    }
                    else if(strcmp(buffer, ge) == 0){
                        ge_flag = true;
                        writeToLog(tardebug, "TARGETS: GE message received from server");
                    }
                }
            }
        }
    }
    writeToLog(tardebug, "TARGETS: exiting with return value 0");
    // freeing memory
    for(int i = 0; i<ntargets; i++){
        free(target[i]);
    }
    close(sock);
    fclose(debug);
    fclose(errors);
    fclose(tardebug);

    return 0;
}