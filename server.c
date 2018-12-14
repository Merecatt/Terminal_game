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


void mq_init(int n, int *pid, int *mq, int all_ready, int sem){
    /* Function initializing connection with client processes.
    Creates new process and message queue to deal with one player.
    Returns pid of the new process.
    @n - process number: {0, 1, 2}
    @pid - new pid variable
    @mq - message queue id  
    @all_ready - shm segment address with variable all_ready
    @sem - semaphore for all_ready */
    
    if ((*pid = fork()) == -1){
        perror("init - fork function");
        exit(-1); // am I sure it's right?
    }
    if (*pid == 0){ // child process
        /* initialize message queue and connection */
        *mq = mq_open(2137 + n);
        message msg;
        mq_receive(*mq, &msg, 1); // wait for a message from player
        printf("Connection with player %d established.\n", n);
        display_message(&msg);

        /* send info through shm that player is ready */
        int *my_all_ready = shm_attach(all_ready);
        sem_p(sem);
        (*my_all_ready)++; // CRITICAL SECTION
        sem_v(sem);
        shm_detach(my_all_ready);

        /* temporary instructions not to make a mess */
        mq_remove(*mq); // do it later
    }
}


void shm_players_init (player *array[]){
    for (int i = 0; i < 3; i++){
        array[i]->n = i+1;
        array[i]->military[0] = 0;
        array[i]->military[1] = 0;
        array[i]->military[2] = 0;
        array[i]->workers = 0;
        array[i]->resources_increase = 50;
        array[i]->resources = 300;
        array[i]->victories = 0;
    }
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

    /* connect with clients */
    mq_init(0, &pid[0], &mq[0], shm_all_ready, sem_all_ready); // get player 0
    sleep(1);
    if (pid[0] != 0){ // the main process
        mq_init(1, &pid[1], &mq[1], shm_all_ready, sem_all_ready); // get player 1
    }
    sleep(1);
    if (pid[0] != 0 && pid[1] != 0){ // the main process
        mq_init(2, &pid[2], &mq[2], shm_all_ready, sem_all_ready); // get player 2
    }

    if (getpid() == ppid){ // parent process
    while (*parent_all_ready != 3){
        sleep(1);
    }

    /* remove shared memory segment which was only needed to connect all players. */
    shm_detach(parent_all_ready);
    shm_remove(shm_all_ready);
    /* and the semaphores too */
    sem_remove(sem_all_ready);

    printf("\nLet's start the game!\n");
    
    /* send initial message to child processes to start the game */
    message init_message = {-1, -1, -1, 1};
    for (int i = 0; i < 3; i++) //send three messages for three players
        mq_send(mq0, &init_message, 1);
    }

    message start_game;

    /* server child processes should wait until they get message from parent */
    if (pid[0] == 0){
        mq_receive(mq0, &start_game, 1);
        if (start_game.add_info == 1){
            // serve player 1
        }
    }

    if (pid[1] == 0){
        mq_receive(mq0, &start_game, 1);
        if (start_game.add_info == 1){
            // serve player 2
        }
    }

    if (pid[2] == 0){
        mq_receive(mq0, &start_game, 1);
        if (start_game.add_info == 1){
            //serve player 3
        }
    }

    /* delete all the trash from system */
    //TODO

    sleep(5);
    exit(0);
    
    // return 0;
}