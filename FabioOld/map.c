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
#include "library/window.h"


#define DEBUG 1
typedef struct Position {
    int x;
    int y;
} position;


int main (int argc, char* argv[])
{
    int pipe[argc];
    int i;
    WINDOW* ring;
    int hight, width;
    int raws, cols;
    int offsetx, offsety;
    position p;
// #ifndef DEBUG

    initscr();
    raw();
    noecho();
    start_color();

    for (i = 0; i < argc; i++) {
        pipe[i] = atoi(argv[i]); // converts from the string to the integer
    }
    close(pipe[1]);
    // I only want data in reading
    getmaxyx(stdscr, hight, width);
    refresh();
    raws = 0.8 * hight;
    cols = 0.8 * width;
    offsetx = raws / 2; // always summing this positions 
    offsety = cols / 2; // before printing

    p.x = 0;
    p.y = 0;
    // window creation
    ring = newwin(hight, width, (hight - raws), (width - cols)); //#raw and col, positions

    // main loop for functioning
    while (p.x != -1000 && p.y != -1000){
        // read from the pipe
        if(read(pipe[0], &p, sizeof(position)) < 0) {
            perror("read positions");
            exit(EXIT_FAILURE);
        }
        // print the drone
        mvwprintw(ring, (int) (p.y + offsety - 1), (int) (p.x + offsetx), "o o");
        mvwprintw(ring, (int) (p.y + offsety), (int) (p.x + offsetx), " O");
        mvwprintw(ring, (int) (p.y + offsety + 1), (int) (p.x + offsetx), "o o");
        wrefresh(ring);
    } 

    // From now I have to read all the data of the structure of poition  
    //while (x != -1 && y != -1) {
    // computa posizione avendo posizione assoluta e trasformandola in relativa
    // getmax(win princ
    // getmax(win map)
    // pos = x/maxprinc *maxmap
    // trasla tutto di mezza finestra a dx e in basso
    // passo da server posizione e dimensione della mappa
    // fscanf(stdin, "%d", &ch);
    //devo capire come muovere il drone sulla mappa
    // }
// #endif

    close(pipe[0]);
    return 0;
}