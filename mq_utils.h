#ifndef MQ_UTILS_H
#define MQ_UTILS_H
#include "mq_utils.c"

void display_message(message *msg);
int mq_create();
int mq_open(key_t key);
int mq_send(int qid, message *my_msg, long type);
int mq_receive(int qid, message *buf, long type);
int mq_receive2(int qid, message *buf, long type, int flags);
int mq_remove(int qid);
void mq_init(int n, int *pid, int *mq, int all_ready, int sem);

#endif
