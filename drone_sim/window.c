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
#include <stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/select.h>
#include <math.h>
#include <errno.h>
#define T 0.1 //s   time instants' duration

float rho2 = 2;


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

typedef struct {
    int x;
    int y;
    bool taken;
} targets;

typedef struct {
    char c;
    char r;
} Border;

typedef struct {
    int startx, starty;
    int height, width;
    Border border;
} Win;

void writeToLog(FILE *logFile, const char *message) {
    time_t crtime;
    time(&crtime);
    fprintf(logFile,"%s => ", ctime(&crtime));
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
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

void init_win (Win *p_win, int height, int width, int starty, int startx)
{
    p_win->height = height;
    p_win->width = width;
    p_win->starty = starty;
    p_win->startx = startx;
    p_win->border.c = '|';
    p_win->border.r = '-';
}

void borderCreation (Win *p_win, WINDOW *w)
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_BLACK);
    for (int j = 0; j < p_win->width; j++)
    {
        wattron(w, COLOR_PAIR(1) | A_BOLD);
        mvwaddch(w, 0, j, p_win->border.r);
        mvwaddch(w, p_win->height - 1, j, p_win->border.r);
        wattroff(w, COLOR_PAIR(1) | A_BOLD);
        // 
    }
    for (int i = 0; i < p_win->height; i++)
    {
        wattron(w, COLOR_PAIR(1) | A_BOLD);
        mvwaddch(w, i, 0, p_win->border.c);
        mvwaddch(w, i, p_win->width - 1, p_win->border.c);
        wattroff(w, COLOR_PAIR(1) | A_BOLD);
        
    }  
    
}


void destroy_win(WINDOW *local_win) {
    werase(local_win); // Clear window content
    wrefresh(local_win); // Refresh to show changes
    delwin(local_win); // Delete the window
}

bool isTargetTaken(int x, int y, int xt, int yt){
    float rho = sqrt(pow(x-xt, 2) + pow(y-yt, 2));
    if(rho < rho2)
        return true;
}

int main(char argc, char*argv[]){
    // resizing the window
    //sleep(1);
    //char *get_wid = "xdotool search --onlyvisible --name window";


    FILE * debug = fopen("logfiles/debug.log", "a");
	FILE * winfile = fopen("logfiles/window.log", "w");    // debug log file
	FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    
    if (winfile == NULL || debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }

    writeToLog(debug, "WINDOW: process started");
    WINDOW * win;
    Drone * drone;
    Drone dr;
    drone = &dr;
    Win  winpar;
    char symbol = '%';	// '%' is the symbol of the drone
    int nedges, nobstacles = 0, ntargets = 0;
    // char edge_symbol = '#';
    char obs_symbol = 'o';
    char tar_symbol = 'T';
    int pipeSefd;
    int rows, cols;
    int srows, scols, newrows, newcols;
    int x;
	int y;
    float vx, vy;
    int fx, fy;
    int counter = 0, countertot = 0;

    char msg[100]; // for writing to log files

    initscr(); // start curses mode
    
    raw();
    noecho();
    start_color();
    curs_set(0);
    writeToLog(winfile, "WINDOW: curses mode started");

    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // for info
    init_pair(3, COLOR_RED, COLOR_BLACK); // for obstacles
    init_pair(4, COLOR_GREEN, COLOR_BLACK); // for targets
    init_pair(5, COLOR_CYAN, COLOR_BLACK); // for drones
    writeToLog(winfile, "WINDOW: colors initialized");


    if (stdscr == NULL) {
        // Gestisci l'errore di inizializzazione di ncurses
        fprintf(errors, "WINDOW: Errore durante l'inizializzazione di ncurses\n");
        return 1; // Esce con un codice di errore
    }
    
    
    if((getmaxyx(stdscr, srows, scols) == ERR)){
        // Gestisci l'errore di inizializzazione di ncurses
        fprintf(errors, "WINDOW: Errore durante l'inizializzazione di ncurses\n");
        return 1; // Esce con un codice di errore
    }
    writeToLog(winfile, "WINDOW: max rows and cols taken");
    if (srows == ERR || scols == ERR) {
        // Gestisci l'errore di inizializzazione di ncurses
        fprintf(errors, "WINDOW: Errore durante l'inizializzazione di ncurses\n");
        return 1; // Esce con un codice di errore
    }
    if (refresh() == ERR) {
        // Gestisci l'errore di inizializzazione di ncurses
        fprintf(errors, "WINDOW: Errore durante l'inizializzazione di ncurses\n");
        return 1; // Esce con un codice di errore
    }
    

// SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }
    // OPENING PIPE
    
    fd_set readfds;
    pipeSefd = atoi(argv[1]);
    writeToLog(debug, "WINDOW: pipe opened");
    FD_ZERO(&readfds);
    FD_SET(pipeSefd, &readfds);
    writeToLog(winfile, "WINDOW: pipe set");

    // Read Raws and Cols
    if((read(pipeSefd, &rows, sizeof(int))) == -1){
        perror("read");
        writeToLog(errors, "WINDOW: error in read rows");
        exit(EXIT_FAILURE);
    }
    if((read(pipeSefd, &cols, sizeof(int))) == -1){
        perror("read");
        writeToLog(errors, "WINDOW: error in read cols");
        exit(EXIT_FAILURE);
    }
    // Checking pipe functionality
  
    sprintf(msg, "WINDOW: rows = %d, cols = %d", rows, cols);
    writeToLog(winfile, msg);
    delwin(stdscr);
    stdscr = newwin(rows +5, cols +5, 0, 0);
    refresh();
    // generating window after server send rows and cols
    // Creo la window al centro della console con le dimensioni date dal server
    win = newwin(rows, cols, 0, 0);
    box(win, 0, 0);
    init_win(&winpar, rows, cols, 0, 0);
    
    if (srows >= rows && scols >= cols){
        mvwin(win, (((srows - rows)/2) + rows), (((scols - cols)/2) + cols));
        writeToLog(winfile, "WINDOW: window moved after resizing");
    }

    writeToLog(winfile, "WINDOW: window created");
    struct obstacle * obs[20];
    targets * tar[20];
	
    while(1){
        
        char buffer[4];
        FD_SET(pipeSefd, &readfds);
        int sel;
        do{
            sel = select(pipeSefd + 1, &readfds, NULL, NULL, NULL);
        }while(sel == -1 && errno == EINTR);
        if (sel == -1){
            perror("select");
            writeToLog(errors, "WINDOW: error in select()");
            exit(EXIT_FAILURE);
        }
        else if(sel>0){
            if((read(pipeSefd, buffer, sizeof(buffer)-1)) == -1){
                perror("read");
                writeToLog(errors, "WINDOW: error in read buffer");
                exit(EXIT_FAILURE);
            }
            writeToLog(winfile, buffer);
            //buffer[numRead] = '\0';
            if(strcmp(buffer, "obs") == 0){
                // datas for obstacles
                //writeToLog(winfile, "WINDOW: reading obstacles");
                
                if ((read(pipeSefd, &nobstacles, sizeof(int))) == -1){
                    perror("read");
                    writeToLog(errors, "WINDOW: error in read nobstacles");
                    exit(EXIT_FAILURE);
                }
                sprintf(msg, "WINDOW: number of obstacles %d", nobstacles);
                writeToLog(winfile, msg);
                // struct obstacle * obs[nobstacles];
                for (int i = 0; i<nobstacles; i++){
                    obs[i] = malloc(sizeof(struct obstacle));
                    if ((read(pipeSefd, obs[i], sizeof(struct obstacle))) == -1){
                        perror("read");
                        writeToLog(errors, "WINDOW: error in read obstacles");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(msg, "WINDOW: obstacle %d: x = %d, y = %d", i, obs[i]->x, obs[i]->y);
                    writeToLog(winfile, msg);
                }
            }
            else if(strcmp(buffer, "tar") == 0){
                // reading targets from server
                if((read(pipeSefd, &ntargets, sizeof(int))) == -1){
                    perror("read");
                    writeToLog(errors, "WINDOW: error in read ntargets");
                    exit(EXIT_FAILURE);
                }
                countertot += ntargets;
                char pos_targets[ntargets][10];
                sprintf(msg, "WINDOW: number of targets %d", ntargets);
                writeToLog(winfile, msg);
                for (int i = 0; i<ntargets; i++){
                    tar[i] = malloc(sizeof(targets));
                    if ((read(pipeSefd, tar[i], sizeof(targets))) == -1){
                        perror("read");
                        writeToLog(errors, "WINDOW: error in read targets");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(pos_targets[i], "%d,%d", tar[i]->x, tar[i]->y);
                    writeToLog(winfile, pos_targets[i]);
                }
            }
            else if(strcmp(buffer, "coo") == 0){
                // read the coordinates
                //drone =malloc(sizeof(Drone));
                if ((read(pipeSefd, drone, sizeof(Drone))) == -1){
                    perror("read");
                    writeToLog(errors, "WINDOW: error in read drone from Server");
                    exit(EXIT_FAILURE);
                }
                // writeToLog(winfile, "WINDOW: reading drone");
            }  
        }
		x = drone->x;
		y = drone->y;
        vx = (drone->vx) - 5;
        vy = (drone->vy) - 5;
        fx = drone->fx;
        fy = drone->fy;

        wclear(win);
        borderCreation(&winpar, win);
        wattron(win, COLOR_PAIR(5) | A_BOLD);
		mvwprintw(win, y, x, "%c", symbol);
        wattroff(win, COLOR_PAIR(5) | A_BOLD);

        // printing obstacles
        if (nobstacles != 0) {
            for (int i = 0; i<nobstacles; i++){
                wattron(win, COLOR_PAIR(3) | A_BOLD);
                mvwprintw(win, obs[i]->y, obs[i]->x, "%c", obs_symbol);
                wattroff(win, COLOR_PAIR(3) | A_BOLD);
            }
        }
        
        // printing targets
        if (ntargets != 0) {
            for (int i = 0; i<ntargets; i++){
                if((tar[i]->taken == false) && (isTargetTaken(x,y,tar[i]->x, tar[i]->y))){ // if coordinates of drone and target are the same
                    tar[i]->taken = true; // target is taken
                    counter++;
                    sprintf(msg, "WINDOW: target %d taken and not in window anymore", i);
                    writeToLog(winfile, msg);
                }
                    
                
                if(tar[i]->taken == false){
                    wattron(win, COLOR_PAIR(4) | A_BOLD);
                    mvwprintw(win, tar[i]->y, tar[i]->x, "%c", tar_symbol);
                    wattroff(win, COLOR_PAIR(4) | A_BOLD);
                }
            }
        }
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(win, 0, 5, "X: %d, Y: %d, Vx: %f m/s, Vy: %f m/s, Fx: %d N, Fy: %d N, Score: %d / %d", x, y, vx, vy, fx, fy, counter, countertot);
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
        wrefresh(win);  // Print changes to the screen
    }

    destroy_win(win);
	endwin();	// end curses mode
    free(obs);
    free(tar);
    close(pipeSefd);
    fclose(debug);
    fclose(winfile);
    fclose(errors);

    return 0;
}
