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
#include "mq_utils.h"
#include "shm_utils.h"
#include "sem_utils.h"

typedef struct {
    int price[4];
    double attack[4];
    double defense[4];
    int production_time[4];
}units_stats;

units_stats G_units_stats = {{100, 250, 550, 150},
                             {1, 1.5, 3.5, 0}, 
                             {1.2, 3, 1.2, 0},
                             {2, 3, 5, 2}};


void update_resources(int mq, player *play, int semaphore){
    printf("Updating resources.\n");
    sleep(1);
    sem_p(semaphore);
    play->resources += play->resources_increase; // CRITICAL SECTION
    mq_send_status(mq, play);
    sem_v(semaphore);
}

void remove_trash(shm players[], shm_int all_ready, shm_int end_game, int mq0, int mq[], int mq_input[]){
        /* Function removing created IPCs from the system */
        /* players' shared memory and semaphores */
        printf("Removing players' structures.\n");
        for (int i = 0; i < 3; i++){
            shm_detach(players[i].addr);
            shm_remove(players[i].id);
            sem_remove(players[i].semaphore);
        }

        /* all_ready and end_game shared memory and semaphores */
        printf("Removing all_ready.\n");
        shm_detach(all_ready.addr);
        shm_remove(all_ready.id);
        sem_remove(all_ready.semaphore);
        printf("Removing end_game.\n");
        shm_detach(end_game.addr);
        shm_remove(end_game.id);
        sem_remove(end_game.semaphore);

        /* message queues */
        printf("Removing message queues.\n");
        mq_remove(mq0);
        for (int i = 0; i < 3; i++){
            mq_remove(mq[i]);
            mq_remove(mq_input[i]);
        }
    }

int winner(shm players[]){
    /* Function returning id of the winner (in {0, 1, 2} notation). 
    If there is no winner, returns -1. */
    for (int i = 0; i < 3; i++){
        player *ptr = players[i].addr; // pointer to player's structure
        if ((*ptr).victories == 5){
            return i;
        }
    }
    return -1;
}

void train_units(int units[], shm play, int mq_output){
    // make a queue for training processes
    int cost = 0;
    for (int i = 0; i < 4; i++){
        cost += G_units_stats.price[i] * units[i];
    }
    printf("Cost: %d\n", cost);
    sem_p(play.semaphore);
    int p_resources = (*(play.addr)).resources;
    if (p_resources < cost){
        sem_v(play.semaphore);
        message msg;
        msg.add_info = 3; // tell client to print the message
        strcpy(msg.text, "Not enough resources.");
        mq_send(mq_output, &msg, 3);
    }
    else {
        sem_v(play.semaphore);
        // TODO - fork and train
        for (int i = 0; i < 4; i++){
            printf("Training %d units of %d type.\n", units[i], i);
            // fork
            // add and send info to player after every addition
            // exit 
            // ignore children's end in main input process not to leave zombies
        }
    }
}

void get_input(int mq, shm play, int mq_output){
    int status;
    message msg;
    status = mq_receive2(mq, &msg, 0, IPC_NOWAIT); // MUST BE NOWAIT so the process can end in case of end of the game
    if (status != -1){
        printf("msg.action: %c\n", msg.action);
        switch(msg.action) {
            case 't':
                printf("Get_input: got message 't'.\n");
                train_units(msg.unit_number, play, mq_output);
                break;
            default:
                printf("Player sent illegal message.\n");
        }
    }
}



int main()
{   
    /* initialize players' structures with semaphores and shared memory */
    shm players[3];
    
    for (int i = 0; i < 3; i++){
        players[i].semaphore = sem_create(2137+i);      // create semaphores
        sem_initialize(players[i].semaphore);           // initialize semaphores
        players[i].id = shm_create(sizeof(player));     // create memory segment for a player
        players[i].addr = shm_attach(players[i].id);    // attach memory segment
        shm_players_init(players[i].addr, i);           // initialize players' structures
    }

    
    
    int ppid; 
    ppid = getpid(); // main server process' pid


    /* initialize all_ready and its semaphore */
    shm_int all_ready;
    all_ready.id = shm_create(sizeof(int));
    all_ready.addr = shm_attach(all_ready.id);
    *(all_ready.addr) = 0;  // initial number of ready players; start the game when reaches 3
    all_ready.semaphore = sem_create2();
    sem_initialize(all_ready.semaphore);


    /* initialize end_game and its semaphore */
    shm_int end_game;
    end_game.id = shm_create(sizeof(int));
    end_game.addr = shm_attach(end_game.id);
    *(end_game.addr) = 0;  // when it reaches 1, it's time to finish
    end_game.semaphore = sem_create2();
    sem_initialize(end_game.semaphore);

    
    /* initialize message queue to communicate with child processes */
    int mq0;
    mq0 = mq_create();


    /* initialize message queues and processes for communication with players */
    int pid[3];          // server processes dedicated for players
    int mq[3];           // server->player message queues' id
    int pid_input[3];    // dedicated for getting players' input processes
    int mq_input[3];     // message queues for getting players' input
    for (int i = 0; i < 3; i++){
        mq[i] = mq_open(2137 + i);
        mq_input[i] = mq_open(2140+i);
    }

    /* connect with clients */
    mq_init(0, &pid[0], mq[0], all_ready); // get player 0
    if (getpid() == ppid){ // only the main process calls fork
        mq_init(1, &pid[1], mq[1], all_ready); // get player 1
    }
    if (getpid() == ppid){ // only the main process calls fork
        mq_init(2, &pid[2], mq[2], all_ready); // get player 2
    }

    /* fork to get processes dedicated for user's input */
    for (int i = 0; i < 3; i++){
        if (getpid() == ppid){
            if ((pid_input[i] = fork()) == -1) {
                perror("preparing to get input - fork");
            }
        }
    }

    /* start the game */
    if (getpid() == ppid){ // parent process
    while (*(all_ready.addr) != 3){ // wait to start the game
        sleep(1);
    }

    printf("\nLet's start the game!\n");
    
    /* send initial message to child processes to start the game */
    message init_message = {-1, -1, -1, 1, "", ' '};
    for (int i = 0; i < 6; i++) //send six messages for two processes of every player
        mq_send(mq0, &init_message, 1);
    } //every process should wait for its dedicated message!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    message start_game;

    /* server child processes should wait until they get message from parent */

    /* Update resources until the end of the game */
    for (int i = 0; i < 3; i++) {
        if (pid[i] == 0){
            mq_receive(mq0, &start_game, 1); // wait for the initial message
                if (start_game.add_info == 1){ // if you got the initial message
                    while (*(end_game.addr) != 1){
                        update_resources(mq[i], players[i].addr, players[i].semaphore);
                    }
                    int who_won = winner(players);
                    message msg_end_game;
                    msg_end_game.add_info = 2; // set info that the game is over
                    /* set text field so that is says who the winner is */
                    if (who_won == -1){
                        strcpy(msg_end_game.text, "Nobody won.");
                    }
                    else {
                        char str[20], temp[10];
                        strcpy(str, "Player ");
                        sprintf(temp, "%d", who_won);
                        strcat(str, temp);
                        strcat(str, " won.");
                        strcpy(msg_end_game.text, str);
                    }
                    mq_send(mq[i], &msg_end_game, 3);
                    printf("Output process %d finishes.\n", i);
                    exit(0);
                }
        }
    }

    /* Get user's input until the end of the game */
    for (int i = 0; i < 3; i++) {
        if (pid_input[i] == 0){
            // ignore childrens' end
            mq_receive(mq0, &start_game, 1); // wait for the initial message
            if (start_game.add_info == 1){ // if you got the initial message
                while (*(end_game.addr) != 1){
                    // TODO
                    get_input(mq_input[i], players[i], mq[i]);
                }
                printf("Input process %d finishes.\n", i);
                exit(0);
            }
        }
    }

    if (getpid() == ppid){ 
        /* 
        while (1){ // main server process sleeps until the end of the game
            sleep(1);
        }
        */
        sleep(20);
        /* delete all the trash from system */
        *(end_game.addr) = 1;
        printf("Waiting for children to end.\n");
        for (int i = 0; i < 6; i++){
            wait(NULL);
        }
        remove_trash(players, all_ready, end_game, mq0, mq, mq_input);
        printf("Trash removed successfully.\n");
    }
    

    

    //sleep(5);
    exit(0);
}
