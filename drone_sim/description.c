#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <string.h> 
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include "library/win.h"

#define DEBUG 1


int main (int argc, char* argv[])
{
    // WINDOW *external_window;
    // WINDOW *printing_window;
    WINDOW *arrw[NUMWINDOWS]; // change numwindows in library
    char ch;
    int i, index = 0;
    // change with the new windows.c and the new functiones
    
    int nrows, ncols;
    int h,w;
  
    initscr(); // start curses mode
    raw();
    noecho();
    start_color();
    keypad(stdscr, TRUE);

    getmaxyx(stdscr, h, w);
    refresh();
    
    // Windows initialization
    //init_windows(Srow, Scol, &external_window, &printing_window,&PRy,&PRx,&Startx,&Starty,&Wcol,&Wrow);

    refresh();    
    init_pair(1,COLOR_GREEN,COLOR_BLACK);
    squareCreation(arrw,h,w, &nrows,&ncols);
    printw("Description of the game: \n");
    printw("\nThis game is a simple game of a drone control, where the user can press \nthis buttons to drive the robot to avoid the obstacles and reach the targets.\n");
    // wrefresh(external_window);
    refresh();
    // wrefresh(printing_window);

    for (int i = 0; i < NUMWINDOWS; i++){
        switch (i)
        {
            case BTTW:
                // arrw[i] = create_new_window(Wrow, Wcol, (CBstarty-Wrow), (CBstartx-Wcol));
                print_btt_windows(arrw[i], 'w');
                break;
            case BTTE:
                // arrw[i] = create_new_window(Wrow*2, Wcol, (CBstarty-(Wrow*2)), CBstartx);
                print_btt_windows(arrw[i], 'e');
                break;
            case BTTR:  
                // arrw[i] = create_new_window(Wrow, Wcol, (CBstarty-Wrow), (CBstartx+Wcol));
                print_btt_windows(arrw[i], 'r');
                break;
            case BTTS:
                //arrw[i] = create_new_window(Wrow, Wcol*2, CBstarty, CBstartx-(Wcol*2));
                print_btt_windows(arrw[i], 's');
                break;
            case BTTD:
                print_btt_windows(arrw[i], 'd');
                //arrw[i] = create_new_window(Wrow, Wcol, Starty, Startx);
                break;
            case BTTF:
                //arrw[i] = create_new_window(Wrow, Wcol*2, CBstarty, CBstartx+(Wcol));
                print_btt_windows(arrw[i], 'f');
                break;
            case BTTX:
                // arrw[i] = create_new_window(Wrow, Wcol, (CBstarty+Wrow), (CBstartx-Wcol));
                print_btt_windows(arrw[i], 'x');
                break;
            case BTTC:
                // arrw[i] = create_new_window(Wrow*2, Wcol, (CBstarty+Wrow), CBstartx);
                print_btt_windows(arrw[i], 'c');
                break;
            case BTTV:
                // arrw[i] = create_new_window(Wrow, Wcol, (CBstarty+Wrow), (CBstartx+Wcol));
                print_btt_windows(arrw[i], 'v');
                break;
            case BTTQ:
                // arrw[i] = create_new_window(Wrow, Wcol, (CBstarty-(Wrow*2)), CBstartx-(Wcol*2));
                // print_btt_windows(arrw[i], 'Q');
                break;
            case BTTB:
                // print_btt_windows(arrw[i], 'B');
                break;
            default:
                printf("Error in the switch case\n");
                break;
        }
    }

    printw("\nPress any key to exit\n");
    refresh();
    ch = getch();
    printw("Enjoy");
    refresh();
    for (i = 0; i < NUMWINDOWS; i++)
    {
        destroy_win(arrw[i]);
    }
    endwin();

    return 0;
}
    
