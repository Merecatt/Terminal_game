/* File containing IPC message queues functionality */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define IPC_FLAGS (IPC_CREAT | 0640)
#define MSG_SIZE 12


typedef struct {
    /* Message struct */
    char text[MSG_SIZE];
    int unit_type;
    int number;
    int action;
}message;

struct mq_buf {
    /* Redefined msgbuf stuct */
    long mtype;
    message msg;
};


void display_message(message *msg){
    printf("----- Message -----\ntext: %s\n", msg->text);
    printf("unit type: %d\nnumber: %d\n", msg->unit_type, msg->number);
    printf("action: %d\n", msg->action);
}


int mq_create(){
    /* Wrapper for creating message queue function. 
    Returns key value of queue or -1 in case of error. */
    int qid;
    if ((qid = msgget(IPC_PRIVATE, IPC_FLAGS)) == -1){
        perror("mq - creating message queue");
    }
    return qid;
}


int mq_open(key_t key){
    /* Wrapper for opening (or creating) message queue function. 
    Returns key value of queue or -1 in case of error. */
    int qid;
    if ((qid = msgget(key, IPC_FLAGS)) == -1){
        perror("mq - opening/creating message queue");
    }
    return qid;
}


int mq_send(int qid, message *my_msg, long type){
    /* Wrapper for sending message to queue function. 
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
    /* Wrapper for receiving message from  queue function.
    Returns the size of received message on success 
    and -1 in case of error. */
    int result;
    struct mq_buf tmp;
    struct mq_buf *buf_ptr = &tmp;
    if ((result = msgrcv(qid, buf_ptr, sizeof(struct mq_buf), type, 0)) == -1){
        perror("mq - receiving message");
    }
    *buf = buf_ptr->msg;
    return result;
}


int mq_remove(int qid){
    /* Wrapper for removing message queue from system. */
    int result;
    if ((result = msgctl(qid, IPC_RMID, 0)) == -1){
        perror("mq - removing message queue from system");
    }
    return result;
}
