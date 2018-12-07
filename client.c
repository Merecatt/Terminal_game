#include "mq_utils.h"
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
    printf("Okay, I'm player %s\n", args[1]);
    key_t key = 2137 + atoi(args[1]);
    int mq = mq_open(key);
    printf("The queue nr %s is open; its id is %d.\n", args[1], mq);
    message msg;
    strcpy(msg.text, "Battle!\0");
    mq_send(mq, &msg, 1);
    printf("Message sent.\n");
    
    sleep(30);
    exit(0);
    return 0;
}

  