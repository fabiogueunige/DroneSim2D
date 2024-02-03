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

typedef struct{
    bool exit_flag;
} Flag;

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        //received_signal =1;
        fprintf(debug, "%s\n", "INPUT: signal SIGUSR1 received from WATCH DOG");
        //printf("SERVER: Signal SIGUSR1 received from watch dog\n");
        //printf("SERVER: sending signal to wd\n");
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
    writeToLog(debug, "INPUT: process started");
    printf("I'm in input\n");
    char ch;
    int writefd;
    sscanf(argv[1], "%d", &writefd);
    Flag * flags;
    
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
    /*
    // SHARED MEMORY OPENING AND MAPPING
    const char * shm_name = "/flagsmem";
    const int SIZE = 4096;
    int shm_fd;
    shm_fd = shm_open(shm_name, O_CREAT| O_RDWR, 0666); // open shared memory segment for read and write
    if (shm_fd == 1) {
        perror("shared memory segment failed\n");
        writeToLog(errors, "DRONE:shared memory segment failed");
        exit(EXIT_FAILURE);
    }
    

    flags = (Flag *)mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0); // protocol write
    if (flags == MAP_FAILED) {
        perror("Map failed");
        writeToLog(errors, "Map Failed");
        return 1;
    }
    flags->exit_flag = false;*/
    // SHARED MEMORY

    
    // KEY READING
    while(1){
        
        ch = getKeyPress();
        
        // Interpret the command and perform corresponding action
        switch (ch) {
            case 'w':
                printf("Drone moving forward-left.\n");
                
                break;
            case 's':
                printf("Drone moving left.\n");
               
                break;
            case 'd':
                printf("Drone is breaking.\n");
                
                break;
            case 'f':
                printf("Drone moving right.\n");
                break;
            case 'e':
                printf("Drone moving forward.\n");
                break;
            case 'r':
                printf("Drone moving forward-right.\n");
                break;
            case 'x':
                printf("Drone moving back-left.\n");
                break;
            case 'c':
                printf("Drone moving back.\n");
                break;
            case 'v':
                printf("Drone moving back-right.\n");
                break;
            case 'q':
                printf("INPUT: Exiting program...\n");
                kill(wd_pid, SIGUSR2);
                //flags->exit_flag = true;
                exit(EXIT_FAILURE);  // Exit the program
            case 'u':
                printf("Resetting drone...\n");
                break;
            case 'b':
                printf("Goes on by inertia\n");
                break;
            default:
                //printf("INPUT: Unknown command. Please enter a valid command.\n");
                break;
        }

        write(writefd, &ch, sizeof(char));
        fsync(writefd);
    }

    return 0;
}