#ifndef SHM_H
#define SHM_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <iostream>

#define SHMKEY ((key_t) 169566) /* base value for shmem key */
#define SEMKEY1 ((key_t) 131149) /* client semaphore key */
#define SEMKEY2 ((key_t) 231349) /* server semaphore key */
#define SEMKEY3 ((key_t) 331449) /* client semaphore key */
#define SEMKEY4 ((key_t) 431549) /* server semaphore key */
#define SEMKEY5 ((key_t) 531649) /* client semaphore key */
#define SEMKEY6 ((key_t) 631749) /* server semaphore key */
#define BIGCOUNT 200
#define PERMS 0666

static struct sembuf op_endcreate[2] = {
    1, -1, SEM_UNDO,/* decrement [1] (proc counter) with undo on exit */
    /* UNDO to adjust proc counter if process exits
    before explicitly calling sem_close() */
    2, -1, SEM_UNDO /* decrement [2] (lock) back to 0 -> unlock */
};

static struct sembuf op_close[3] = {
    2, 0, 0, /* wait for [2] (lock) to equal 0 */
    2, 1, SEM_UNDO,/* then increment [2] to 1 - this locks it */
    1, 1, SEM_UNDO /* then increment [1] (proc counter) */
};

static struct sembuf op_open[1] = {
    1, -1, SEM_UNDO /* decrement [1] (proc counter) with undo on exit */
};

static struct sembuf op_lock[2] = {
    2, 0, 0, /* wait for sem#0 to become 0 */
    2, 1, SEM_UNDO /* then increment sem#0 by 1 */
};

static struct sembuf op_unlock[1] = {
    2, -1, SEM_UNDO /* decrement sem#0 by 1 (sets it to 0) */
};

static struct sembuf op_op[1] = {
    0, 1, SEM_UNDO
};

static struct sembuf op_wait[1] = {
    0, -1, SEM_UNDO
};

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};

int sem_create(key_t key, int initval);
void sem_rm(int id);
int sem_open(key_t key);
void sem_close(int id);
int sem_op(int id, int value);
int sem_wait(int id);
int sem_signal(int id);
#endif