/* File containing shared memory functionality */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define SHM_FLAGS (IPC_CREAT | 0640)


typedef struct {
    /* Player information struct. */
    int n;
    int military[3]; 
    /* 0 - light_infantry;
    1 - heavy_infantry;
    2 - cavalry;        */
    int workers;
    int resources_increase; //resources increase speed per 1 second
    int resources;
    int victories;
}player;

struct mq_player_buf {
    /* Redefined msgbuf stuct */
    long mtype;
    player play;
};


void display_player(player *play){
    printf("\n----- Player %d -----\n", play->n);
    printf("Light infantry: %d\nHeavy infantry: %d\n", play->military[0], play->military[1]);
    printf("Cavalry: %d\nWorkers: %d\nResources: %d\n", play->military[2], play->workers, play->resources);
    printf("Resource increase speed: %d\nNumber of victories: %d\n", play->resources_increase, play->victories);
}


int shm_create(size_t size){
    /* Wrapper function for creating shared memory segment (without key). 
    Returns identifier of shared memory segment or -1 in case of error. */
    int shmid;
    if ((shmid = shmget(IPC_PRIVATE, size, SHM_FLAGS)) == -1){
        perror("shm - creating shared memory segment");
    }
    return shmid;
}


int shm_create2(key_t key){
    /* Wrapper function for creating shared memory segment. 
    Returns  identifier of shared memory segment or -1 in case of error. */
    int shm;
    if ((shm = shmget(key, sizeof(player), SHM_FLAGS)) == -1){
        perror("shm - creating shared memory segment (with key)");
    }
    return shm;
}


void *shm_attach(int shmid){
    /* Wrapper function for attaching shared memory segment. 
    Returns segment address for the process. */
    void *addr;
    if ((addr = shmat(shmid, 0, 0)) == (void*)-1){
        perror("shm - attaching shared memory segment");
    }
    return addr;
}


int shm_remove(int shmid){
    /* Wrapper function for marking shared memory segment for removal. */
    int status;
    if ((status = shmctl(shmid, IPC_RMID, 0)) == -1){
        perror("shm - removing shared memory segment");
    }
    return status;
}


int shm_detach(void *addr){
    /* Wrapper function for detaching shared memory segment. */
    int status;
    if ((status = shmdt(addr)) == -1){
        perror("shm - detaching shared memory segment");
    }
    return status;
}


int mq_send_status(int qid, player *my_msg){
    /* Wrapper function for sending player info to queue. 
    Returns 0 on success and -1 in case of error. 
    Player info (game status) has message type 7. */
    int result, length;
    length = sizeof(player);
    struct mq_player_buf tmp;
    struct mq_player_buf *buf = &tmp;
    buf->mtype = 7;
    buf->play = *my_msg;

    if ((result = msgsnd(qid, buf, length, 0)) == -1){
        perror("mq - sending player info");
    }
    return result;
}


int mq_receive_status(int qid, player *buf){ 
    /* Wrapper function for receiving player info from  queue.
    Returns the size of received message on success 
    and -1 in case of error. 
    Player info (game status) has message type 7. */
    int result;
    struct mq_player_buf tmp;
    struct mq_player_buf *buf_ptr = &tmp;
    if ((result = msgrcv(qid, buf_ptr, sizeof(struct mq_player_buf), 7, 0)) == -1){
        perror("mq - receiving player info");
    }
    *buf = buf_ptr->play;
    return result;
}
