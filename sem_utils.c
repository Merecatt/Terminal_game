/* File containing semaphore functionality */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "sem_utils.h"

#define SEM_FLAGS (IPC_CREAT | 0640)

/* initialize semaphores */
struct sembuf p = {0, -1, 0};
struct sembuf v = {0, 1, 0};


int sem_create(key_t key){
    /* Wrapper function for creating a single semaphore.
    Returns identifier of semaphore or -1 in case of error. */
    int semid;
    if ((semid = semget(key, 1, SEM_FLAGS)) == -1){
        perror ("sem - creating a semaphore");
    }
    return semid;
}


int sem_create2() {
    /* Wrapper function for creating a single semaphore (without key).
    Returns identifier of semaphore or -1 in case of error. */
    int semid;
    if ((semid = semget(IPC_PRIVATE, 1, SEM_FLAGS)) == -1){
        perror ("sem - creating a semaphore");
    }
    return semid;
}


int sem_initialize (int semid) {
    /* Wrapper function for initializing a single semaphore with 1.
    Returns 0 on success or -1 in case of error. */
    int status;
    if ((status = semctl(semid, 0, SETVAL, 1)) == -1){
        perror ("sem - initializing a semaphore with 1");
    }
    return status;
}


int sem_remove(int semid) {
    /* Wrapper function for removing a single semaphore.
    Returns 0 on success or -1 in case of error. */
    int status;
    if ((status = semctl(semid, 0, IPC_RMID, NULL)) == -1) {
        perror ("sem - removing a semaphone");
    }
    return status;
}


int sem_p(int semid) {
    /* Wrapper function for winding a single semaphore down.
    Returns 0 on success or -1 in case of error. */
    int status;
    if ((status = semop(semid, &p, 1)) == -1) {
        perror ("sem - winding a semaphore down");
    }
    return status;
}


int sem_v(int semid) {
    /* Wrapper function for raising a single semaphore.
    Returns 0 on success or -1 in case of error. */
    int status;
    if ((status = semop(semid, &v, 1)) == -1) {
        perror ("sem - raising a semaphore");
    }
    return status;
}
