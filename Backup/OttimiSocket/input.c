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
#include <sys/shm.h>
#include <sys/mman.h>
#include <stdbool.h>

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

char getKeyPress() {
    struct termios oldt, newt;
    char ch;

    // Get the current terminal settings
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        // Handle the error or exit as needed
        exit(EXIT_FAILURE);  // Return a default value indicating an error
    }

    // Disable buffering and echoing for stdin
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        perror("tcsetattr");
        // Handle the error or exit as needed
        exit(EXIT_FAILURE);  // Return a default value indicating an error
    }

    // Read a single character
    ch = getchar();

    // Restore the old terminal settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) {
        perror("tcsetattr");
        // Handle the error or exit as needed
        exit(EXIT_FAILURE);  // Return a default value indicating an error
    }
    return ch;
}

int main(int argc, char* argv[]){
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    char ch;
    int writefd;

    if (debug == NULL || errors == NULL) {
        printf("INPUT: error opening log files\n");
        exit(EXIT_FAILURE);
    }

    writeToLog(debug, "INPUT: process started");
    fclose(debug);
    printf("INPUT: process started\n");
    
    sscanf(argv[1], "%d", &writefd);
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
    // KEY READING
    while(!exit_flag){
        
        ch = getKeyPress();
        if(ch=='q'){
            printf("INPUT: Exiting program...\n");
            sleep(2); // gives the time to master to open watchdog (if pressed when just opened)
            kill(wd_pid, SIGUSR2);
            //flags->exit_flag = true;
            exit_flag = true;
        }
        if ((write(writefd, &ch, sizeof(char))) == -1) {
            perror("write");
            writeToLog(errors, "INPUT: error in write to drone");
            exit(EXIT_FAILURE);
        }
        fsync(writefd);
    }


    fclose(errors);
    return 0;
}