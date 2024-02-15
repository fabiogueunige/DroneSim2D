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
#define MAX_MSG_LEN 1024
#define MAX_OBSTACLES 20

pid_t wd_pid = -1;
bool sigint_rec = false;

struct obstacle {
    int x;
    int y;
};

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "OBSTACLES: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "OBSTACLES: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
    if (signo == SIGINT){
        //pressed q or CTRL+C
        printf("OBSTACLE: Terminating with return value 0...");
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "OBSTACLE: terminating with return value 0...");
        fclose(debug);
        sigint_rec = true;
    }
    
}

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}


int main (int argc, char *argv[]) 
{
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    FILE * obsdebug = fopen("logfiles/obstacles.log", "w");
    // these var are used because there aren't pipes, but these values are imported by server
    char msg[100]; // message to write on debug file
    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }

    writeToLog(debug, "OBSTACLES: process started");
    printf("OBSTACLES: process started\n");
    sleep(1);
    // SOCKETS
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(50000);
    //char server_ip[100] = "130.251.107.87"; unige wifi
    // hotspot cell 192.168.39.210
    inet_pton(AF_INET, "130.251.107.87", &server_address.sin_addr);
    
    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("connect");
        writeToLog(errors, "OBSTACLES: error in connect()");
        return 1;
    }

    char * message = "OI";
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("send");
        return 1;
    }
    // Receive echo from server
    char buffer[MAX_MSG_LEN];
    ssize_t bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead == -1) {
        perror("recv");
        writeToLog(errors, "OBSTACLES: error in recv()");
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = '\0';
    printf("OBSTACLES: Server sent: %s\n", buffer);
    writeToLog(obsdebug, buffer);

    // Null-terminate the string
    if (strcmp(buffer, "OI") != 0) {
        writeToLog(errors, "OBSTACLES: server did not respond with OI");
        exit(EXIT_FAILURE);
    }
    bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead == -1) {
        perror("recv");
        writeToLog(errors, "OBSTACLES: error in recv()");
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = '\0'; // null terminate the strings
    printf("TARGETS: Server sent: %s\n", buffer);
    writeToLog(obsdebug, buffer);
    // save rows and cols
    char format_string[MAX_MSG_LEN]="%d,%d";
    int rows, cols;
    sscanf(buffer, format_string, &rows, &cols);

    // echo of rows and cols
    if (send(sock, buffer, strlen(buffer), 0) == -1) {
        perror("send");
        return 1;
    }
    writeToLog(debug, "OBSTACLES: rows and cols sent to server");
    
    // SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "INPUT: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
        perror("sigaction");
        writeToLog(errors, "INPUT: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    sprintf(msg, "OBSTACLES: rows = %d, cols = %d", rows, cols);
    writeToLog(obsdebug, msg);
    struct obstacle *obstacles[MAX_OBSTACLES];
    // obstacle generation cycle
    while(!sigint_rec){
        time_t t = time(NULL);

        

        srand(time(NULL));
        int nobstacles = rand() % MAX_OBSTACLES;
        printf("OBSTACLES: number of obstacles = %d\n", nobstacles);
        sprintf(msg, "OBSTACLES: number of obstacles = %d", nobstacles);
        writeToLog(obsdebug, msg);
        char pos_obstacles[nobstacles][10];
        //struct obstacle *obstacles[nobstacles];

        char obstacleStr[1024] = "";
        char temp[50];
        // Add number of targets to the string
        sprintf(temp, "O[%d]", nobstacles);
        strcat(obstacleStr, temp);
/*
        if ((write(pipeSefd[1], &nobstacles, sizeof(int))) == -1){ // implementare lettura su server
            perror("error in writing to pipe");
            writeToLog(errors, "OBSTACLES: error in writing to pipe number of obstacles");
            exit(EXIT_FAILURE);
        }*/
        
        // create obstacles
        for (int i = 0; i < nobstacles; i++){
            obstacles[i] = malloc(sizeof(struct obstacle)); //allocate memory for each obstacle
            // generates random coordinates
            obstacles[i]->x = rand() % (cols-2) + 1; 
            obstacles[i]->y = rand() % (rows-2) + 1;
            int x = obstacles[i]->x;
            int y = obstacles[i]->y;
            printf("OBSTACLES: obstacle %d created at (%d, %d)\n", i, x, y);
            sprintf(msg, "OBSTACLES: obstacle %d created at (%d, %d)\n", i, x, y);
            writeToLog(obsdebug, msg);
            //sprintf(pos_obstacles[i], "%d,%d", x, y);
            // write to server with pipe
            /*
            if (write(pipeSefd[1], obstacles[i], sizeof(struct obstacle)) == -1){
                perror("error in writing to pipe");
                writeToLog(errors, "OBSTACLES: error in writing to pipe obstacles");
                exit(EXIT_FAILURE);
            }*/
            sprintf(temp, "%d,%d|", obstacles[i]->x, obstacles[i]->y);
            strcat(obstacleStr, temp);
        }
        // Remove the last '|' character
        obstacleStr[strlen(obstacleStr) - 1] = '\0';
        writeToLog(obsdebug, obstacleStr);
        
        // wait 60 seconds before generating new obstacles
        time_t t2 = time(NULL); 
        while(t2 - t < 60){
            t2 = time(NULL);
        }
    }

    // close the socket
    close(sock);
    // free the memory allocated for obstacles
    for(int i = 0; i<20; i++){
        free(obstacles[i]);
    }
    // close log files
    fclose(debug);
    fclose(errors);
    fclose(obsdebug);
    return 0;
}