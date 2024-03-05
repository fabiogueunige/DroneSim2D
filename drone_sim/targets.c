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

#define MAX_TARGETS 20

pid_t wd_pid = -1;
bool sigint_rec = false;

typedef struct {
    int x;
    int y;
    bool taken;
} targets;



void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "TARGETS: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "TARGETS: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
    if(signo == SIGINT){
        //pressed q or CTRL+C
        printf("TARGETS: Terminating with return value 0...");
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "TARGETS: terminating with return value 0...");
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
    FILE * tardebug = fopen("logfiles/targets.log", "w");

    char msg[100]; // for writing to log files
    
    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "TARGETS: process started");
    printf("TARGETS: process started\n");

    // opening pipes
    int pipeSefd[2];
    sscanf(argv[1], "%d", &pipeSefd[0]);
    sscanf(argv[2], "%d", &pipeSefd[1]);
    writeToLog(debug, "TARGETS: pipes opened");

    int rows, cols;
    
    
    // rows and cols and ntargets value is passed from server using pipes, now they will be initialized here
    
    //char pos_targets[ntargets][10];

    // SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "TARGETS: error in sigaction()");
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
    // Reads rows and cols from server
    if(read(pipeSefd[0], &rows, sizeof(int)) == -1){
        perror("error in reading from pipe");
        writeToLog(errors, "TARGETS: error in reading from pipe");
        exit(EXIT_FAILURE);
    }
    if(read(pipeSefd[0], &cols, sizeof(int)) == -1){
        perror("error in reading from pipe");
        writeToLog(errors, "TARGETS: error in reading from pipe");
        exit(EXIT_FAILURE);
    }
    sprintf(msg, "TARGETS: rows = %d, cols = %d", rows, cols);
    writeToLog(tardebug, msg);
    targets *target[MAX_TARGETS];
    int ntargets;
    sleep(2);
    while(!sigint_rec){
        time_t t = time(NULL);
        srand(time(NULL)); // for change every time the seed of rand()
        ntargets = rand() % MAX_TARGETS;
        sprintf(msg, "TARGETS: ntargets = %d", ntargets);
        writeToLog(tardebug, msg);
        char pos_targets[ntargets][10];
        //targets *target[ntargets];
        
        for(int i = 0; i<ntargets; i++){
            target[i] = malloc(sizeof(targets));
            // generates random coordinates for targets
            // i put targets away from edges because if they are too close to it coulb be a problem to take them due to repulsive force
            target[i]->x = rand() % (cols-4) + 2;
            target[i]->y = rand() % (rows-4) + 2;
            target[i]->taken = false;
            sprintf(pos_targets[i], "%d,%d", target[i]->x, target[i]->y);
            writeToLog(tardebug, pos_targets[i]);
            printf("TARGETS: target %d: x = %d, y = %d\n", i, target[i]->x, target[i]->y);
            //sprintf(pos_targets[i], "%d,%d", targets[i].x, targets[i].y);
            
        }
        if ((write(pipeSefd[1], &ntargets, sizeof(int))) == -1){
            perror("error in writing to pipe");
            writeToLog(errors, "TARGETS: error in writing to pipe");
        }
        for(int i = 0; i<ntargets; i++){
            if ((write(pipeSefd[1], target[i],sizeof(targets))) == -1){
                perror("error in writing to pipe");
                writeToLog(errors, "TARGETS: error in writing to pipe");
            }
        }
        time_t t2 = time(NULL);
        while(t2-t < 60){
            t2 = time(NULL);
        }
    }

    // closing pipes
    for (int i = 0; i < 2; i++){
        if (close(pipeSefd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "TARGETS: error in closing pipe");
        }
    }
    // freeing memory
    for(int i = 0; i<20; i++){
        free(target[i]);
    }
    fclose(debug);
    fclose(errors);
    fclose(tardebug);

    return 0;
}