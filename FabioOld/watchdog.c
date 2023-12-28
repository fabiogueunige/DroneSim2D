#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>  
#include <unistd.h> 
#include <stdlib.h> 
#include <semaphore.h> 
#include <sys/mman.h> 
#include <signal.h> 
#include <time.h>
#include <stdarg.h>

#define DEBUG 1

void writeLog(const char *format, ...) {
    
    FILE *logfile = fopen("logfile.txt", "a");
    if (logfile < 0) {
        perror("Error opening logfile");
        exit(EXIT_FAILURE);
    }
    va_list args;
    va_start(args, format);

    time_t current_time;
    time(&current_time);

    fprintf(logfile, "%s => ", ctime(&current_time));
    vfprintf(logfile, format, args);

    va_end(args);
    fflush(logfile);
}



/* 
when signal arrive counter --
when wd send kill counter ++ 
*/

int counter = 0;

//#ifndef DEBUG

/*signal hadler function*/
void sigusr2Handler(int signum, siginfo_t *info, void *context) {
    if(signum == SIGUSR2){
        writeLog("WATCHDOG received signal from %d", info->si_pid);
        counter --;
    }
}    

//#endif
int main(int argc, char *argv[])  
{
    // In this array I will put all the proces pid converted in int
    pid_t pids[argc];
    int i; //declared for all the for cycle
    FILE *wdfile;

    wdfile = fopen("watchd.txt", "w");
    if (wdfile == NULL)
    {
        perror("Error opening file!\n");
        return 1;
        //exit(1);
    }
    fprintf(wdfile, "wd created\n");
    fclose(wdfile);

// #ifndef DEBUG
    /*configure the handler for sigusr2*/
    struct sigaction sa_usr2;
    sa_usr2.sa_sigaction = sigusr2Handler;
    sa_usr2.sa_flags = SA_SIGINFO; // I need also the info foruse the pid of the process for unde
    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1){ 
        perror("sigaction");
        return -1;
    }
    /* convert the pid in argv from dtring to int*/
    for (i = 0; i < argc; i++){
        // convert all the pid form string to int
        pids[i] = atoi(argv[i]);
    }

    while(1){
        /* send a signal to all process */
        for (i = 0; i< argc; i++){  
            /* send signal to all process*/
            kill(pids[i], SIGUSR1);
            /* increment the counter when send the signal*/
            counter ++; 
            sleep(1);
            printf("%d", counter);
            fflush(stdout);
            if(counter == 0){
                /* in this case the proccess is alive*
                /* write into logfile*/   
                writeLog("Process %d is alive", pids[i]);
               
            }else{
                /*The proces doesn't work*/
                /*kill all process*/
                for (int j = 0; j < argc; j++){
                    kill(pids[j], SIGKILL);
                    /*write into logfile that wd close the process*/   
                    writeLog("process %d is closed by WATCHDOG", pids[j]);
                }
                exit(0);
            }    
            sleep(5);     
        }     
    }
// #endif
    return 0;
}

