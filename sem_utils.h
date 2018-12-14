#ifndef SEM_UTILS_H
#define SEM_UTILS_H
#include "sem_utils.c"

extern struct sembuf p;
extern struct sembuf v;

int sem_create (key_t key);
int sem_create2 ();
int sem_initialize (int semid);
int sem_remove (int semid);
int sem_p (int semid);
int sem_v (int semid);

#endif
