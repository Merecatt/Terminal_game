/* File containing IPC message queues functionality */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include "shm_utils.h"
#include "sem_utils.h"

#define IPC_FLAGS (IPC_CREAT | 0640)
#define MSG_SIZE 12


typedef struct {
    /* Message struct */
    int winner; //1 if you won, 0 if you lost
    int your_units; //number of your units
    int enemy_units; //number of your enemy's units
    int add_info; 
    /* additional information: 
    1 - initial message; 
    2 - end of the game;
    3 - print text field; */
    char text[20];
    char action;
    char unit_type;
    int unit_number;
}message;

struct mq_buf {
    /* Redefined msgbuf stuct 
    Types:
    7 - game status (player info);
    1 - init message;
    3 - battle/other info; */
    long mtype; 
    message msg;
};


typedef struct {
    int id;
    player *addr;
    int semaphore;
}shm;

typedef struct {
    int id;
    int *addr;
    int semaphore;
}shm_int;


void display_message(message *msg){
    printf("----- Message -----\nwinner: %d\n", msg->winner);
    printf("your units: %d\nenemy units: %d\n", msg->your_units, msg->enemy_units);
    printf("additional information: %d\n", msg->add_info);
    printf("text: %s\naction: %c\n" , msg->text, msg->action);
    printf("unit type: %c\nunit number: %d\n", msg->unit_type, msg->unit_number);
}


int mq_create(){
    /* Wrapper function for creating message queue. 
    Returns identifier of queue or -1 in case of error. */
    int qid;
    if ((qid = msgget(IPC_PRIVATE, IPC_FLAGS)) == -1){
        perror("mq - creating message queue");
    }
    return qid;
}


int mq_open(key_t key){
    /* Wrapper function for opening (or creating) message queue. 
    Returns identifier of queue or -1 in case of error. */
    int qid;
    if ((qid = msgget(key, IPC_FLAGS)) == -1){
        perror("mq - opening/creating message queue");
    }
    return qid;
}


int mq_send(int qid, message *my_msg, long type){
    /* Wrapper function for sending message to queue. 
    Returns 0 on success and -1 in case of error. */
    int result, length;
    length = sizeof(message);
    struct mq_buf tmp;
    struct mq_buf *buf = &tmp;
    buf->mtype = type;
    buf->msg = *my_msg;

    if ((result = msgsnd(qid, buf, length, 0)) == -1){
        perror("mq - sending message");
    }
    return result;
}


int mq_receive(int qid, message *buf, long type){ 
    /* Wrapper function for receiving message from  queue.
    Returns the size of received message on success 
    and -1 in case of error. */
    int result;
    struct mq_buf tmp;
    struct mq_buf *buf_ptr = &tmp;
    if ((result = msgrcv(qid, buf_ptr, sizeof(struct mq_buf), type, 0)) == -1){
        if (errno != ENOMSG){
            perror("mq - receiving message");
        }
    }
    *buf = buf_ptr->msg;
    return result;
}


int mq_receive2(int qid, message *buf, long type, int flags){ 
    /* Wrapper function for receiving message from  queue.
    Has additional parameter flags.
    Returns the size of received message on success 
    and -1 in case of error. */
    int result;
    struct mq_buf tmp;
    struct mq_buf *buf_ptr = &tmp;
    if ((result = msgrcv(qid, buf_ptr, sizeof(struct mq_buf), type, flags)) == -1){
        if (errno != ENOMSG){
            perror("mq - receiving message");
        }
    }
    *buf = buf_ptr->msg;
    return result;
}


int mq_remove(int qid){
    /* Wrapper function for removing message queue from system. */
    int result;
    if ((result = msgctl(qid, IPC_RMID, 0)) == -1){
        perror("mq - removing message queue from system");
    }
    return result;
}


void mq_init(int n, int *pid, int mq, shm_int all_ready){
    /* Function initializing connection with client processes.
    Creates new process to deal with one player and waits for a message from this player.
    Returns pid of the new process through *pid pointer.
    @n - process number: {0, 1, 2}
    @pid - new pid variable
    @mq - message queue id  
    @all_ready - shm segment address with variable all_ready and its semaphore */
    
    if ((*pid = fork()) == -1){
        perror("init - fork function");
        exit(-1);
    }
    if (*pid == 0){ // child process
        /* initialize message queue connection */
        message msg;
        mq_receive(mq, &msg, 1); // wait for a message from player
        printf("Connection with player %d established.\n", n);
        display_message(&msg);

        /* send info through shm that player is ready */
        sem_p(all_ready.semaphore);
        (*(all_ready.addr))++; // CRITICAL SECTION
        sem_v(all_ready.semaphore);
    }
}
