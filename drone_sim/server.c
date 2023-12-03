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
#define SEM_PATH_2 "/sem_drone2"

typedef enum {NO=0, OVER=1, UNDER=2} EDGE;

pid_t wd_pid = -1;
pid_t window_pid;

//volatile sig_atomic_t received_signal = 0;

typedef struct {
    int x;
    int y;
    EDGE isOnEdgex;
    EDGE isOnEdgey;
} Drone;    // Drone object

/*
typedef struct {
    int n_rows; // height
    int n_cols; // width
} Window;   // Window object
*/
/* this uses signal
void sig_handler(int signo)
{
    if (signo == SIGUSR1){
        printf("received signal from watch dog\n");
        
    }   
}*/

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        //received_signal =1;
        fprintf(debug, "%s\n", "SERVER: signal SIGUSR1 received from WATCH DOG");
        //printf("SERVER: Signal SIGUSR1 received from watch dog\n");
        //printf("SERVER: sending signal to wd\n");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "SERVER: terminating by WATCH DOG");
        printf("SERVER: terminating by WATCH DOG");
        fclose(debug);/*
        if (kill(window_pid, SIGTERM) == 0) {
        printf("SERVER: SIGTERM signal sent successfully to process %d.\n", window_pid);
    } else {
        perror("Error sending SIGTERM signal");
        exit(EXIT_FAILURE);
    }*/
        kill(window_pid, SIGTERM);
        exit(EXIT_FAILURE);
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
    printf("SERVER : process started\n");
    FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
    FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file

    
    Drone * drone;
    char *window_path[] = {"konsole", "-e", "./window", NULL};

// OPENING SEMAPHORES
    sem_t *sem_drone;   // semaphore for writing and reading drone
    //sem_t *sem_window;  // semaphore for writing and reading window
    writeToLog(debug, "SERVER: process started");
    sem_drone = sem_open(SEM_PATH_1, O_CREAT | O_RDWR, 0666, 1);    // Initial value: 1
    //sem_window = sem_open(SEM_PATH_2, O_CREAT | O_RDWR, 0666, 1);   // Initial value: 1
    sem_init(sem_drone, 1, 1);

    // Join the elements of the array into a single command string
    char command[100];
    snprintf(command, sizeof(command), "%s %s %s", window_path[0], window_path[1], window_path[2]);

    // OPENING WINDOW
    int window_pid = fork();
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


    // window mapping
    /*edges = (Window * )mmap(0, sizeof(Window), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // maps the shared memory object into the server's adress space
    if (edges == MAP_FAILED) {    // if there are errors in mapping
        perror("map failed\n");
        writeToLog(errors, "SERVER: window map failed");
        exit(EXIT_FAILURE);
    }*/

    // world parameters initialization ( I have to make them imported from a file)
    float k = 1.0; // N*s*m

    // initial position
    sem_wait(sem_drone);
    writeToLog(debug, "SERVER: initialized starting values");
    drone->x =2;
    drone->y =2;
    drone->isOnEdgex= NO;
    drone->isOnEdgey= NO;
    sem_post(sem_drone);

/*
    edges->n_rows = 100;    // height
    edges->n_cols = 200;    // base

    int edgx = edges -> n_rows;
    int edgy = edges -> n_cols;
    */

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
   int edgx = 100;
   int edgy = 40;

    while(1){
        /*sem_wait(sem_drone);
         // REGARD 2ND ASSIGNMENT
        if(drone->x >= edgx){
            
            drone->isOnEdgex = OVER;
        }
        else if(drone->x <= 0){
            drone->isOnEdgex = UNDER;
        }
        else{
            drone->isOnEdgex = NO;
        }
        if(drone->y >= edgy){
            drone->isOnEdgey = OVER;
        }
        else if(drone->y <= 0){
            drone->isOnEdgey = UNDER;
        }
        else{
            drone->isOnEdgey = NO;
        }
        sem_post(sem_drone);*/
        sleep(2);
    }


        

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
    //sem_close(sem_window);
    int failed = munmap(drone, SIZE);
    printf("FAILED FLAG: %d\n", failed);
    fprintf(debug, "%d\n", failed);
    fflush(debug);

    fclose(debug);
    fclose(errors);
    return 0;
}