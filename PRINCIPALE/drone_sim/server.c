#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include<stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/select.h>
#include <errno.h>


pid_t wd_pid = -1;
pid_t window_pid;
bool sigint_rec = false;

typedef struct {
    int x;
    int y;
    float vx, vy;
    int fx, fy;
} Drone;    // Drone object

struct obstacle {
    int x;
    int y;
};

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
        fprintf(debug, "%s\n", "SERVER: terminating with return value 0...");
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
    Drone dr;
    drone = &dr;
    int nobstacles;
    int rows = 50;
    int cols = 100;
    int nobstacles_edge = 2 * (rows + cols);
    struct obstacle *edges[nobstacles_edge];

    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }

    char *window_path[] = {"konsole", "-e", "./window", NULL};  // path of window process
/*
// OPENING SEMAPHORES
    sem_t *sem_drone;   // semaphore for writing and reading drone
    sem_drone = sem_open(SEM_PATH_1, O_CREAT | O_RDWR, 0666, 1);    // Initial value: 1
*/
    // OPENING WINDOW
    // Join the elements of the array into a single command string
    char command[100];
    // pipe to window
    int pipeWdfd[2];
    if(pipe(pipeWdfd) == -1){
        perror("error in pipe");
        writeToLog(errors, "SERVER: error in pipe opening");
        exit(EXIT_FAILURE);
    }
    snprintf(command, sizeof(command), "%s %s %s %d", window_path[0], window_path[1], window_path[2], pipeWdfd[0]);
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

    // PIPES
    int pipeDrfd[2];    // pipe for drone: 0 for reading, 1 for writing
    int pipeObfd[2];    // pipe for obstacles: 0 for reading, 1 for writing
    int pipeTafd[2];    // pipe for targets: 0 for reading, 1 for writing
    fd_set read_fds;
    fd_set write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    

    sscanf(argv[1], "%d", &pipeDrfd[0]);
    sscanf(argv[2], "%d", &pipeDrfd[1]);
    sscanf(argv[3], "%d", &pipeObfd[1]);
    sscanf(argv[4], "%d", &pipeObfd[0]);
    sscanf(argv[5], "%d", &pipeTafd[1]);
    sscanf(argv[6], "%d", &pipeTafd[0]);
    writeToLog(debug, "SERVER: pipes opened");

    // SENDING ROWS AND COLUMNS TO WINDOW
    if((write(pipeWdfd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeWdfd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeDrfd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeDrfd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeObfd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeObfd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeTafd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeTafd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    char *obs = "obs";
    char *tar = "tar";
    char *coo = "coo";
/*
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

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }


    while(!sigint_rec){
        // select wich pipe to read from between drone and obstacles
        FD_SET(pipeDrfd[0], &read_fds);
        FD_SET(pipeObfd[0], &read_fds); // include pipeObfd[0] in read_fds
        FD_SET(pipeTafd[0], &read_fds);
        FD_SET(pipeWdfd[1], &write_fds);

        int max_fd = -1;
        if (pipeTafd[0] > max_fd) {
            max_fd = pipeTafd[0];
        }
        if(pipeObfd[0] > max_fd) {
            max_fd = pipeObfd[0];
        }
        if(pipeDrfd[0] > max_fd) {
            max_fd = pipeDrfd[0];
        }
        int sel;
        // ciclo do while per evitare errori dovuuti a segnali
        
        do{
            sel = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        }while(sel == -1 && errno == EINTR);
        
        if(sel ==-1){
            perror("error in select");
            writeToLog(errors, "SERVER: error in select");
            exit(EXIT_FAILURE);
        }
        else if(sel>0){
            if(FD_ISSET(pipeTafd[0], &read_fds)){
                printf("SERVER: reading from targets\n");
                int ntargets;
                if ((read(pipeTafd[0], &ntargets, sizeof(int))) == -1) { // reads from drone ntargets
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe targets");
                    exit(EXIT_FAILURE);
                }
                printf("SERVER: number of targets: %d\n", ntargets);
                if((write(pipeDrfd[1], tar, strlen(tar))) == -1){
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe drone that it will sends targets");
                    exit(EXIT_FAILURE);
                }
                if ((write(pipeDrfd[1], &ntargets, sizeof(int))) == -1){  // writes to drone ntargets
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe number of targets");
                    exit(EXIT_FAILURE);
                }
                if((write(pipeWdfd[1], tar, strlen(tar))) == -1){
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe window that it will sends targets");
                    exit(EXIT_FAILURE);
                }

                if ((write(pipeWdfd[1], &ntargets, sizeof(int))) == -1){  // writes to window ntargets
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe number of targets");
                    exit(EXIT_FAILURE);
                }
                targets *target[ntargets];
                for(int i = 0; i<ntargets; i++){
                    target[i] = malloc(sizeof(targets));
                    read(pipeTafd[0], target[i], sizeof(targets));
                    printf("SERVER: target %d position: (%d, %d)\n", i, target[i]->x, target[i]->y);
                    write(pipeDrfd[1], target[i], sizeof(targets));
                    write(pipeWdfd[1], target[i], sizeof(targets));
                }
            }

            if(FD_ISSET(pipeObfd[0], &read_fds)){
                printf("SERVER: reading from obstacles\n");
                if ((read(pipeObfd[0], &nobstacles, sizeof(int))) == -1) { // reads from drone nobstacles
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe obstacles");
                    exit(EXIT_FAILURE);
                } 
                printf("SERVER: number of obstacles: %d\n", nobstacles);
                if((write(pipeDrfd[1], obs, strlen(obs))) == -1){
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe drone that it will sends obstacles");
                    exit(EXIT_FAILURE);
                }
                if ((write(pipeDrfd[1], &nobstacles, sizeof(int))) == -1){  // writes to drone nobstacles
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe number of obstacles");
                    exit(EXIT_FAILURE);
                }
                //strcpy(data_info, "obs");
                if ((write(pipeWdfd[1], obs, strlen(obs))) == -1){  // writes to window that it will sends obstacles
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe window that it will sends obstacles");
                    exit(EXIT_FAILURE);
                }
                if ((write(pipeWdfd[1], &nobstacles, sizeof(int))) == -1){  // writes to window nobstacles
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe number of obstacles");
                    exit(EXIT_FAILURE);
                }

                struct obstacle *obstacles[nobstacles];
                for(int i = 0; i<nobstacles; i++){
                    obstacles[i] = malloc(sizeof(struct obstacle));
                    read(pipeObfd[0], obstacles[i], sizeof(struct obstacle));
                    printf("SERVER: obstacle %d position: (%d, %d)\n", i, obstacles[i]->x, obstacles[i]->y);
                    write(pipeDrfd[1], obstacles[i], sizeof(struct obstacle));
                    write(pipeWdfd[1], obstacles[i], sizeof(struct obstacle));
                }
                //write(pipeDrfd[1], obstacles, sizeof(obstacles));
            }

            if(FD_ISSET(pipeDrfd[0], &read_fds)){
                if ((read(pipeDrfd[0], drone, sizeof(Drone))) == -1) { // reads from drone
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe drone");
                    exit(EXIT_FAILURE);
                }
                printf("SERVER: drone position: (%d, %d)\n", drone->x, drone->y);
            }
            printf("SERVER: writing to window\n");
            // NON HO TROVATO LETTURA SU WINDOW (AGGIUNTA)
            if((write(pipeWdfd[1], coo, strlen(coo))) == -1){
                perror("error in writing to pipe");
                writeToLog(errors, "SERVER: error in writing to pipe window that it will sends coordinates");
                exit(EXIT_FAILURE);
            }
            if ((write(pipeWdfd[1], drone, sizeof(Drone))) == -1){  // writes to window drone
                perror("error in writing to pipe");
                writeToLog(errors, "SERVER: error in writing to pipe window the drone");
                exit(EXIT_FAILURE);
            }
            

        }
        
    }

    // waits window to terminate
    if(wait(NULL)==-1){
        perror("wait");
        writeToLog(errors, "SERVER: error in wait");
    };
/*
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
*/
    // closing pipes
    for (int i = 0; i < 2; i++){
        if (close(pipeDrfd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe Drone");
        }
        if (close(pipeObfd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe obstacles");
        }
        if (close(pipeTafd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe Targets");
        }
    }
    close(pipeWdfd[0]);
    close(pipeWdfd[1]);
    fclose(debug);
    fclose(errors);
    return 0;
}