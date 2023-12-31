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
#include <ncurses.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/select.h>
#define T 0.1 //s   time instants' duration


typedef struct {
    int x;
    int y;
    float vx,vy;
    int fx, fy;
} Drone;

struct obstacle {
    int x;
    int y;
};

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

void sig_handler(int signo, siginfo_t *info, void *context) {
    FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
    if (signo == SIGUSR1) {
        printf("WINDOW: terminating with value 0...\n");
        writeToLog(debug, "WINDOW: terminating with value 0...");
        exit(EXIT_SUCCESS);
    }
    fclose(debug);
}
int main(char argc, char*argv[]){
	
	FILE * windebug = fopen("logfiles/windebug.log", "w");    // debug log file
	FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    
    writeToLog(windebug, "WINDOW: process started");

    initscr();	//Start curses mode 
	Drone * drone;

    
	char symbol = '%';	// '%' is the symbol of the drone
	curs_set(0);

// SIGNALS
struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

// SHARED MEMORY OPENING AND MAPPING
	const char * shm_name = "/dronemem";
    const char * shm_name_flags = "/flagsmem";
    const int SIZE = 4096;

    int shm_fd_flags, shm_fd;
    shm_fd = shm_open(shm_name, O_RDONLY, 0666); // open shared memory segment
    if (shm_fd == 1) {
        perror("shared memory segment failed\n");
        writeToLog(errors, "DRONE:shared memory segment failed");
        exit(EXIT_FAILURE);
    }
    
    drone = (Drone *)mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0); // protocol write
    if (drone == MAP_FAILED) {
        perror("Map failed\n");
        writeToLog(errors, "WINDOW: map failed for drone");
        return 1;
    }

    // OPENING PIPE
    int pipeSefd;
    fd_set readfds;
    pipeSefd = atoi(argv[1]);
    writeToLog(windebug, "WINDOW: pipe opened");
    FD_ZERO(&readfds);
    FD_SET(pipeSefd, &readfds);
    // EDGE IMPORT FROM SERVER
    int nedges;
    char edge_symbol = '#';
    read(pipeSefd, &nedges, sizeof(int));

    struct obstacle * edges[nedges];
    
    //printf("WINDOW: nedges = %d\n", nedges);
    
    char a[7];
    sprintf(a, "%d", nedges);
    writeToLog(windebug, a);
    for(int i = 0; i<nedges; i++){
        //writeToLog(debug, "WINDOW: reading edges");
        int sel = select(pipeSefd + 1, &readfds, NULL, NULL, NULL);
        if (sel == -1){
            perror("select");
            writeToLog(errors, "WINDOW: error in select()");
            exit(EXIT_FAILURE);
        }
        else if(sel>0){
            
            read(pipeSefd, &edges[i], sizeof(struct obstacle));
            //printf("WINDOW: edge %d: x = %d, y = %d \n", i, edges[i]->x, edges[i]->y);
            writeToLog(windebug, "WINDOW: edge read");
            /*
            char ksdhc[50];
            sprintf(ksdhc, "WINDOW: edge %d: x = %d, y = %d \n", i, edges[i]->x, edges[i]->y);
            writeToLog(debug, ksdhc);*/
        
        }
        else{
            writeToLog(errors, "WINDOW: select() timeout");
        break; // exit the loop if select() returns 0
    }
        //read(pipeSefd, &edges[i], sizeof(struct obstacle));
        //sprintf(debug, "WINDOW: edge %d: x = %d, y = %d \n", i, edges[i].x, edges[i].y);
    }

	int x;
	int y;
    float vx, vy;
    int fx, fy;

	while(1){
        writeToLog(windebug, "WINDOW: cycle");
		x = drone->x;
		y = drone->y;
        vx = (drone->vx) - 5;
        vy = (drone->vy) - 5;
        fx = drone->fx;
        fy = drone->fy;
		clear();
		mvprintw(y, x, "%c", symbol);
        mvprintw(LINES - 1, COLS - 80, "X: %d, Y: %d, Vx: %f m/s, Vy: %f m/s, Fx: %d N, Fy: %d N", x, y, vx, vy, fx, fy);
        /*
        for(int i = 0; i<nedges; i++){
            int ex = edges[i].x;
            int ey = edges[i].y;
            mvprintw(ex, ey, "%c", edge_symbol);
        }*/

        refresh();  // Print changes to the screen
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
    munmap(drone, SIZE);

	endwin();	// end curses mode

    return 0;
}