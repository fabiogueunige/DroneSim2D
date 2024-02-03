#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <ncurses.h>
#include "library/win.h"

 #define DEBUG 1

#define MAX 30
void counterImplementation (int *cnt, char ct, int lung);

int main (int argc, char* argv[])
{
    WINDOW *cwin[NUMWINDOWS + 1];
    int counter[4] = {0,0,0,0}; // s, e, f, c
    char realchar = '\0';
    int lng = 4;
    int ch;
    int i;
    int index = 0;
    int pipefd[argc];
// #ifndef DEBUG
    for (i = 0; i < argc; i++) { // something is wrong here
        pipefd[i] = atoi(argv[i]); // converts from the string to the integer
    }
    close(pipefd[0]);
// #endif
    initscr();
    raw();
    noecho();
    start_color();
    keypad(stdscr, TRUE);
    

    int hight, width;
    int shight, swidth;
    getmaxyx(stdscr, hight, width);
    refresh();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);

    // Calcola le dimensioni per i sottoquadrati
    squareCreation(cwin, hight, width, &shight, &swidth); //giving the array
    //mvwprintw(cwin[BTTW],0,0,"Scompaiono adesso");
    wrefresh(cwin[BTTW]);
    //sleep(1);
    // while to get char
    while (1) {
        ch = getch();
        realchar = (char) ch;
        //mvwprintw(cwin[BTTW],0,0,"Scompaiono fienstre");
        wrefresh(cwin[BTTW]);
        switch (ch)
        {
        case 'w': // up left
            index = BTTW;
            break;
        case 'e': // up
            index = BTTE;
            break;
        case 'r': // up right
            index = BTTR;
            break;
        case 's': // left
            index = BTTS;
            break;
        case 'd': // delete forces
            index = BTTD;
            break;
        case 'f': // right
            index = BTTF;
            break;
        case 'x': // Down left
            index = BTTX;
            break;
        case 'c': // Down
            index = BTTC;
            break;
        case 'v': // down Right
            index = BTTV;
            break;
        case 'p':
            realchar = 'p'; // pause command
            break;
        case 'b':
            realchar = 'b'; // break
            index = BTTB;
            break;
        case 'q':
            realchar = 'Q'; // Termina il programma
            index = BTTQ;
            mvprintw(0, 0, "Closing the program\n");
            sleep(3);
            break;
        default:
            realchar = '\0'; // Comando non valido
            break;
        }

        counterImplementation(counter, realchar, lng);
        //lightWindow(cwin[index], COLOR_PAIR(1) | A_BOLD);
        printCounter(cwin[BTTS], counter[0]);
        printCounter(cwin[BTTE], counter[1]);
        printCounter(cwin[BTTF], counter[2]);
        printCounter(cwin[BTTC], counter[3]);
        for (i = 0; i < NUMWINDOWS; i++) {
            wrefresh(cwin[i]);
        }   
        // pipe construction
        if (realchar != '\0') {
// #ifndef DEBUG
            //write in the pipe
            if ((write(pipefd[1],&realchar,sizeof(char))) < 0) {
                perror("error writing on input");
                return 3;
            }
// #endif
            if (realchar == 'Q') {
                clear();
                printw("Closing the program\n");
                refresh();
                sleep(3);
                break;
            }
        } 
    }
//#ifndef DEBUG
    // closing all the pipes
    close (pipefd[1]);
//#endif
    for (i = 0; i < NUMWINDOWS; i++) {
        destroy_win(cwin[i]);
    }   
    endwin();
    return 0;
}


void counterImplementation (int *cnt, char ct, int lung)
{
    // printw("Char: %c\n", ct);
    if (ct == 'w' || ct == 'e'|| ct == 'r')
    {
        if (cnt[1] < MAX )
        {
            cnt[1]++;
            if (cnt[3] > 0)
            {
                cnt[3]--;
            }
        }
        
    }
    if (ct == 'v' || ct == 'f'|| ct == 'r')
    {
        if (cnt[2] < MAX )
        {
            cnt[2]++;
            if (cnt[0] > 0)
            {
                cnt[0]--;
            }
        }
        
    }
    if (ct == 'v' || ct == 'c'|| ct == 'x')
    {
        if (cnt[3] < MAX )
        {
            cnt[3]++;
            if (cnt[1] > 0)
            {
                cnt[1]--;
            }
        }
        
    }
    if (ct == 'w' || ct == 's'|| ct == 'x')
    {
        if (cnt[0] < MAX )
        {
            cnt[0]++;
            if (cnt[2] > 0)
            {
                cnt[2]--;
            }
        }
        
    }
    if (ct == 'd' || ct == 'b')
    {
        for (int i = 0; i < lung; i++)
        {
            cnt[i] = 0;
        }
    }
}