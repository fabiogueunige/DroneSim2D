#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include<stdbool.h>
#include <time.h>
#include <sys/file.h>
#define SEM_PATH_1 "/sem_drone1"

pid_t wd_pid = -1;
pid_t window_pid;
bool sigint_rec = false;

typedef struct {
    int x;
    int y;
    float vx, vy;
    int fx, fy;
} Drone;    // Drone object

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "SERVER: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "SERVER: terminating by WATCH DOG with return value 0...");
        printf("SERVER: terminating by WATCH DOG with return value 0...");
        fclose(debug);
        if (kill(window_pid, SIGTERM) == 0) {
            printf("SERVER: SIGTERM signal sent successfully to window");
        } 
        else {
            perror("Error sending SIGTERM signal");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_FAILURE);
    }
    if (signo == SIGINT){
        //pressed q or CTRL+C
        printf("SERVER: Terminating with return value 0...");
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "SERVER: terminating with return value 1...");
        fclose(debug);
        kill(window_pid, SIGTERM);
        sigint_rec = true;
    }
}

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

int main(int argc, char* argv[]){
    FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
    FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    writeToLog(debug, "SERVER: process started");
    printf("SERVER : process started\n");
    Drone * drone;
    char *window_path[] = {"konsole", "-e", "./window", NULL};  // path of window process
// OPENING SEMAPHORES
    sem_t *sem_drone;   // semaphore for writing and reading drone
    sem_drone = sem_open(SEM_PATH_1, O_CREAT | O_RDWR, 0666, 1);    // Initial value: 1
    // OPENING WINDOW
    // Join the elements of the array into a single command string
    char command[100];
    snprintf(command, sizeof(command), "%s %s %s", window_path[0], window_path[1], window_path[2]);
    pid_t window_pid = fork();
    if (window_pid ==-1){
        //error in fork
        perror("error in fork");
        writeToLog(errors, "SERVER: error in fork()");
    }
    if (window_pid == 0){
        //child process
        int system_return = system(command);
        writeToLog(debug, "SERVER: opened window");
        if (system_return != 0) {
            perror("system");
            writeToLog(errors, "SERVER: error in system");
            exit(EXIT_FAILURE);
        }
    }

// SHARED MEMORY INITIALIZATION AND MAPPING
    const char * shm_name = "/dronemem"; //name of the shm
    const int SIZE = 4096; //size of the shared memory
    
    int i, shm_fd;
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);    //generates shared memory for reading and writing
    if (shm_fd == -1) { //if there are errors generating shared memory
        perror("error in shm_open\n");
        writeToLog(errors, "SERVER: error in shm_open");
        exit(EXIT_FAILURE);
    }
    else{
        printf("SERVER: generated shared memory\n");
        writeToLog(debug, "SERVER: generated shared memory");
    }

    if(ftruncate(shm_fd, SIZE) == -1){
        perror("ftruncate");
        writeToLog(errors, "SERVER: ftruncate");
        exit(EXIT_FAILURE);
    } //set the size of shm_fd

    // drone mapping
    drone = (Drone * )mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // maps the shared memory object into the server's adress space
    if (drone == MAP_FAILED) {    // if there are errors in mapping
        perror("map failed\n");
        writeToLog(errors, "SERVER: drone map failed");
        exit(EXIT_FAILURE);
    }

    // initial position
    sem_wait(sem_drone);
    writeToLog(debug, "SERVER: initialized starting values");
    printf("SERVER: initialized starting values");
    drone->x =2;
    drone->y =20;
    sem_post(sem_drone);

   // SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
        perror("sigaction");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }
    int edgx = 100;
    int edgy = 40;

    while(!sigint_rec);

    // waits window to terminate
    if(wait(NULL)==-1){
        perror("wait");
        writeToLog(errors, "SERVER: error in wait");
    };

// CLOSE AND UNLINK SHARED MEMORY
    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    
    if (shm_unlink(shm_name) == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    sem_close(sem_drone);
    int failed = munmap(drone, SIZE);
    printf("FAILED FLAG: %d\n", failed);
    fprintf(debug, "%d\n", failed);
    fflush(debug);

    fclose(debug);
    fclose(errors);
    return 0;
}