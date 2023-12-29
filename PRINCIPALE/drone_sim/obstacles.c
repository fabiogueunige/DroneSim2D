#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <time.h>


#define MAX_OBSTACLES 20

pid_t wd_pid = -1;
bool exit_flag = false;

struct obstacle {
    int x;
    int y;
};

typedef struct {
    int rows;
    int cols;
    int nobstacles;
} window;

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "OBSTACLES: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "OBSTACLES: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
    
}

void writeToLog(FILE * logFile, const char *message) {
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
}


int main (int argc, char *argv[]) 
{
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    writeToLog(debug, "OBSTACLES: process started");
    printf("OBSTACLES: process started\n");
    struct window *window;

    // opening pipes
    int pipeSefd[2];
    sscanf(argv[1], "%d", &pipeSefd[0]);
    sscanf(argv[2], "%d", &pipeSefd[1]);
    writeToLog(debug, "OBSTACLES: pipes opened");
    
    // these var are used because there aren't pipes, but these values are imported by server
    int rows = 100;
    int cols = 100;
    srand(time(NULL));
    int nobstacles = rand() % MAX_OBSTACLES;

    /* for 3d assignment :
    if (nobstacles>20){
        printf("OBSTACLES: too many obstacles, max 20\n");
    }
    */
    
    char pos_obstacles[nobstacles][10];
    char pos_edges[2*(rows+cols)][10];
    int nobstacles_edge = 2 * (rows + cols);
    int strlength;
    strlength = 10 * nobstacles + 10;

    char pos_all_obs[strlength];


    struct obstacle *obstacles[nobstacles];
    struct obstacle *edges[nobstacles_edge];

    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "INPUT: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
        perror("sigaction");
        writeToLog(errors, "INPUT: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    // create obstacles
    for (int i = 0; i < nobstacles; i++){
        obstacles[i] = malloc(sizeof(struct obstacle)); //allocate memory for each obstacle
        obstacles[i]->x = rand() % cols;
        obstacles[i]->y = rand() % rows;
        int x = obstacles[i]->x;
        int y = obstacles[i]->y;
        printf("OBSTACLES: obstacle %d created at (%d, %d)\n", i, x, y);
        fprintf(debug, "OBSTACLES: obstacle %d created at (%d, %d)\n", i, x, y);
        sprintf(pos_obstacles[i], "%d,%d", x, y);
        // write to server with pipe ...
    }

    // create edges
    for (int i = 0; i< rows; i++){
        edges[i] = malloc(sizeof(struct obstacle));
        edges[i]->x = 0;
        edges[i]->y = i;
        edges[i+rows+cols] = malloc(sizeof(struct obstacle));
        edges[i+rows+cols]->x = cols-1;
        edges[i+rows+cols]->y = i;
        printf("OBSTACLES: edge %d created at (%d, %d)\n", i, edges[i]->x, edges[i]->y);
        printf("OBSTACLES: edge %d created at (%d, %d)\n", i+rows+cols, edges[i+rows+cols]->x, edges[i+rows+cols]->y);
        sprintf(pos_edges[i], "%d,%d", edges[i]->x, edges[i]->y);
        sprintf(pos_edges[i+rows+cols], "%d,%d", edges[i+rows+cols]->x, edges[i+rows+cols]->y);
        // write to server with pipe ...
    }
    
    for (int i = rows; i< rows+cols; i++){
        edges[i] = malloc(sizeof(struct obstacle));
        edges[i]->x = i;
        edges[i]->y = 0;
        edges[i+rows+cols] = malloc(sizeof(struct obstacle));
        edges[i+rows+cols]->x = i;
        edges[i+rows+cols]->y = cols-1;
        printf("OBSTACLES: edge %d created at (%d, %d)\n", i, edges[i]->x, edges[i]->y);
        printf("OBSTACLES: edge %d created at (%d, %d)\n", i+rows+cols, edges[i+rows+cols]->x, edges[i+rows+cols]->y);
        sprintf(pos_edges[i], "%d,%d", edges[i]->x, edges[i]->y);
        // write to server with pipe ...
    }

    // closing pipes
    
    for (int i = 0; i < 2; i++){
        if (close(pipeSefd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "OBSTACLES: error in closing pipe");
        }
    }

    return 0;
}