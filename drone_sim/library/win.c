#include "win.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void destroy_win(WINDOW *local_win) {
    werase(local_win); // Clear window content
    wrefresh(local_win); // Refresh to show changes
    delwin(local_win); // Delete the window
}

void print_btt_windows(WINDOW *win, char ch) {
    int maxX, maxY;
    getmaxyx(win, maxY, maxX);
    start_color();
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    wattron(win, COLOR_PAIR(1) | A_BOLD);

    mvwaddch(win,(int) ((maxY) / 2), (int) ((maxX - 1) / 2), ch);
    /*    
    if (ch == 'Q') {
        wattroff(*win, COLOR_PAIR(2) | A_BOLD);
    }
    else {
        wattroff(*win, COLOR_PAIR(1) | A_BOLD);
    }
    */
    wrefresh(win);
}

void boxCreation(WINDOW **win, int *maxY, int *maxX) {
    getmaxyx(*win, *maxY, *maxX);
    box(*win, 0, 0);
    wrefresh(*win);
}

void squareCreation(WINDOW **win, int height, int width, int *hg, int *wg)
 {
    int heightq = height / 5;
    int widthq = width / 5;
    start_color();

    init_pair (3, COLOR_RED, COLOR_BLACK);

    // Q subsquare
    win[BTTQ] = newwin(heightq, widthq, heightq, 0);
    box(win[BTTQ], 0, 0);
    mvwprintw(win[BTTQ], heightq / 2, widthq / 2, "Q");
    wattron(win[BTTQ], COLOR_PAIR(3) | A_BOLD);
    wrefresh(win[BTTQ]);
    win[BTTB] = newwin(heightq, widthq, (height - 2 * heightq), (width - widthq));
    box(win[BTTB], 0, 0);
    mvwprintw(win[BTTB], heightq / 2, widthq / 2, "B");
    wattron(win[BTTB], COLOR_PAIR(3) | A_BOLD);
    wrefresh(win[BTTB]);
    //sleep(1);
    // Subsquare
    int y, x;
    for (int i = 0; i < NUMMOTIONS; ++i) {
        y = (i / 3) * heightq + heightq;
        x = (i % 3) * widthq + widthq;
        win[i] = newwin(heightq, widthq, y, x);
        box(win[i], 0, 0);
        wrefresh(win[i]);
    }
    *hg = heightq;
    *wg = widthq;
}

void lightWindow(WINDOW *win, chtype attr) {
    start_color();
    wattron(win, attr); 
    wrefresh(win); 
    //sleep(2); 
    wattroff(win, attr); 
    wrefresh(win);
}

void printCounter(WINDOW *win, int num) {
    int h, w;
    getmaxyx(win, h, w);

    // Calcola le coordinate per centrare il carattere e il numero
    int y = h / 2;
    int x = (w - 2) / 2; // Sottrai 2 per far spazio al carattere e al numero

    // Stampa il carattere e il numero al centro della finestra
    mvwaddch(win, y, x, 'x');
    mvwprintw(win, y, x + 1, "%d", num);
    wrefresh(win);
}
