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
#define NUMPIPE 4 
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
    int portSe = 60000;
    int portTO = 60000;
    char portStrSe[10];
    char portStrTO[10];
    char ipAddress[20] = "IP ADDRESS NOT SET YET";
    int nprc = 0;

    // CREATING PIPE
/*
    pipe 0: input -> drone  
    pipe 1: drone -> server 
    pipe 2: server -> drone 
    pipe 3: master -> description
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
    sprintf(portStrSe, "%d", portSe);
    sprintf(portStrTO, "%d", portTO);


    // processes path
    char * server_path[] = {"./server", piperd[1], pipewr[2],portStrSe, NULL};
    char * drone_path[] = {"./drone", piperd[0],pipewr[1],piperd[2], NULL};
    char * input_path[] = {"./input",pipewr[0], NULL};
    char * obstacles_path[] = {"./obstacles",portStrTO,ipAddress, NULL};
    char * targets_path[] = {"./targets",portStrTO, ipAddress, NULL};

    // INTRO
    char keyy;
    bool right_key= false;
    char * argdes_path[] = {"konsole", "-e","./description", pipewr[3], NULL};
    pidDes = spawn("konsole", argdes_path);
    close (pipe_fd[3][1]);
    usleep(50000);

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

    if ((read(pipe_fd[3][0], &keyy, sizeof(char))) == -1){
        perror("error in reading from pipe");
        writeToLog(errors, "MASTER: error in reading from pipe");
    }
    close(pipe_fd[3][0]);
    if (keyy == 'q'){
        printf("The game is over\n");
        writeToLog(debug, "MASTER: The game is over");
        return 0;
    }
    else if (keyy == 'i'){
        printf("The game is in online mode\n");
        writeToLog(debug, "MASTER: The game is in online mode");
    }
    else if (keyy == 't'){
        printf("The game is in online mode with targets and obstacles\n");
        writeToLog(debug, "MASTER: The game is in online mode with targets and obstacles");
    }
    else{
        printf("The game is in local mode\n");
        writeToLog(debug, "MASTER: The game is in local mode");
        right_key = true;
    }

    // waiting to close the description
    if ((waitpid(pidDes, NULL, 0)) == -1){
        perror("waitpid");
        writeToLog(errors, "MASTER: error in waitpid for description");
    }
    
    if (keyy == 'i' || right_key){
        nprc = 3;
        proIds[SERVER] = spawn("./server", server_path);
        usleep(500000);            
        proIds[DRONE] = spawn("./drone", drone_path);
        usleep(500000);
        proIds[INPUT] = spawn("./input", input_path);
        usleep(500000);
        for (size_t i = 0; i < (nprc); ++i) {  // this for cycle passes  to pidString all elements of pidList
            sprintf(pidStr[i], "%d", proIds[i]);
        }
    }
    if (keyy == 't' || right_key){
        nprc = 5;
        proIds[OBSTACLES] = spawn("./obstacles", obstacles_path);
        usleep(500000);
        proIds[TARGETS] = spawn("./targets", targets_path); 
    }
    if (keyy != 't'){
        char *wd[] = {"./wd", pidStr[SERVER], pidStr[DRONE], pidStr[INPUT], NULL};
        sleep(1);
        writeToLog(debug, "MASTER: Watchdog for input game is running");
        proIds[WATCHDOG] = spawn("./wd", wd);
    }

    // Waiting for processes to end
    for(int i = 0; i < (nprc + 1); i++){  
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
