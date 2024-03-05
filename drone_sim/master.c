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
    pid_t child_pid = fork();
    if (child_pid != 0)
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

// CREATING PIPE
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
    
    ///char * argdes_path[] = {"konsole", "-e","./description", NULL};
    
    pid_t server;
    pid_t input;
    pid_t drone;
    pid_t wd;
    
// INTRO
    char keyy;
    bool right_key= false;
    printf("\t\t  ____________________________________\n");
    printf("\t\t |                                    |\n");
    printf("\t\t |   Advanced and Robot Programming   |\n");
    printf("\t\t |           DRONE: part 1            |\n");
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
    printf("Press s to start or q to quit, then press ENTER...\n");
    scanf("%c", &keyy);
    do{
        if (keyy == 's'){
            right_key = true;
            printf("\n\n\n\n\t\t\t\tLET'S GO!\n\n\n\n");
        }
        else if(keyy == 'q'){
            right_key = true;
            exit(EXIT_SUCCESS);
        }
        else{
            right_key = false;
            printf("\nPress s to start or q to quit, then press ENTER...\n");
            scanf("%c", &keyy);
        }
    }while(!right_key);

// EXECUTING PROCESSES
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
    
    for(int i = 0; i<4; i++){   //waits for all processes to end
        wait(NULL); 
    }
    close(pipe_fd1[0]);
    close(pipe_fd1[1]);
    fclose(debug);
    fclose(errors);
    return 0;
}