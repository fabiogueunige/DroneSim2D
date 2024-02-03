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
#include <ncurses.h>
#include "library/window.h"

/*
Acquired all the char and sends to the drone through pipes
*/

#define DEBUG 1

//v#ifndef DEBUG
//managing signal
void sig_handler(int , siginfo_t *, void *);

void writeToLog(FILE *, const char *);

// #endif

int main (int argc, char* argv[])
{
    //pipe creation
    int inpfd[2];
    char pidstr[2][70];
    pid_t cid;
    int ch;
    char realchar = '\0';
    int counter[NUMMOTIONS];
    FILE *inputfile;
    FILE *debug;
    FILE * errors;
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
    printf("I'm in input\n");

    // Samu to implement your tie ifyou want


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
        writeToLog(errors, "Input: pipe input ncurses");
        return 2;
    }
    for (int i = 0; i < 2; i++) {
        sprintf(pidstr[i], "%d", inpfd[i]);
    }

    char * argcou[] = {"konsole", "-e","./inputc",pidstr[0], pidstr[1], NULL};
    cid = spawn("konsole",argcou,errors);
    // closing write
    close(inpfd[1]);
    // Doing input process
    int pinpdrone[2];

    // opening pipe trough drone and input
    for (int i = 0; i < 2; i++) {
        pinpdrone[i] = atoi(argv[i]); // converts from the string to the integer
    }
    close(pinpdrone[0]);

    // while to get char
    while(ch != 'Q') {
        if ((read(inpfd[0], &ch, sizeof(char))) < 0) {
            perror("read input ncurses");
            writeToLog(errors, "Input: read pipe input ncurses");
            return 3;
        }
        if ((write(pinpdrone[1], &ch, sizeof(char))) < 0) {
            perror("write input ncurses");
            writeToLog(errors, "Input: write to drone pipe");
            return 3;
        }
    }
    // closing read
    close(inpfd[0]);
    close(pinpdrone[1]);

    wait(NULL); // wait for the child to terminate
// #endif
    return 0;
}

int spawn(const char * program, char ** arg_list, FILE * err) {
    pid_t child_pid;
    char *msgerr[70] = "INPUT: error in fork of ";
    
    if ((child_pid = fork()) == -1) {
        perror("fork failed");
        strcat(msgerr, program);
        writeToLog(err, msgerr);
        exit(EXIT_FAILURE);
    }
    if (child_pid != 0)
        return child_pid;
    else {
        execvp (program, arg_list);
        *msgerr [70] = "INPUT: error in execvp of";
        perror("exec failed");
        strcat(msgerr, program);
        writeToLog(err, msgerr);
        exit(EXIT_FAILURE);
    }
}

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug;
        debug = fopen("logfiles/debug.log", "a");
        if (debug < 0) {
            perror("INPUT, fopen: debug file sgnl 2");
        return 1;
        }
        // SIGUSR1 received
        wd_pid = info->si_pid;
        //received_signal =1;
        writeToLog(debug, "INPUT: signal SIGUSR1 received from WATCH DOG");
        //printf("SERVER: Signal SIGUSR1 received from watch dog\n");
        //printf("SERVER: sending signal to wd\n");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug;
        debug = fopen("logfiles/debug.log", "a");
        if (debug < 0) {
            perror("INPUT, fopen: debug file signl2");
        return 1;
        }
        writeToLog(debug, "INPUT: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
    
    
}

void writeToLog(FILE *logFile, const char *message) {
    // function to write on the files
    time_t crtime;
    time(&crtime);
    fprintf(logFile,"%s => ", ctime(&crtime));
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
    /*void writeToLog(FILE * logFile, const char *message) {
    //lock(logFile);
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
    //unlock(logFile);
    / lock() per impedire che altri usino il log file -> unlock()
}*/
}