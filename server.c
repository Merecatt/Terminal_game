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

void mq_init(int n, int *pid, int *mq){
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
        mq_remove(*mq);
    }
}

int main()
{   
    int pid1, pid2, pid3, mq0, mq1, mq2, mq3;
    
    mq_init(0, &pid1, &mq1);
    if (pid1 != 0){ //the main process
        mq_init(1, &pid2, &mq2);
    }
    if (pid1 != 0 && pid2 != 0){ //the main process
        mq_init(2, &pid3, &mq3);
    }
    sleep(30);
    exit(0);
    
    //return 0;
}