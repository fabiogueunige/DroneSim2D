#ifndef WIN_H
#define WIN_H
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

void print_btt_windows(WINDOW*, char);
void boxCreation(WINDOW **win, int *maxY, int *maxX);
// New functions
void destroy_win(WINDOW *local_win);
void squareCreation (WINDOW **, int , int ,int *, int *);
void lightWindow(WINDOW *, chtype );
void printCounter(WINDOW *, int );


#endif
