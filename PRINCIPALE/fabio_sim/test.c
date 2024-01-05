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
    init_pair(1,COLOR_RED,COLOR_BLACK);
    for (int j = 0; j < p_win->width; j++)
    {
        mvwaddch(w, 0, j, p_win->border.r);
        wattron(w, COLOR_PAIR(1) | A_BOLD);
        
        mvwaddch(w, p_win->height - 1, j, p_win->border.r);
        wattron(w, COLOR_PAIR(1) | A_BOLD);
    }
    for (int i = 0; i < p_win->height; i++)
    {
        mvwaddch(w, i, 0, p_win->border.c);
        wattron(w, COLOR_PAIR(1) | A_BOLD);
        mvwaddch(w, i, p_win->width - 1, p_win->border.c);
        wattron(w, COLOR_PAIR(1) | A_BOLD);
    }  
    wrefresh(w);
}

void destroy_win(WINDOW *local_win) {
    werase(local_win); // Clear window content
    wrefresh(local_win); // Refresh to show changes
    delwin(local_win); // Delete the window
}

int main(char argc, char*argv[]){
	
    
	FILE * winfile = fopen("logfiles/window.log", "w");    // debug log file
	

    WINDOW * win;
    Win winpar;
    char symbol = '%';	// '%' is the symbol of the drone
    int nedges, nobstacles;
    char edge_symbol = '#';
    char obs_symbol = 'o';
    int rows, cols;
    int srows, scols;
    int newrows, newcols;
   

    initscr(); // start curses mode
    raw();
    noecho();
    start_color();

    // Starting Window
    getmaxyx(stdscr, srows, scols);
    refresh();
    win = newwin(srows, scols, 0, 0);

    // Riceve la pipe e si ridimensiona!
    rows = 50;
    cols = 100;
    wresize(stdscr, (rows + 10), (cols + 10));
    refresh();
    wresize(win, rows, cols);
    mvwin(win, 5, 5);
    wrefresh(win);
    writeToLog(winfile, "WINDOW: window created");
    init_win(&winpar, rows, cols, 0, 0);
    writeToLog(winfile, "WINDOW: win initialized");
    borderCreation(&winpar, win);
    writeToLog(winfile, "WINDOW: border created");
    wrefresh(win);
    /*
    while (1)
    {
        getmaxyx(stdscr, newrows, newcols);
        if (newrows != srows || newcols != scols)
        {
            rows = newrows;
            cols = newcols;
            wresize(stdscr, (rows + 10), (cols + 10));
            refresh();
            wresize(win, rows, cols);
            mvwin(win, 5, 5);
            wrefresh(win);
            clear();
            refresh();
            wclear(win);
            init_win(&winpar, rows, cols, 0, 0);
            writeToLog(winfile, "WINDOW: win initialized");
            borderCreation(&winpar, win);
            writeToLog(winfile, "WINDOW: border created");
            wrefresh(win);

        }
        sleep(10);
    }
    */

    
    sleep(20);
	


    destroy_win(win);
    
	endwin();	// end curses mode

    fclose(winfile);
    return 0;
}