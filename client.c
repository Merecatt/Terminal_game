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


int main(int argv, char **args)
{   
    key_t key = 2137 + atoi(args[1]);
    int mq = mq_open(key);
    message msg;
    msg.add_info = 1;
    msg.enemy_units = -1;
    msg.winner = -1;
    msg.your_units = -1;
    mq_send(mq, &msg, 1);
    
    sleep(30);
    exit(0);
    return 0;
}

  