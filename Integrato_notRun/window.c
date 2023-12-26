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
typedef enum {NO=0, UP=1, DOWN=2, LEFT=3, RIGHT=4} EDGE;


typedef struct {
    int x;
    int y;
    EDGE isOnEdgex;
    EDGE isOnEdgey;
} Drone;

typedef struct{
    bool exit_flag;
} Flag;

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

int main(char argc, char*argv[]){
	printf("WINDOW: process started\n");
	FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
	FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    writeToLog(debug, "WINDOW: process started");

    initscr();	//Start curses mode 
	Drone * drone;
    Flag * flags;
	char symbol = '%';	// '%' is the symbol of the drone
	
	curs_set(0);

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

	while(1){
        
		x = drone->x;
		y = drone->y;
        
		clear();
		mvprintw(y, x, "%c", symbol);
        mvprintw(LINES - 1, COLS - 20, "X: %d, Y: %d", x, y);
        refresh();  // Print changes to the screen

	}

	// CLOSE AND UNLINK SHARED MEMORY
    if (close(shm_fd) == -1) {
		perror("close");
        exit(EXIT_FAILURE);
    }
	/*
    if (shm_unlink(shm_name) == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }*/
    munmap(drone, SIZE);

	endwin();	// end curses mode

    return 0;
}