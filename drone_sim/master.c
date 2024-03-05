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
#define NUMPIPE 7 //diventa 7
#define SERVER 0
#define DRONE 1
#define INPUT 2
#define OBSTACLES 3
#define TARGETS 4
#define WATCHDOG 5

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

    pid_t pidDes;
    pid_t proIds[NUMPROCESS];
    char pidStr[NUMPROCESS - 1][50];
    char piperd[NUMPIPE][10];    // string that contains the readable fd of pipe_fd
    char pipewr[NUMPIPE][10];
    int pipe_fd[NUMPIPE][2]; 

    // CREATING PIPE
/*
    pipe 0: input -> drone  = indr
    pipe 1: drone -> server = drse
    pipe 2: server -> drone = sedr
    pipe 3: server -> obstacles = seob
    pipe 4: obstacles -> server = obse 
    pipe 5: server -> targets = seta
    pipe 6: targets -> server = tase
*/
    // generates all the pipes
    for (int i = 0; i < NUMPIPE; i++){
        if (pipe(pipe_fd[i]) == -1){
            perror("error in pipe");
            writeToLog(errors, "MASTER: error opening pipe;");
        }
    }
    // converts the fd of the pipes in strings
    for (int i = 0; i < NUMPIPE; i++){
        sprintf(piperd[i], "%d", pipe_fd[i][0]);
        sprintf(pipewr[i], "%d", pipe_fd[i][1]);
    }


    // processes path
    char * server_path[] = {"./server", piperd[1], pipewr[2],pipewr[3],piperd[4],pipewr[5],piperd[6], NULL};
    char * drone_path[] = {"./drone", piperd[0],pipewr[1],piperd[2], NULL};
    char * input_path[] = {"./input",pipewr[0], NULL};
    char *obstacles_path[] = {"./obstacles",piperd[3], pipewr[4], NULL};
    char *targets_path[] = {"./targets",piperd[5], pipewr[6], NULL};
    
    ///char * argdes_path[] = {"konsole", "-e","./description", NULL};

    // INTRO
    char keyy;
    bool right_key= false;
    char * argdes_path[] = {"konsole", "-e","./description", NULL};
    pidDes = spawn("konsole", argdes_path);
    usleep(500000);

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
    printf("Press any key on the window to start the game\n");

    if ((waitpid(pidDes, NULL, 0)) == -1){
        perror("waitpid");
        writeToLog(errors, "MASTER: error in waitpid for description");
    }

    // EXECUTING PROCESSES
    proIds[SERVER] = spawn("./server", server_path);
    usleep(500000);
    proIds[DRONE] = spawn("./drone", drone_path);
    usleep(500000);
    proIds[INPUT] = spawn("./input", input_path);
    usleep(500000);
    proIds[OBSTACLES] = spawn("./obstacles", obstacles_path);
    usleep(500000);
    proIds[TARGETS] = spawn("./targets", targets_path);

    // Executing watchdog
    for (size_t i = 0; i < (NUMPROCESS - 1); ++i) {  // this for cycle passes  to pidString all elements of pidList
        sprintf(pidStr[i], "%d", proIds[i]);
    }
    char *wd_path[] = {"./wd", pidStr[SERVER], pidStr[DRONE], pidStr[INPUT], pidStr[OBSTACLES], pidStr[TARGETS], NULL}; 
    sleep(1);
    proIds[WATCHDOG] = spawn("./wd", wd_path);
    
    // Waiting for processes to end
    for(int i = 0; i<4; i++){   //waits for all processes to end
        wait(NULL); 
    }
    
    for (int i = 0; i < NUMPIPE; i++){ // Chiude pipe nel master
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

    fclose(debug);
    fclose(errors);
    return 0;
}