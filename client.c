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


int main(int argv, char **args)
{   
    key_t key = 2137 + atoi(args[1]);
    int mq = mq_open(key);
    message msg = {-1, -1, -1, 1, "", ' '};
    mq_send(mq, &msg, 1);
    
    /* open queue to send input to server */
    key_t key2 = 2140 + atoi(args[1]);
    int input = mq_open(key2);

    player me; // structure with current information
    mq_receive_status(mq, &me);

    while (msg.add_info != 2){ //there's no 'end game' signal
        mq_receive2(mq, &msg, 3, IPC_NOWAIT); // check for general messages (type 3)
        mq_receive_status2(mq, &me, IPC_NOWAIT); // check for player struct messages (type 7)
        display_player(&me);
        sleep(4);
    }

    sleep(30); // delete it
    exit(0);
    return 0;
}

  