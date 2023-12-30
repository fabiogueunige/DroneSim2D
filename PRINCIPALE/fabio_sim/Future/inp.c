#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

/*
Acquired all the char and sends to the drone through pipes
*/

#define DEBUG 1
#define NUMMOTIONS 9

pid_t wd_pid = -1;
bool exit_flag = false;

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "INPUT: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "INPUT: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }   
}

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}

int main (int argc, char* argv[])
{
    //pipe creation
    
    pid_t cid;
    
    FILE *debug;
    FILE * errors;

    int ch;
    char realchar = '\0';
    int counter[NUMMOTIONS];
    int inpfd[2];
    int pipeDrwr;
    char piperd[10];
    char pipewr[10];

    debug = fopen("logfiles/debug.log", "a");
    if (debug < 0) {
        perror("INPUT, fopen: debug file main");
        return 1;
    }    // debug log file
    errors = fopen("logfiles/errors.log", "a");  // errors log file
    if (errors < 0) {
        perror("INPUT, fopen: eroor file main");
        return 1;
    }
    writeToLog(debug, "INPUT: process started");
    printf("INPUT: process started\n");

  

    //configure the handler for sigusr1
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
    writeToLog(debug, "INPUT: Signal succesfully created");

    //create input file

// #ifndef DEBUG
    // opening pipe through input and userinterface
    if (pipe(inpfd) < 0) {
        perror("pipe input ncurses");
        writeToLog(errors, "INPUT: pipe input ncurses");
        return 2;
    }

    sprintf(pipewr, "%d", inpfd[1]);

    char * argcou[] = {"konsole", "-e","./inpCourses", pipewr, NULL};
    cid = spawn("konsole",argcou);
    // closing write
    close(inpfd[1]);

    // pipe to write on the drone
    sscanf(argv[1], "%d", &pipeDrwr);

    // opening pipe trough drone and input

    // while to get char  !! Aggiungi SELECT !!
    while(ch != 'Q') {
        if ((read(inpfd[0], &ch, sizeof(char))) < 0) {
            perror("read input ncurses");
            writeToLog(errors, "Input: read pipe input ncurses");
            return 3;
        }
        if ((write(pipeDrwr, &ch, sizeof(char))) < 0) {
            perror("write input ncurses");
            writeToLog(errors, "Input: write to drone pipe");
            return 3;
        }
    }
    // closing read
    close(inpfd[0]);
    close(pipeDrwr);

    wait(NULL); // wait for the child to terminate
// #endif
    return 0;
}
