#include "shm.h"

int sem_create(key_t key, int initval)
{
    int id, semval;
    if(key == IPC_PRIVATE) return(-1); /* not intended for private semaphores */
    else if (key == (key_t)-1) return(-1); /* probably an ftok() error by caller */
    
again:
    if((id = semget(key, 3, 0666 | IPC_CREAT)) < 0) return(-1);
    if(semop(id, &op_lock[0], 2) < 0) {
        if (errno == EINVAL) goto again;
        
        std::cout << "can't lock" << std::endl;
        return -1;
    }

    if((semval = semctl(id, 1, GETVAL, 0)) < 0) {
        std::cout << "can't GETVAL" << std::endl;
        return -1;
    }
    if(semval == 0) {
        union semun semctl_arg;
        semctl_arg.val = initval;
        if (semctl(id, 0, SETVAL, semctl_arg) < 0) {
            std::cout << "can't SETVAL[0]" << std::endl;
            return -1;
        }
        semctl_arg.val = BIGCOUNT;
        if (semctl(id, 1, SETVAL, semctl_arg) < 0) {
            std::cout << "can't SETVAL[1]" << std::endl;
            return -1;
        }
    }
    if (semop(id, &op_endcreate[0], 2) < 0) {
        std::cout << "can't end create" << std::endl;
        return -1;
    }
    return(id);
}

void sem_rm(int id) {
    if(semctl(id, 0, IPC_RMID, 0) < 0) {
        //std::cout << "can't IPC_RMID" << std::endl;
        exit(0);
    }
}

int sem_open(key_t key) {
    int id;
    if(key == IPC_PRIVATE) return(-1);
    else if (key == (key_t) -1) return(-1);
    if((id = semget(key, 3, 0)) < 0) return(-1); /* doesn't exist, or tables full */

    // Decrement the process counter. We don't need a lock to do this.
    if(semop(id, &op_open[0], 1) < 0) {
        //std::cout << "can't open" << std::endl;
        return -1;
    }
    return(id);
}

void sem_close(int id) {
    int semval;
    // The following semop() first gets a lock on the semaphore,
    // then increments [1] - the process counter.
    if(semop(id, &op_close[0], 3) < 0) {
        //std::cout << "can't semop 3" << std::endl;
        exit(0);
    }

    // if this is the last reference to the semaphore, remove this.
    if((semval = semctl(id, 1, GETVAL, 0)) < 0) {
        //std::cout << "can't GETVAL" << std::endl;
        exit(0);
    }
    if(semval > BIGCOUNT) {
        //std::cout << "sem[1] > BIGCOUNT" << std::endl;
        exit(0);
    } else if(semval == BIGCOUNT) sem_rm(id);
    else if(semop(id, &op_unlock[0], 1) < 0) {
        //std::cout << "can't unlock" << std::endl;
        exit(0);
    } /* unlock */
}

int sem_op(int id, int value) {
    if((op_op[0].sem_op = value) == 0) {
        //std::cout << "can't have value == 0" << std::endl;
        return -1;
    }
    if(semop(id, &op_op[0], 1) < 0) {
        //std::cout << "sem_op error "<< value << std::endl;
        return -1;
    }
    return 0;
}

int sem_wait(int id) {
    if(semop(id, &op_wait[0], 1) < 0) {
        //std::cout << "sem_op error " << std::endl;
        return -1;
    }
    return 0;
}

int sem_signal(int id) {
    if(semop(id, &op_op[0], 1) < 0) {
        //std::cout << "sem_op error " << std::endl;
        return -1;
    }
    return 0;
}