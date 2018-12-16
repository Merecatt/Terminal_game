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
    /* initialize semaphores for players' structures */
    int semaphores[3];
    
    for (int i = 0; i < 3; i++){
        semaphores[i] = sem_create(2137+i); //semget(SEM_KEY + i, 1, IPC_CREAT|0640);
        sem_initialize(semaphores[i]);
    }

    
    int pid[3], mq[3]; // pid[3] - child processes pid's; // mq[3] - player-server message queues' id
    int ppid, mq0; 
    ppid = getpid(); // main server process' pid


    /* *parent_all_ready == 3 means all players are connected */
    int shm_all_ready = shm_create(sizeof(int)); 
    int *parent_all_ready = shm_attach(shm_all_ready);
    *parent_all_ready = 0; // initial number of ready players
    /* all_ready - semaphore */
    int sem_all_ready;
    sem_all_ready = sem_create2();
    sem_initialize(sem_all_ready);


    /* end_game == 1 means there is a winner and it's time to finish */
    int shm_end_game = shm_create(sizeof(int));
    int *end_game = shm_attach(shm_end_game);
    *end_game = 0; 
    /* end_game - semaphore */
    int sem_end_game;
    sem_end_game = sem_create2();
    sem_initialize(sem_end_game);
    
    /* initialize message queue to communicate with child processes */
    mq0 = mq_create();

    /* initialize players' shared memory */
    int shm_players[3];
    player *players_array[3];

    for (int i = 0; i < 3; i++){
        shm_players[i] = shm_create(sizeof(player));
        players_array[i] = shm_attach(shm_players[i]);
    }
    shm_players_init(players_array);

    /* initialize message queues and processes for players' input */
    int mq_input[3];
    int pid_input[3];
    for (int i = 0; i < 3; i++){
        mq_input[i] = mq_open(2140+i);
    }

    /* connect with clients */
    mq_init(0, &pid[0], &mq[0], shm_all_ready, sem_all_ready); // get player 0
    if (pid[0] != 0){ // the main process
        mq_init(1, &pid[1], &mq[1], shm_all_ready, sem_all_ready); // get player 1
    }
    if (pid[0] != 0 && pid[1] != 0){ // the main process
        mq_init(2, &pid[2], &mq[2], shm_all_ready, sem_all_ready); // get player 2
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
    while (*parent_all_ready != 3){ // wait to start the game
        sleep(1);
    }


    /* remove shared memory segment which was only needed to connect all players. */
    shm_detach(parent_all_ready);
    shm_remove(shm_all_ready);
    /* and the semaphores too */
    sem_remove(sem_all_ready);

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
                    while (*end_game != 1){
                        update_resources(mq[i], players_array[i], semaphores[i]);
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
