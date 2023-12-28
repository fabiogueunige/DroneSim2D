// Last update: 10:00 14/11/2017
#ifndef WINDOW_H
#define WINDOW_H
#include <ncurses.h>
#include <curses.h>
#include <time.h>

# define NUMWINDOWS 11
# define NUMMOTIONS 9
# define BTTW 0
# define BTTE 1
# define BTTR 2
# define BTTS 3
# define BTTD 4
# define BTTF 5
# define BTTX 6
# define BTTC 7
# define BTTV 8
# define BTTB 9
# define BTTQ 10





/*
Remember to put the windows for row in the array that you'll create
*/
/*
WINDOW *central_butt; // central button

WINDOW *up_butt; 
WINDOW *down_butt;
WINDOW *left_butt;
WINDOW *right_butt;

WINDOW *up_left_butt;
WINDOW *up_right_butt;
WINDOW *down_left_butt;
WINDOW *down_right_butt;
*/


// WINDOW *create_new_window(int , int , int , int );

// void init_windows(int, int, WINDOW**, WINDOW**, int*, int*, int*, int*,int*, int*);
void print_btt_windows(WINDOW*, char);
void boxCreation(WINDOW **win, int *maxY, int *maxX);
// New functions
void destroy_win(WINDOW *local_win);
void squareCreation (WINDOW **, int , int ,int *, int *);
void lightWindow(WINDOW *, chtype );
void printCounter(WINDOW *, int );


#endif