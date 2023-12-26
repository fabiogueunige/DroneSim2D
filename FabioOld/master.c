#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <stdarg.h>

#define NUMPROCESS 4
#define SERVER 0
#define DRONE 1
#define INPUT 2
#define WATCHDOG 3

int newprocess (int num, char* pname, char** arglist[])
{
    pid_t pid;
    if ((pid = fork()) == -1) {
        perror("fork");
        printf("error in fork of %d\n",num);
        return -1;
    }
    if (pid == 0) {
        // child process
        if (execvp(pname, arglist) == -1){
            perror("exec failed");
            printf("\nerror in exec of %d\n",num);
            return -2;
        }
        else{
        //parent process
            return pid;
        }
    }
}
/*
void writeLog(const char *format, ...) {
    
    FILE *logfile = fopen("logfile.txt", "a");
    if (logfile < 0) {
        perror("Error opening logfile");
        exit(EXIT_FAILURE);
    }
    va_list args;
    va_start(args, format);

    time_t crtime;
    time(&crtime);

    fprintf(logfile, "%s => ", ctime(&crtime));
    vfprintf(logfile, format, args);

    va_end(args);
    fflush(logfile);
}
*/


int main ()
{
    int pipesfd[NUMPROCESS + 1][2]; // not consideing the pipe of wd
    int i;
    int waitpid[NUMPROCESS];
    pid_t pid[NUMPROCESS]; // child pids
    char pidstr[NUMPROCESS][70]; //from int to string
    pid_t pid_des;
   
    //Inizialize the log file, inizialize with mode w, all the data inside will be delete
    // implements the pipe such as the code vault ones using the new part from Andrea
    
    FILE *logfile = fopen("logfile.txt", "w");
    if(logfile < 0){ //if problem opening file, send error
        perror("fopen: logfile");
        return 1;
    }else{
        //wtite in logfile
        time_t crtime;
        //obtain local time
        time(&crtime);
        fprintf(logfile, "%s => create master with pid %d\n", ctime(&crtime), getpid());
        fclose(logfile);
    }
    // now we start showing the description of th game
    
    printf("Welcome to the Drone Game!\n");
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
    }else{
        //parent process
        wait(NULL);
    }
    //Now we start the game, so it is needed to create all
    sleep(2);

    for (i = 0; i < NUMPROCESS + 1; i++) {
        if (pipe(pipesfd[i]) == -1) {
            perror("pipe");
            printf("error in pipe %d\n",i);
            return 2;
        }
    }
    // IT's temporary because of the shared memory
    for (i = 0; i < NUMPROCESS + 1; i++) {
        if (i != DRONE || i != INPUT) {
            close(pipesfd[i][0]);
        }
        if (i != DRONE || i != NUMPROCESS) {
            close(pipesfd[i][1]);
        }
    }

    char * argserver[] = {NULL};
    pid[SERVER] = newprocess(SERVER, "./server", argserver);
    // Now it's needed to give the pipes to input and drone
    sprintf(pidstr[DRONE], "%d", pipesfd[DRONE][0]);
    sprintf(pidstr[INPUT], "%d", pipesfd[INPUT][1]);

    char * argdrone[] = {pidstr[DRONE],pidstr[INPUT],NULL};
    pid[DRONE] = newprocess(DRONE, "./drone", argdrone);
    sprintf(pidstr[INPUT], "%d", pipesfd[DRONE][0]);
    sprintf(pidstr[0], "%d", pipesfd[NUMPROCESS][1]);

    char * arginput[] = {pidstr[DRONE],pidstr[INPUT],NULL};
    pid[INPUT] = newprocess(INPUT, "./input", arginput);

    sleep(1);

    for (i = 0; i< NUMPROCESS; i++) {
        sprintf(pidstr[i], "%d", pid[i]);
    }

    char * argwatchdog[] = {pidstr[SERVER], pidstr[DRONE], pidstr[INPUT], NULL};
    pid[WATCHDOG] = newprocess(WATCHDOG, "./watchdog", argwatchdog);

    // just for now
    close (pipesfd[NUMPROCESS][1]);
    close (pipesfd[INPUT][0]);
    close (pipesfd[DRONE][0]);
    close (pipesfd[INPUT][1]);

    pid_t waitResult;
    int status;
    for (i = 0; i <= NUMPROCESS; i++)
    {/*
        waitResult = waitpid(pid[i], &status, 0);
        if(waitResult == -1){
            perror("waitpid:");
            return 3;
        }
        if (WIFEXITED(status)) {
            printf("Process %d is termined with status %d\n", i, WEXITSTATUS(status));
        } else {
            printf("Iprocess %d is termined with anomaly\n", i);
        }
        */
       wait(NULL);
    }
    return 0;
}