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

pid_t wd_pid = -1;
bool exit_flag = false;

typedef struct {
    int x;
    int y;
} targets;

typedef struct {
    int rows;
    int cols;
    int nobstacles;
    int ntargets;
} window;



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
    
}

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}


int main (int argc, char *argv[]) 
{
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    writeToLog(debug, "TARGETS: process started");
    printf("TARGETS: process started\n");

    // opening pipes
    int pipeSefd[2];
    sscanf(argv[1], "%d", &pipeSefd[0]);
    sscanf(argv[2], "%d", &pipeSefd[1]);
    writeToLog(debug, "TARGETS: pipes opened");

    int rows, cols, ntargets;
    
    
    // rows and cols and ntargets value is passed from server using pipes, now they will be initialized here
    rows = 100;
    cols = 100;
    ntargets = 10;
    targets targets[ntargets];
    char pos_targets[ntargets][10];


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

    for(int i = 0; i<ntargets; i++){
        targets[i].x = rand() % rows;
        targets[i].y = rand() % cols;
        printf("TARGETS: target %d: x = %d, y = %d\n", i, targets[i].x, targets[i].y);
        sprintf(pos_targets[i], "%d,%d", targets[i].x, targets[i].y);
        // write to server with pipe
    }

    // closing pipes
    for (int i = 0; i < 2; i++){
        if (close(pipeSefd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "TARGETS: error in closing pipe");
        }
    }
    return 0;
}