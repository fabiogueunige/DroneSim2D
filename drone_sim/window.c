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
#define T 0.1 //s   time instants' duration


typedef struct {
    int x;
    int y;
    float vx,vy;
    int fx, fy;
} Drone;


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
	printf("WINDOW: process started\n");
	FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
	FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    writeToLog(debug, "WINDOW: process started");

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

	int x;
	int y;
    float vx, vy;
    int fx, fy;
    pid_t fsfa = getpid();
    char s [50];
    sprintf(s, "WINDOW: %d", fsfa);
    writeToLog(debug, s);
	while(1){
		x = drone->x;
		y = drone->y;
        vx = (drone->vx) - 5;
        vy = (drone->vy) - 5;
        fx = drone->fx;
        fy = drone->fy;
		clear();
		mvprintw(y, x, "%c", symbol);
        mvprintw(LINES - 1, COLS - 80, "X: %d, Y: %d, Vx: %f m/s, Vy: %f m/s, Fx: %d N, Fy: %d N", x, y, vx, vy, fx, fy);
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