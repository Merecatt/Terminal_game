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


void update_resources(int mq, player *play, int semaphore){
    printf("Updating resources.\n");
    sleep(1);
    sem_p(semaphore);
    play->resources += play->resources_increase; // CRITICAL SECTION
    mq_send_status(mq, play);
    sem_v(semaphore);
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
    
    /* fork every server's child to get processes dedicated for user's input */
    /* 
    for (int i = 0; i < 3; i++){
        if (pid[i] == 0){ // if it's proper child process
            if ((pid_input[i] = fork()) == -1) {
                perror("preparing to get input - fork");
            }
        }
    } */

    /* start the game */
    if (getpid() == ppid){ // parent process
    while (*(all_ready.addr) != 3){ // wait to start the game
        sleep(1);
    }


    /* remove shared memory segment which was only needed to connect all players. */
    shm_detach(all_ready.addr);
    shm_remove(all_ready.id);
    /* and the semaphore too */
    sem_remove(all_ready.semaphore);

    printf("\nLet's start the game!\n");
    
    /* send initial message to child processes to start the game */
    message init_message = {-1, -1, -1, 1, "", ' '};
    for (int i = 0; i < 3; i++) //send three messages for three players
        mq_send(mq0, &init_message, 1);
    }

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
                }
        }
    }

    if (getpid() == ppid){ 
        while (1){ // main server process sleeps until the end of the game
            sleep(1);
        }
        /* delete all the trash from system */
        //TODO
    }

    

    sleep(5);
    exit(0);
}
