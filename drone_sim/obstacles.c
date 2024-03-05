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
#include <errno.h>

#include <sys/select.h>


#define MAX_OBSTACLES 20
#define MAX_MSG_LEN 1024

pid_t wd_pid = -1;
bool sigint_rec = false;

struct obstacle {
    float x;
    float y;
};


void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}

void Send(int sock, char *msg, FILE *obsdebug){
    FILE *error = fopen("logfiles/errors.log", "a");
    if (send(sock, msg, strlen(msg) + 1, 0) == -1) {
        perror("send");
        writeToLog(error, "TARGETS: error in sending message to server");
        exit(EXIT_FAILURE);
    }
    writeToLog(obsdebug, msg);
    char recvmsg[MAX_MSG_LEN];
    if (recv(sock, recvmsg, MAX_MSG_LEN, 0) < 0) {
        perror("recv");
        writeToLog(error, "TARGETS: error in receiving message from server");
        exit(EXIT_FAILURE);
    }
    writeToLog(obsdebug, "Message echo:");
    writeToLog(obsdebug, recvmsg);
    if(strcmp(recvmsg, msg) != 0){
        writeToLog(error, "TARGETS: echo is not equal to the message sent");
        exit(EXIT_FAILURE);
    }
    fclose(error);
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
    //char msg[MAX_MSG_LEN] = buffer;
    if(send(sockfd, buffer, strlen(buffer)+1, 0) < 0) {
        writeToLog(error, "SOCKSERVER: Error sending message to client");
        exit(EXIT_FAILURE);
    }
    fclose(error);
}

int main (int argc, char *argv[]) 
{
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    FILE * obsdebug = fopen("logfiles/obstacles.log", "w");
    char msg[100]; // message to write on debug file
    struct sockaddr_in server_address;
    
    struct hostent *server;
    int port = 40000; // default port
    int sock;
    char sockmsgo[MAX_MSG_LEN];
    float r,c;
    int rows = 0, cols = 0;
    char stop[] = "STOP";
    char message [] = "OI";
    bool stopReceived = false;
    int nobstacles = 0;
    struct obstacle *obstacles[MAX_OBSTACLES];
    fd_set readfds;
    FD_ZERO(&readfds);

    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }

    writeToLog(debug, "OBSTACLES: process started");
    
    sscanf(argv[1], "%d", &port);
    sprintf(msg, "OBSTACLES: port = %d", port);
    writeToLog(obsdebug,msg);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        writeToLog(errors, "OBSTACLES: error in creating socket");
        return 1;
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);  

    // convert the string in ip address
    if ((inet_pton(AF_INET, argv[2], &server_address.sin_addr)) <0) {
        perror("inet_pton");
        writeToLog(errors, "OBSTACLES: error in inet_pton() in converting IP address");
        return 1;
    }
    
    // Connect to the server
    if ((connect(sock, (struct sockaddr*)&server_address, sizeof(server_address))) == -1) {
        perror("connect");
        writeToLog(errors, "OBSTACLES: error in connecting to server");
        return 1;
    }
    writeToLog(debug, "OBSTACLES: connected to serverSocket");
    memset(sockmsgo, '\0', MAX_MSG_LEN);
    
    Send(sock, message, obsdebug);
    // receiving rows and cols from server
    Receive(sock, sockmsgo, obsdebug);
    // setting rows and cols
    char *format = "%f,%f";
    sscanf(sockmsgo, format, &r, &c);
    rows = (int)r;
    cols = (int)c;
    printf("OBSTACLES: rows = %d, cols = %d\n", rows, cols);
    
    sleep(1);
    int sel;
    while(!stopReceived){
        srand(time(NULL));
        nobstacles = rand() % MAX_OBSTACLES;
        printf("OBSTACLES: number of obstacles = %d\n", nobstacles);
        sprintf(msg, "OBSTACLES: number of obstacles = %d", nobstacles);
        writeToLog(obsdebug, msg);
        char pos_obstacles[nobstacles][10];
        
        char obstacleStr[MAX_MSG_LEN];
        char temp[50];
        // add number of targets to the string
        sprintf(obstacleStr, "O[%d]", nobstacles);
        if (nobstacles == 0){
            strcat(obstacleStr, "|");
        }
        // create obstacles
        for (int i = 0; i < nobstacles; i++){
            obstacles[i] = malloc(sizeof(struct obstacle)); //allocate memory for each obstacle
            // generates random coordinates
            obstacles[i]->x = rand() % (cols-2) + 1; 
            obstacles[i]->y = rand() % (rows-2) + 1;
            sprintf(msg, "OBSTACLES: obstacle %d created at (%.3f, %.3f)\n", i, obstacles[i]->x, obstacles[i]->y);
            writeToLog(obsdebug, msg);
            sprintf(temp, "%.3f,%.3f|", obstacles[i]->x, obstacles[i]->y);
            strcat(obstacleStr, temp);
        }
        obstacleStr[strlen(obstacleStr)-1] = '\0'; // remove the last |
        writeToLog(obsdebug, obstacleStr);

        // Sending the obstacles to the socket server
        Send(sock, obstacleStr, obsdebug);
        
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        FD_SET(sock, &readfds);
        do {
            sel = select(sock+1, &readfds, NULL, NULL, &timeout);
        }
        while(sel<0 && errno==EINTR);
        if (sel<0){
            writeToLog(errors, "OBSTACLE: error in select");
            perror("select");
            exit(EXIT_FAILURE);
        }
        else if (sel>0){
            if(FD_ISSET(sock, &readfds)){
                writeToLog(obsdebug, "Reading message sent via socket");
                char buffer[MAX_MSG_LEN];
                Receive(sock, buffer, obsdebug);
                if(strcmp(buffer, stop) == 0){
                    writeToLog(obsdebug, "OBSTACLES: STOP message received from server");
                    stopReceived = true;
                }
            }
        }
        else{
            writeToLog(obsdebug, "OBSTACLES: timeout expired");
        }
        
    }
    writeToLog(obsdebug, "OBSTACLES: exiting with return value 0");

    // free the memory allocated for obstacles
    for(int i = 0; i<nobstacles; i++){
        free(obstacles[i]);
    }
    // close log files
    close(sock);
    fclose(debug);
    fclose(errors);
    fclose(obsdebug);
    return 0;
}