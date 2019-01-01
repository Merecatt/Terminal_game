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

shm_int end_game;

void initialize_curses(int *lines, int *cols){
    initscr(); 
    cbreak(); // disable line buffering, take one char at a time
    noecho(); // don't echo the input
    clear(); // clear the screen
    *lines = LINES - 1;
    *cols = COLS - 1;
}

void display_info_curses (WINDOW *win, int lines, int cols, player *play){
    mvwprintw(win, 0, (cols-20)/2, "----- Player %d -----", play->n);
    mvwprintw(win, 1, (cols-12)/2, "Victories: %d", play->victories);
    mvwprintw(win, 2, 1, "Resources: %d", play->resources);
    mvwprintw(win, 3, 1, "Resources increase speed: %d", play->resources_increase);
    mvwprintw(win, 4, 1, "Workers: %d", play->workers);
    mvwprintw(win, 2, cols/2, "Light infantry: %d", play->military[0]);
    mvwprintw(win, 3, cols/2, "Heavy infantry: %d", play->military[1]);
    mvwprintw(win, 4, cols/2, "Cavalry: %d", play->military[2]);
    wrefresh(win);  
}

void display_unit_info (WINDOW *win, int lines, int cols){
    mvwprintw(win, 6, (cols - 21)/2, "----- Unit info -----");
    mvwprintw(win, 7, 19, "Price");
    mvwprintw(win, 7, 27, "Attack");
    mvwprintw(win, 7, 38, "Defense");
    mvwprintw(win, 7, 48, "Production time");
    mvwprintw(win, 8, 1, "Light infantry:");
    mvwprintw(win, 9, 1, "Heavy infantry:");
    mvwprintw(win, 10, 1, "Cavalry:");
    mvwprintw(win, 11, 1, "Workers:");
    mvwprintw(win, 8, 19, "100");
    mvwprintw(win, 9, 19, "250");
    mvwprintw(win, 10, 19, "550");
    mvwprintw(win, 11, 19, "150");
    mvwprintw(win, 8, 27, "1");
    mvwprintw(win, 9, 27, "1.5");
    mvwprintw(win, 10, 27, "3.5");
    mvwprintw(win, 11, 27, "0");
    mvwprintw(win, 8, 38, "1.2");
    mvwprintw(win, 9, 38, "3");
    mvwprintw(win, 10, 38, "1.2");
    mvwprintw(win, 11, 38, "0");
    mvwprintw(win, 8, 48, "2s");
    mvwprintw(win, 9, 48, "3s");
    mvwprintw(win, 10, 48, "5s");
    mvwprintw(win, 11, 48, "2s");
    mvwprintw(win, 13, (cols - 25)/2, "----- Communication -----");
    wrefresh(win);
}

void display_communication (WINDOW *win, int lines, int cols){
    wclear(win);
    mvwprintw(win, 0, 1, "Possible actions:");
    mvwprintw(win, 1, 1, "t - train units");
    mvwprintw(win, 2, 1, "a - attack");
    mvwprintw(win, 3, 1, "q - quit the game");
    wrefresh(win);
}

void display_units_number (WINDOW *win, int lines, int cols, message *msg){
    wclear(win);
    char unit_name[20];
    switch (msg->unit_type) {
        case 0:
            strcpy(unit_name, "light infantry");
            break;
        case 1:
            strcpy(unit_name, "heavy infantry");
            break;
        case 2:
            strcpy(unit_name, "cavalry");
            break;
        case 3:
            strcpy(unit_name, "workers");
            break;
        default: 
            strcpy(unit_name, "something incorrect");
    }
    mvwprintw(win, 0, 1, "You chose to train %s", unit_name);
    mvwprintw(win, 1, 1, "How many of them would you like to train?");
    wrefresh(win);
    int temp;
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0) {};
    temp = temp - '0';
    mvwprintw(win, 1, 43, "%d", temp);
    wrefresh(win);
    for (int i = 0; i < 4; i++){
        if (i == msg->unit_type) msg->unit_number[i] = temp;
        else msg->unit_number[i] = 0;
    }
}

void train(WINDOW *win, int lines, int cols, message *msg){
    int temp;
    wclear(win);
    mvwprintw(win, 0, 1, "Choose the type of units to train:");
    mvwprintw(win, 1, 1, "0 - light infantry");
    mvwprintw(win, 2, 1, "1 - heavy infantry");
    mvwprintw(win, 3, 1, "2 - cavalry");
    mvwprintw(win, 4, 1, "3 - workers");
    wrefresh(win);
    
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0){};
    msg->unit_type = temp - '0';
    display_units_number(win, lines, cols, msg);
}

void display_attack (WINDOW *win, int lines, int cols, int answer){ 
    wclear(win);
    mvwprintw(win, 0, 1, "Tremble and despair, player %d! Doom has come to your world!", answer);
    wrefresh(win);
}


void attack(WINDOW *win, int lines, int cols, message *msg){
    wclear(win);
    msg->action = 'a';
    int temp;
    mvwprintw(win, 0, 1, "Choose the number of units to send:");
    mvwprintw(win, 1, 1, "Light infantry: ");
    wrefresh(win);
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0){}; // wait for input
    msg->unit_number[0] = temp - '0';
    mvwprintw(win, 1, 17, "%d", msg->unit_number[0]);
    mvwprintw(win, 2, 1, "Heavy infantry: ");
    wrefresh(win);
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0){};
    msg->unit_number[1] = temp - '0';
    mvwprintw(win, 2, 17, "%d", msg->unit_number[1]);
    mvwprintw(win, 3, 1, "Cavalry: ");
    wrefresh(win);
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0){}; 
    msg->unit_number[2] = temp - '0';
    mvwprintw(win, 3, 10, "%d", msg->unit_number[2]);
    mvwprintw(win, 4, 1, "Who would you like to attack? Type the proper number.");
    wrefresh(win);
    while ((temp = wgetch(win)) == ERR && *(end_game.addr) == 0) {};
    msg->add_info = temp - '0';
    display_attack(win, lines, cols, msg->add_info);
}



void got_q (WINDOW *win, int lines, int cols){
    wclear(win);
    mvwprintw(win, 0, 1, "You chose to quit the game.");
    mvwprintw(win, 1, 1, "Okay, have a nice day, chicken.");
    wrefresh(win);
    sem_p(end_game.semaphore);
    *(end_game.addr) = 1;
    sem_v(end_game.semaphore);
}

void main_menu(WINDOW *win, char answer, int lines, int cols, message *msg, int qid){
    display_communication(win, lines, cols);
    wrefresh(win);
    switch (answer){
        case 'a':
            // display dialogue, get number of units and player to attack number
            attack(win, lines, cols, msg);
            mq_send(qid, msg, 3);
            // it has to be sent to the server and check if everything's okay
            break;
        case 't':
            msg->action = 't';
            train(win, lines, cols, msg);
            mq_send(qid, msg, 3);
            // get info if it can be done
            break;
        case 'q':
            got_q(win, lines, cols);
            // end_game = 1;
            break;
        default: 
            mvwprintw(win, 0, 1, "Something need doing?");
            wrefresh(win);
    }
    wrefresh(win);
    sleep(1);
    display_communication(win, lines, cols);
    wrefresh(win);
        
    
}

void clear_message(message *msg){
    msg->winner = -1;
    msg->your_units = -1;
    msg->enemy_units = -1;
    msg->add_info = -1;
    strcpy(msg->text, "");
    msg->action = -1;
    msg->unit_type = -1;
    for (int i = 0; i < 4; i++){
        msg->unit_number[i] = 0;
    }
}

void display_server_message (WINDOW *win, message *msg){
    wclear(win);
    mvwprintw(win, 0, 0, msg->text);
    wrefresh(win);
    clear_message(msg);
}

int main(int argv, char **args)
{   
    
    end_game.id = shm_create3(sizeof(int), 1945);
    end_game.addr = shm_attach(end_game.id);

    /* curses */
    int lines, cols;    

    key_t key = 2137 + atoi(args[1]);
    int mq = mq_open(key);
    message msg = {-1, -1, -1, 1, "", ' '};
    mq_send(mq, &msg, 1);
    
    /* open queue to send input to server */
    key_t key2 = 2140 + atoi(args[1]);
    int mq_input = mq_open(key2);

    player me; // structure with current information
    mq_receive_status(mq, &me);
    player temp;
    message msg_temp;

    

    initialize_curses(&lines, &cols);
    WINDOW *window1, *window2;
    WINDOW *window3;
    window1 = newwin(14, cols, 0, 0);
    window2 = newwin((lines-16), cols, 14, 0);
    window3 = newwin(2, cols, lines-1, 0);
    

    display_unit_info(window1, lines, cols);
    display_info_curses(window1, lines, cols, &me);
    display_communication(window2, lines, cols);
    wrefresh(window1);
    wrefresh(window2);
    wrefresh(window3);
    
    timeout(0); // set non-blocking read
    char input;
    //int input_i;
    message input_msg;


    while (msg.add_info != 2){ //there's no 'end game' signal
        //display_unit_info(lines, cols);
        if ((mq_receive2(mq, &msg_temp, 3, IPC_NOWAIT)) != -1){ // check for general messages (type 3)
            msg = msg_temp;
            if (msg.add_info == 3){
                display_server_message(window3, &msg);
            }
        } 
        if ((mq_receive_status2(mq, &temp, IPC_NOWAIT)) != -1){ // check for player struct messages (type 7)
            me = temp; // don't touch 'me', if there are no messages
            display_info_curses(window1, lines, cols, &me);
        }
        input = getch();    
        if (input != ERR){ // if a char is caught
            mvwprintw(window3, 0, 0, "call main menu");
            main_menu(window2, input, lines-16, cols, &input_msg, mq_input);
            
        }
        display_info_curses(window1, lines, cols, &me);
        display_unit_info(window1, lines, cols);
        clear_message(&input_msg);
        
        
        //wrefresh(window3);
        
    }
    shm_detach(end_game.addr);
    timeout(-1); // read is again a blocking function
    delwin(window1);
    delwin(window2);
    delwin(window3);
    /* end of the game */
    clear();
    mvprintw(lines/2-1, (cols-13)/2, "Game is over.");
    mvprintw(lines/2, (cols-sizeof(msg.text))/2, msg.text);


    mvaddstr(lines, 0, "Press any key to quit");
    refresh();
    getch(); // wait for the user to press any key
    endwin();
    exit(0);
    return 0;
}

  