#include "mq_utils.h"
#include "shm_utils.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h> //fifo
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <curses.h>

void initialize_curses(int *lines, int *cols){
    initscr(); 
    cbreak(); // disable line buffering, take one char at a time
    noecho(); // don't echo the input
    clear(); // clear the screen
    *lines = LINES - 1;
    *cols = COLS - 1;
}

void display_info_curses (int lines, int cols, player *play){
    refresh();
    mvprintw(0, (cols-20)/2, "----- Player %d -----", play->n);
    mvprintw(1, (cols-12)/2, "Victories: %d", play->victories);
    mvprintw(2, 1, "Resources: %d", play->resources);
    mvprintw(3, 1, "Resources increase speed: %d", play->resources_increase);
    mvprintw(4, 1, "Workers: %d", play->workers);
    mvprintw(2, cols/2, "Light infantry: %d", play->military[0]);
    mvprintw(3, cols/2, "Heavy infantry: %d", play->military[1]);
    mvprintw(4, cols/2, "Cavalry: %d", play->military[2]);
    refresh();  
}

void display_unit_info (int lines, int cols){
    mvprintw(6, (cols - 21)/2, "----- Unit info -----");
    mvprintw(7, 19, "Price");
    mvprintw(7, 27, "Attack");
    mvprintw(7, 38, "Defense");
    mvprintw(7, 48, "Production time");
    mvprintw(8, 1, "Light infantry:");
    mvprintw(9, 1, "Heavy infantry:");
    mvprintw(10, 1, "Cavalry:");
    mvprintw(11, 1, "Workers:");
    mvprintw(8, 19, "100");
    mvprintw(9, 19, "250");
    mvprintw(10, 19, "550");
    mvprintw(11, 19, "150");
    mvprintw(8, 27, "1");
    mvprintw(9, 27, "1.5");
    mvprintw(10, 27, "3.5");
    mvprintw(11, 27, "0");
    mvprintw(8, 38, "1.2");
    mvprintw(9, 38, "3");
    mvprintw(10, 38, "1.2");
    mvprintw(11, 38, "0");
    mvprintw(8, 48, "2s");
    mvprintw(9, 48, "3s");
    mvprintw(10, 48, "5s");
    mvprintw(11, 48, "2s");
}

int main(int argv, char **args)
{   
    /* curses */
    int lines, cols;    

    key_t key = 2137 + atoi(args[1]);
    int mq = mq_open(key);
    message msg = {-1, -1, -1, 1, "", ' '};
    mq_send(mq, &msg, 1);
    
    /* open queue to send input to server */
    key_t key2 = 2140 + atoi(args[1]);
    int input = mq_open(key2);

    player me; // structure with current information
    mq_receive_status(mq, &me);
    player temp;

    initialize_curses(&lines, &cols);
    display_unit_info(lines, cols);
    
    while (msg.add_info != 2){ //there's no 'end game' signal
        mq_receive2(mq, &msg, 3, IPC_NOWAIT); // check for general messages (type 3)
        if ((mq_receive_status2(mq, &temp, IPC_NOWAIT)) != -1){ // check for player struct messages (type 7)
            me = temp; // don't touch 'me', if there are no messages
            display_info_curses(lines, cols, &me);
        }    
    }



    /* end of the game */
    //TODO

    mvaddstr(lines, 0, "Press any key to quit");
    refresh();
    getch(); // wait for the user to press any key
    endwin();
    exit(0);
    return 0;
}

  