#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <sys/file.h>

#define NUMPROCESS 6
#define SERVER 0
#define DRONE 1
#define INPUT 2
#define OBSTACLES 3
#define TARGETS 4
#define WATCHDOG 5
#define MASTER 5



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

int spawn(const char * program, char ** arg_list) {
    FILE * errors = fopen("logfiles/errors.log", "a");
    pid_t child_pid;
    if ((child_pid = fork()) == -1) {
        perror("fork");
        printf("error in fork");
        writeToLog(errors, "MASTER: fork failed");
        return -1;
    }
    else if (child_pid == 0)
        return child_pid;
    else {
        execvp (program, arg_list);
        perror("exec failed");
        writeToLog(errors, "MASTER: execvp failed");
        exit(EXIT_FAILURE);
    }
    fclose(errors);
}

int main(int argc, char* argv[]){

    FILE * debug = fopen("logfiles/debug.log", "w");
    FILE * errors = fopen("logfiles/errors.log", "w");

    pid_t proIds[NUMPROCESS];

// Creating PIPE and Processes (new version)
    int pipe_fd[NUMPROCESS][2]; // Escluding WD
    for (int i = 0; i < NUMPROCESS; i++){
        if (pipe(pipe_fd[i]) == -1){
            perror("error in pipe");
            writeToLog(errors, "MASTER: error opening pipe;");
        }
    }

    char piperd[NUMPROCESS][70];    // string that contains the readable fd of pipe_fd
    char pipewr[NUMPROCESS][70];    // string that contains the writeable fd of pipe_fd
    for (int i = 0; i < NUMPROCESS; i++){
        sprintf(piperd[i], "%d", pipe_fd[i][0]);
        sprintf(pipewr[i], "%d", pipe_fd[i][1]);
    }

    // processes path
    char *server_path[] = {"./server",piperd[DRONE + 1], NULL};
    char *drone_path[] = {"./drone", piperd[TARGETS + 1], piperd[INPUT + 1], piperd[OBSTACLES + 1], pipewr[DRONE + 1], NULL};
    char *input_path[] = {"./input",pipewr[INPUT + 1] ,NULL};
    char *obstacles_path[] = {"./obstacles", pipewr[OBSTACLES + 1], NULL};
    char *targets_path[] = {"./targets", pipewr[TARGETS + 1], NULL};
    
    ///char * argdes_path[] = {"konsole", "-e","./description", NULL};
    proIds[SERVER] = spawn("./server", server_path);
    usleep(500000);
    proIds[DRONE] = spawn("./drone", drone_path);
    usleep(500000);
    proIds[INPUT] = spawn("./input", input_path);
    usleep(500000);
    proIds[OBSTACLES] = spawn("./obstacles", obstacles_path);
    usleep(500000);
    proIds[TARGETS] = spawn("./targets", targets_path);

    for (int i = 0; i < NUMPROCESS; i++){ // Chiude pipe nel master
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

    // Costruisce il watchdog  
    char pidStr[NUMPROCESS - 1][50];

    for (size_t i = 0; i < (NUMPROCESS - 1); ++i)   // this for cycle passes  to pidString all elements of pidList
        sprintf(pidStr[i], "%d", proIds[i]);

    char *wd_path[] = {"./wd", pidStr[SERVER], pidStr[DRONE], pidStr[INPUT], pidStr[OBSTACLES], pidStr[TARGETS], NULL}; // passes to the watch dog all the pid of the processes
    sleep(1);
    proIds[WATCHDOG] = spawn("./wd", wd_path);
    // Spostare poi sotto la description
    

// CREATING PIPE (old version)
/*
    int pipe_fd1[2];    // pipe from input to drone
    if (pipe(pipe_fd1) == -1){
        perror("error in pipe");
        writeToLog(errors, "MASTER: error opening pipe;");
    }

    char piperd[10];    // string that contains the readable fd of pipe_fd
    char pipewr[10];    // string that contains the writeable fd of pipe_fd
    sprintf(piperd, "%d", pipe_fd1[0]);
    sprintf(pipewr, "%d", pipe_fd1[1]);

    // processes path
    char * server_path[] = {"./server", NULL};
    char * drone_path[] = {"./drone", piperd, NULL};
    char * input_path[] = {"./input",pipewr ,NULL};
    
    ///
    
    pid_t server;
    pid_t input;
    pid_t drone;
    pid_t wd;
*/ 

// INTRO
    printf("\t\t  ____________________________________\n");
    printf("\t\t |                                    |\n");
    printf("\t\t |   Advanced and Robot Programming   |\n");
    printf("\t\t |           DRONE: part 2            |\n");
    printf("\t\t |____________________________________|\n");
    printf("\n");
    printf("\t\t    by Samuele Viola and Fabio Guelfi\n\n");
    sleep(2);
    printf("This game is a simple game of a drone control, where the user, by pressing \nsome buttons can drive the drone in order to avoid the obstacles and reach the targets.\n\n");
    printf("\t\t\t  ______________________\n");
    printf("\t\t\t |  KEYS INSTRUCTIONS:  |\n");
    printf("\t\t\t | E: MOVE UP           |\n");
    printf("\t\t\t | C: MOVE DOWN         |\n");
    printf("\t\t\t | S: MOVE LEFT         |\n");
    printf("\t\t\t | F: MOVE RIGHT        |\n");
    printf("\t\t\t | R: MOVE UP-RIGHT     |\n");
    printf("\t\t\t | W: MOVE UP-LEFT      |\n");
    printf("\t\t\t | V: MOVE DOWN-RIGHT   |\n");
    printf("\t\t\t | X: MOVE DOWN-LEFT    |\n");
    printf("\t\t\t |                      |\n");
    printf("\t\t\t | D: REMOVE ALL FORCES |\n"); //inertia
    printf("\t\t\t | B: BRAKE             |\n");
    printf("\t\t\t | U: RESET THE DRONE   |\n");
    printf("\t\t\t | Q: QUIT THE GAME     |\n");
    printf("\t\t\t |______________________|\n\n\n");
    printf("Press any key to start\n");
    
    // waiting for the user to press a key to end the introduction
        /* Mettere description
// Starting the introduction
    pid_t pid_des;
    if ((pid_des = fork()) == -1) {
        perror("fork description");
        return 2;
    }
    if (pid_des == 0) {
        // child description process
        char * argdes[] = {"konsole", "-e","./description", NULL};
        if (execvp("konsole", argdes) == -1){
            perror("exec failed");
            return -1;
        }
    }
*/
    // wait(NULL); // Per description

// EXECUTING PROCESSES (old version)
/*
    server = spawn("./server", server_path);
    usleep(500000);
    drone = spawn("./drone", drone_path);
     usleep(500000);
    input =  spawn("./input", input_path);
    //usleep(500000);
    pid_t pidList[] = {server, drone, input};
    char pidString[3][50];

    for (size_t i = 0; i < sizeof(pidList) / sizeof(pidList[0]); ++i)   // this for cycle passes  to pidString all elements of pidList
        sprintf(pidString[i], "%d", pidList[i]);

    char *wd_path[] = {"./wd", pidString[0], pidString[1], pidString[2], NULL}; // passes to the watch dog all the pid of the processes
    sleep(1);
    wd = spawn("./wd", wd_path);

*/
    
    for(int i = 0; i < NUMPROCESS; i++){   //waits for all processes to end
        wait(NULL); 
    }

    fclose(debug);
    fclose(errors);
    return 0;
}