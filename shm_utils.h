#ifndef SHM_UTILS_H
#define SHM_UTILS_H
#include "shm_utils.c"

void display_player(player *play);
int shm_create();
int shm_create2(key_t key);
void *shm_attach(int shmid);
int shm_remove(int shmid);
int shm_detach(void *addr);
int mq_send_status(int qid, player *my_msg);
int mq_receive_status(int qid, player *buf);

#endif
