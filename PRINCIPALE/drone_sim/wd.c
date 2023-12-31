#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/file.h>
typedef enum {FALSE=0, TRUE=1} BOOL;

BOOL server_check, drone_check, input_check, obstacle_check, target_check;
pid_t drone_pid, server_pid, input_pid, obstacle_pid, target_pid;

void writeToLog(FILE *logFile, const char *message) {
    time_t crtime;
    time(&crtime);
    int lockResult = flock(fileno(logFile), LOCK_EX);
    if (lockResult == -1) {
        perror("Failed to lock the log file");
        // Handle the error as needed (e.g., exit or return)
        return;
    }
    fprintf(logFile,"%s => ", ctime(&crtime));
    fprintf(logFile, "%s\n", message);
    fflush(logFile);

    int unlockResult = flock(fileno(logFile), LOCK_UN);
    if (unlockResult == -1) {
        perror("Failed to unlock the log file");
        // Handle the error as needed (e.g., exit or return)
    }
}

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        if (debug == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        // SIGUSR1 received
        pid_t pid;
        pid = info->si_pid;
        if (pid==drone_pid){
            
            writeToLog(debug, "WATCH DOG: DRONE received signal");
            drone_check = TRUE;
        }
        if(pid == input_pid){
            
            writeToLog(debug, "WATCH DOG: INPUT received signal");
            input_check= TRUE;
        }
        if(pid == server_pid){
            writeToLog(debug, "WATCH DOG: SERVER received signal");
            server_check = TRUE;
        }
        if(pid == obstacle_pid){
            writeToLog(debug, "WATCH DOG: OBSTACLE received signal");
            obstacle_check = TRUE;
        }
        fclose(debug);
    }
    if (signo == SIGUSR2) {
        // input has already terminated, so 
        printf("WATCH DOG: quitting program and returning 0, terminating obstacles and server...\n");
        kill(server_pid, SIGINT);
        kill(obstacle_pid, SIGINT);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]){

    FILE * debug = fopen("logfiles/debug.log", "a");
    if (debug == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    FILE * errors = fopen("logfiles/errors.log", "a");
    if (errors == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "WATCH DOG: process started");
    printf("WATCH DOG: process started");

// SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    //sa.sa_flags = 0;
    sa.sa_sigaction = sig_handler;
    

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "DRONE: error in sigaction()");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "DRONE: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    char * string1 = argv[1];   // s
    char * string2 = argv[2];   // d
    char * string3 = argv[3];   // i
    char * string4 = argv[4];   // o
    char * string5 = argv[5];   // t

    server_pid = atoi(string1);
    drone_pid = atoi(string2);
    input_pid = atoi(string3);
    obstacle_pid = atoi(string4);
    target_pid = atoi(string5);

    
    while(1){
        writeToLog(debug, "WATCH DOG: sending signals to processes");
        server_check = FALSE;
        input_check = FALSE;
        drone_check = FALSE;
        obstacle_check = FALSE;
        target_check = FALSE;

        if (kill(server_pid, SIGUSR1) == -1) {  // send SIGUSR1 to server
            perror("kill server");
            writeToLog(errors, "WATCH DOG: error in kill server");
        }
        
        sleep(1);
        if (kill(drone_pid, SIGUSR1) == -1) {   // send SIGUSR1 to drone
            perror("kill drone");
            writeToLog(errors, "WATCH DOG: error in kill drone");
        }
        
        sleep(1);
        if (kill(input_pid, SIGUSR1) == -1) {   // send SIGUSR1 to input
            perror("kill input");
            writeToLog(errors, "WATCH DOG: error in kill input");
        }
        sleep(1);

        if (kill(obstacle_pid, SIGUSR1) == -1) {   // send SIGUSR1 to obstacle
            perror("kill obstacle");
            writeToLog(errors, "WATCH DOG: error in kill obstacle");
        }

        sleep(1);
/*
        if (kill(target_pid, SIGUSR1) == -1) {   // send SIGUSR1 to obstacle
            perror("kill target");
            writeToLog(errors, "WATCH DOG: error in kill target");
        }
*/
        sleep(10); // timeout

        /* Here we decided to identificate wich process stopped working using 3 different if in order to make, 
        in the 2nd assignment, the watch dog to close and reopen only the process that stopped working, then if after a certain 
        number of times it doesn't respond, we will terminate it. In firt assignment we only make it terminate the whole program.*/

        if(server_check==FALSE) //checks if server responded
        {
            writeToLog(debug, "WATCH DOG: SERVER is not responding, terminating the program...");
            if (kill(server_pid, SIGUSR2) == -1) {  // send SIGUSR1 to server
                perror("kill server");
                writeToLog(errors, "WATCH DOG: error in kill server");
            }
            if (kill(drone_pid, SIGUSR2) == -1) {   // send SIGUSR2 to drone
            perror("kill drone");
            writeToLog(errors, "WATCH DOG: error in kill drone");
            }
            if (kill(input_pid, SIGUSR2) == -1) {   // send SIGUSR2 to input
                perror("kill input");
                writeToLog(errors, "WATCH DOG: error in kill input");
            }
            if(kill(obstacle_pid, SIGUSR2) == -1){
                perror("kill obstacle");
                writeToLog(errors, "WATCH DOG: error in kill obstacle");
            }
            exit(EXIT_FAILURE);
        }
        else
            printf("WATCH DOG: SERVER received signal\n");
        

        if(drone_check==FALSE) //checks if server responded
        {
            writeToLog(debug, "WATCH DOG: DRONE is not responding, terminating the program...");
            if (kill(server_pid, SIGUSR2) == -1) {  // send SIGUSR1 to server
                perror("kill server");
                writeToLog(errors, "WATCH DOG: error in kill server");
            }
            if (kill(drone_pid, SIGUSR2) == -1) {   // send SIGUSR2 to drone
            perror("kill drone");
            writeToLog(errors, "WATCH DOG: error in kill drone");
            }
            if (kill(input_pid, SIGUSR2) == -1) {   // send SIGUSR2 to input
                perror("kill input");
                writeToLog(errors, "WATCH DOG: error in kill input");
            }
            if(kill(obstacle_pid, SIGUSR2) == -1){
                perror("kill obstacle");
                writeToLog(errors, "WATCH DOG: error in kill obstacle");
            }
            exit(EXIT_FAILURE);
        }
        else
            printf("WATCH DOG: DRONE received signal\n");
        
        if(input_check==FALSE) //checks if server responded
        {
            writeToLog(debug, "WATCH DOG: INPUT is not responding, terminating the program...");
            if (kill(server_pid, SIGUSR2) == -1) {  // send SIGUSR1 to server
                perror("kill server");
                writeToLog(errors, "WATCH DOG: error in kill server");
            }
            if (kill(drone_pid, SIGUSR2) == -1) {   // send SIGUSR2 to drone
            perror("kill drone");
            writeToLog(errors, "WATCH DOG: error in kill drone");
            }
            if (kill(input_pid, SIGUSR2) == -1) {   // send SIGUSR2 to input
                perror("kill input");
                writeToLog(errors, "WATCH DOG: error in kill input");
            }
            if(kill(obstacle_pid, SIGUSR2) == -1){
                perror("kill obstacle");
                writeToLog(errors, "WATCH DOG: error in kill obstacle");
            }
            exit(EXIT_FAILURE);
        }
        else
            printf("WATCH DOG: INPUT received signal\n");
        // sends a signal to all processes to check they are alive

        if(obstacle_check==FALSE) //checks if server responded
        {
            writeToLog(debug, "WATCH DOG: OBSTACLE is not responding, terminating the program...");
            if (kill(server_pid, SIGUSR2) == -1) {  // send SIGUSR1 to server
                perror("kill server");
                writeToLog(errors, "WATCH DOG: error in kill server");
            }
            if (kill(drone_pid, SIGUSR2) == -1) {   // send SIGUSR2 to drone
            perror("kill drone");
            writeToLog(errors, "WATCH DOG: error in kill drone");
            }
            if (kill(input_pid, SIGUSR2) == -1) {   // send SIGUSR2 to input
                perror("kill input");
                writeToLog(errors, "WATCH DOG: error in kill input");
            }
            if(kill(obstacle_pid, SIGUSR2) == -1){
                perror("kill obstacle");
                writeToLog(errors, "WATCH DOG: error in kill obstacle");
            }
            exit(EXIT_FAILURE);
        }
        else
            printf("WATCH DOG: DRONE received signal\n");
    }
    fclose(debug);
    fclose(errors);
    return 0;
}