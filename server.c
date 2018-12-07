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


void mq_init(int n, int *pid, int *mq, int all_ready){
    /* Function initializing connection with client processes.
    Creates new process and message queue to deal with one player.
    Returns pid of the new process.
    @n - process number: {0, 1, 2}
    @pid - new pid variable
    @mq - message queue id  */
    
    if ((*pid = fork()) == -1){
        perror("init - fork function");
        exit(-1); //am I sure it's right?
    }
    if (*pid == 0){
        *mq = mq_open(2137 + n);
        message msg;
        mq_receive(*mq, &msg, 1);
        printf("Connection with player %d established.\n", n);
        display_message(&msg);
        int *my_all_ready = shm_attach(all_ready);
        (*my_all_ready)++;
        shm_detach(my_all_ready);
        sleep(1); //delete it
        mq_remove(*mq); //do it later
    }
}


int main()
{   
    /* pid1, pid2, pid3 - child processes pid's */
    int ppid, pid1, pid2, pid3, mq1, mq2, mq3;
    ppid = getpid();
    /* *parent_all_ready == 3 means all players are connected
    end_game == 1 means there is a winner and it's time to finish */
    int shm_all_ready = shm_create();
    int shm_end_game = shm_create();
    int *parent_all_ready = shm_attach(shm_all_ready);
    *parent_all_ready = 0;

    mq_init(0, &pid1, &mq1, shm_all_ready);
    sleep(1);
    if (pid1 != 0){ //the main process
        mq_init(1, &pid2, &mq2, shm_all_ready);
    }
    sleep(1);
    if (pid1 != 0 && pid2 != 0){ //the main process
        mq_init(2, &pid3, &mq3, shm_all_ready);
    }

    if (getpid() == ppid){ //I guess I'm in parent process
    while (*parent_all_ready != 3){
        sleep(1);
    }
    /* Removing shared memory segment which was only needed to connect all players. */
    shm_detach(parent_all_ready);
    shm_remove(shm_all_ready);

    printf("\nLet's start the game!\n");
    }
    sleep(5);
    exit(0);
    
    //return 0;
}