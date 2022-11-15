#include "np_multi_proc.h"

/* deal with the end of child process */
void child_handler(int sig) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        --fork_count;
        process.close_process(pid, status);
    }
    return;
}

void kill_all(int sig) {
    sem_close(clisem);
    sem_close(clienting);
    sem_close(servsem);   
    sem_close(call_system_end);
    sem_close(call_system);
    sem_close(serving);
    shmdt(shm_ptr);
    exit(0);
}

void kill_handler(int sig) {
    sem_close(clisem);
    sem_close(clienting);
    sem_close(serving);

    close(0);
    close(1);
    close(2);
    shmdt(shm_ptr);
    exit(0);

}

void INT_handler(int sig) {
    shmdt(shm_ptr);
    kill(system_pid, SIGKILL);
    waitpid(system_pid, 0, 0);
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) {
        //std::cout << "server: can't remove shared memory" << std::endl;
    }
    
    sem_rm(clisem);
    sem_rm(clienting);
    sem_rm(servsem);
    sem_rm(call_system_end);
    sem_rm(call_system);
    sem_rm(serving);
    exit(0);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << "wrong argc\n";
        exit(0);
    }

    signal(SIGCHLD, SIG_IGN);

    if ((shmid=shmget(SHMKEY, max_broadcast_len, PERMS|IPC_CREAT)) < 0) {
        std::cout << "server: can't get shared memory" << std::endl;
        exit(0);
    }

    shm_ptr = (char *)shmat(shmid, (char *) 0, 0);
    if (!shm_ptr) {
        std::cout << "server: can't get shared memory ptr" << std::endl;
        exit(0);
    }

    if ((clisem = sem_create(SEMKEY1, 1)) < 0) {
        std::cout << "server: can't get sem client" << std::endl;
        exit(0);
    }

    if ((servsem = sem_create(SEMKEY2, 0)) < 0) {
        std::cout << "server: can't get sem server" << std::endl;
        exit(0);
    }

    if ((clienting = sem_create(SEMKEY3, 0)) < 0) {
        std::cout << "server: can't get sem server" << std::endl;
        exit(0);
    }

    if ((serving = sem_create(SEMKEY4, 0)) < 0) {
        std::cout << "server: can't get sem server" << std::endl;
        exit(0);
    }

    if ((call_system = sem_create(SEMKEY5, 1)) < 0) {
        std::cout << "server: can't get sem server" << std::endl;
        exit(0);
    }

    if ((call_system_end = sem_create(SEMKEY6, 0)) < 0) {
        std::cout << "server: can't get sem server" << std::endl;
        exit(0);
    }

    if ((system_pid = fork()) < 0) std::cout << "server: fork error\n";
    else if (system_pid == 0) { /* child process */ /* broadcast system */
        
        signal(SIGKILL, kill_all);

        class system_broadcast sys_b(shm_ptr);
        while(1) {
            sem_wait(servsem);
            while(semctl(call_system_end, 0, GETVAL, 0)) sem_wait(call_system_end);
            sys_b.system_display();
            //while(semctl(clisem, 0, GETVAL, 0)) sem_wait(clisem);
            sem_wait(clisem);

            int semval;
            if((semval = semctl(clienting, 1, GETVAL, 0)) < 0) {
                //std::cout << "can't GETVAL" << std::endl;
                exit(0);
            } else if(semval == BIGCOUNT - 1) {
            } else {
                sem_signal(clienting);
                sem_wait(serving);
                sem_wait(clienting);
            }

            sem_signal(clisem);

            while(semctl(clisem, 1, GETVAL, 0) != semctl(clienting, 1, GETVAL, 0))
                usleep(1000);

            sem_signal(call_system_end);
        }
        exit(0);
    }

    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "server: can't open stream socket\n";
        exit(0);
    }

    signal(SIGINT, INT_handler);

    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        std::cout << "setsockopt(SO_REUSEADDR) failed\n";
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(std::stoi(argv[1]));
    if (bind(sockfd, (struct sockaddr *) & serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "server: can't bind local address\n";
        exit(0);
    }
    listen(sockfd, max_user_num);

    for ( ; ; ) {
        int newsockfd, childpid;
        int clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
        if (newsockfd < 0) std::cout << "server: accept error\n";
        if ((childpid = fork()) < 0) std::cout << "server: fork error\n";
        else if (childpid == 0) { /* child process */
            /* close original socket */
            close(sockfd);
            class np_multi npshell(newsockfd, "bin:.");
            int f_len = 0;
            char addr[1024];
            f_len = sprintf(addr, "%s:%u", npshell.get_addr(cli_addr), ntohs(cli_addr.sin_port));
            addr[f_len] = '\0';
            npshell.set_addr(addr);
            
            int broadcast_reiceiver_pid;

            if ((broadcast_reiceiver_pid = fork()) < 0) std::cout << "server: fork error\n";
            else if (broadcast_reiceiver_pid == 0) { /* child process */

                signal(SIGKILL, kill_handler);

                if ((clisem = sem_open(SEMKEY1)) < 0) {
                    std::cout << "server: can't get sem client" << std::endl;
                    exit(0);
                }

                if ((serving = sem_open(SEMKEY4)) < 0) {
                    std::cout << "server: can't get sem server" << std::endl;
                    exit(0);
                }

                //int start;
                //if ((start = sem_open(SEMKEY7)) < 0) {
                //    std::cout << "server: can't get sem server" << std::endl;
                //    exit(0);
                //}

                //sem_signal(start);

                //sem_close(start);

                while(1) {
                    sem_wait(clisem);
                    if ((clienting = sem_open(SEMKEY3)) < 0) {
                        std::cout << "server: can't get sem server" << std::endl;
                        exit(0);
                    }
                    sem_signal(clisem);
                    sem_wait(clienting);

                    std::string str(shm_ptr);
                    std::string cmd, id;
                    std::stringstream read_str(str);
                    
                    read_str >> cmd >> id;
                    int idx = cmd.length() + id.length() + 2;

                    if(!cmd.compare("all") || !cmd.compare(addr)) {
                        write(1, shm_ptr + idx, strlen(shm_ptr + idx));
                    }

                    int semval;
                    if(semop(clienting, &op_close[0], 3) < 0) {
                        std::cout << "can't semop" << std::endl;
                        exit(0);
                    }

                    // if this is the last reference to the semaphore, remove this.
                    if((semval = semctl(clienting, 1, GETVAL, 0)) < 0) {
                        std::cout << "can't GETVAL" << std::endl;
                        exit(0);
                    }
                    if(semval > BIGCOUNT) {
                        std::cout << "sem[1] > BIGCOUNT" << std::endl;
                        exit(0);
                    } else if(semval == BIGCOUNT - 1) {
                        sem_signal(serving);
                    }
                    if(semop(clienting, &op_unlock[0], 1) < 0) {
                        std::cout << "can't unlock" << std::endl;
                        exit(0);
                    }
                    sem_signal(clienting);
                    clienting = 0;
                }
                exit(0);
            }

            if ((servsem = sem_open(SEMKEY2)) < 0) {
                std::cout << "server: can't get sem server" << std::endl;
                exit(0);
            }

            if ((call_system = sem_open(SEMKEY5)) < 0) {
                std::cout << "server: can't get sem server" << std::endl;
                exit(0);
            }

            if ((call_system_end = sem_open(SEMKEY6)) < 0) {
                std::cout << "server: can't get sem server" << std::endl;
                exit(0);
            }

            while(semctl(call_system_end, 0, GETVAL, 0)) sem_wait(call_system_end);
            
            //int start;
            //if ((start = sem_create(SEMKEY7, 0)) < 0) {
            //    std::cout << "server: can't get sem client" << std::endl;
            //    exit(0);
            //}
            //sem_wait(start);
            //sem_close(start);
            
            
            usleep(1000);
            npshell.display();

            if(kill(broadcast_reiceiver_pid, SIGKILL) < 0) {
                std::cout << "don't send signal" << std::endl;
            }

            int status;
            waitpid(broadcast_reiceiver_pid, &status, 0);
            signal(SIGCHLD, SIG_IGN);

            sem_wait(call_system);
            f_len = sprintf(shm_ptr, "%s left", addr);
            shm_ptr[f_len] = '\0';
            sem_signal(servsem);
            sem_wait(call_system_end);
            sem_signal(call_system);

            sem_close(servsem);
            sem_close(call_system);
            sem_close(call_system_end);
            close(newsockfd);
            shmdt(shm_ptr);
            exit(0);
        }
        close(newsockfd); /* parent process */
    }
    return 0;
}

np_multi::np_multi(int sockfd, char const *path) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, child_handler);
    dup2(sockfd, 0);
    dup2(sockfd, 1);
    dup2(sockfd, 2);
    close(sockfd);
    setenv("PATH", path, 1);
}

int np_multi::display() {
    std::cout << welcome_msg << std::endl;
    sem_wait(call_system);
    int f_len = 0;
    f_len = sprintf(shm_ptr, "%s enter", addr.c_str());
    shm_ptr[f_len] = '\0';
    sem_signal(servsem);
    sem_wait(call_system_end);
    sem_signal(call_system);
    std::cout << "% ";
    while(std::cin.getline(command, max_command_len)) {
        while(semctl(call_system_end, 0, GETVAL, 0)) sem_wait(call_system_end);
        cmd.read_string(command, 1); // tokenize the command and check if there is any error
        revise_command();
        int *fd_in, *fd_out, *fd_err;
        fd_in = fd_out = fd_err = NULL;
        while(!cmd.isempty()) {
            unsigned int instruction_count = cmd.get_instruction_count();
            char FIFO_FILE[100], FIFO_OUT[100];
            FIFO_FILE[0] = '\0';
            FIFO_OUT[0] = '\0';
            int number = -1;
            int filefd = 0;
            char **event = cmd.get_next_cmd(); // get instruction
            fd_in = fd_out; // last output will be the input of current instruction
            if(!*event) { //no instruction now
                fd_out = fd_err = NULL;
                continue;
            }

            if(!fd_in) { // if no input, check if there is number pipe for current instruction
                fd_in = pipe_box.get_fd(instruction_count);
                pipe_box.clean(instruction_count);
                if(fd_in)
                    close(fd_in[1]);
            }
            fd_out = fd_err = NULL;

            int build_in = check_buildin_command(event);
            switch(build_in) {
                case -1: {
                    process.close_all_process();
                    return 0;
                }
                case 3: {
                    sem_wait(call_system);
                    int len = 0;
                    len = sprintf(shm_ptr, "%s who", addr.c_str());
                    shm_ptr[len] = '\0';
                    sem_signal(servsem);
                    sem_wait(call_system_end);
                    sem_signal(call_system);
                }
                    break;
                case 4: { 
                    sem_wait(call_system);
                    int len = 0;
                    len = sprintf(shm_ptr, "%s name %s",
                                  addr.c_str(), event[1]);
                    shm_ptr[len] = '\0';
                    sem_signal(servsem);
                    sem_wait(call_system_end);
                    sem_signal(call_system);
                }
                    break;
                case 5: {
                    sem_wait(call_system);
                    int len = 0;
                    len = sprintf(shm_ptr, "%s tell %s %s",
                                  addr.c_str(), event[1], command + tell_cmd());
                    shm_ptr[len] = '\0';
                    sem_signal(servsem);
                    sem_wait(call_system_end);
                    sem_signal(call_system);
                }
                    break;
                case 6: {
                    sem_wait(call_system);
                    int len = 0;
                    len = sprintf(shm_ptr, "%s yell %s", addr.c_str(), command + yell_cmd());
                    shm_ptr[len] = '\0';
                    sem_signal(servsem);
                    sem_wait(call_system_end);
                    sem_signal(call_system);
                }
                    break;
            }
            if(build_in) {
                event = cmd.get_next_cmd();
                while(*event)
                    event = cmd.get_next_cmd();
                continue;
            }

            char **in_operator = NULL, **out_operator = NULL;
            char **temp_operator = cmd.get_next_cmd();

            while(*temp_operator) {
                int len = strlen(*temp_operator);
                if((*temp_operator)[0] == '|') {
                    if(len == 1) {
                        fd_out = pipe_box.get_new_fd();
                    } else { // NUMBER PIPE
                        number = std::stoi(&(*temp_operator)[1]);

                        fd_out = pipe_box.get_fd(instruction_count, number); 
                        if(!fd_out) { // if no inherent number pipe
                            fd_out = pipe_box.get_new_fd();
                            pipe_box.set_fd(instruction_count, number, fd_out);
                        }
                    }
                    break;
                } else if((*temp_operator)[0] == '!') {
                    fd_out = pipe_box.get_new_fd();
                    fd_err = fd_out;
                    break;
                } else if((*temp_operator)[0] == '>' && len == 1) {
                    filefd = open(temp_operator[1], O_WRONLY | O_CREAT| O_TRUNC, 0644);
                } else {
                    if((*temp_operator)[0] == '>') {
                        out_operator = temp_operator;
                    } else {
                        in_operator = temp_operator;
                    }
                }
                temp_operator = cmd.get_next_cmd();
            }
            int user_pipe_id, id;
            if(in_operator) {
                user_pipe_id = std::stoi(&(*in_operator)[1]);
                sem_wait(call_system);
                int len = 0;
                len = sprintf(shm_ptr, "%s pipe_from %d %s", addr.c_str(), user_pipe_id, command);
                shm_ptr[len] = '\0';
                sem_signal(servsem);
                sem_wait(call_system_end);
                sem_signal(call_system);

                std::string str(shm_ptr);
                std::string cmd;
                std::stringstream read_str(str);
                read_str >> cmd;
                read_str >> cmd;
                id = stoi(cmd);
                if(id >= 0) {
                    len = sprintf(FIFO_FILE, "user_pipe/%d.txt", id);
                    FIFO_FILE[len] = '\0';
                } else {
                    fd_in = pipe_box.get_new_fd();
                    fd_in[0] = open("/dev/null", O_RDONLY);
                }
            }
            if(out_operator) {
                user_pipe_id = std::stoi(&(*out_operator)[1]);
                sem_wait(call_system);
                int len = 0;
                len = sprintf(shm_ptr, "%s pipe_to %d %s", addr.c_str(), user_pipe_id, command);
                shm_ptr[len] = '\0';
                sem_signal(servsem);
                sem_wait(call_system_end);
                sem_signal(call_system);
                std::string str(shm_ptr);
                std::string cmd;
                std::stringstream read_str(str);
                read_str >> cmd;
                read_str >> cmd;
                id = stoi(cmd);
                if(id >= 0) {
                    len = sprintf(FIFO_OUT, "user_pipe/%d.txt", id);
                    FIFO_OUT[len] = '\0';
                } else {
                    fd_out = pipe_box.get_new_fd();
                    fd_out[1] = open("/dev/null", O_WRONLY);
                }
            }
            
            int exec_pid;
            while(fork_count > max_fork_num) { // limit of the fork number
                usleep(100000);
            }
            if((exec_pid = fork()) == -1) {
                exit(0);
            }
            ++fork_count;

            if(exec_pid > 0) {
                /* parent process */
                if(number == -1 && fd_out) {
                    close(fd_out[1]);
                }
                if(filefd > 0) {
                    close(filefd);
                }
                
                process.insert_process(exec_pid, fd_in, fd_out, FIFO_FILE);
                if(!strlen(FIFO_OUT) && !fd_out) { // no fd_out represent that the end of the instruction 
                    int status;
                    waitpid(exec_pid, &status, 0);
                    --fork_count;
                    usleep(5000);
                    process.close_process(exec_pid, status);
                }
            } else if(!exec_pid) {
                /* child process  */
                
                // close all pipe in NUMBER PIPE
                if(number != -1) 
                    pipe_box.close_all(instruction_count, number);
                else   
                    pipe_box.close_all();

                if(fd_in) {
                    dup2(fd_in[0], 0);
                }
                if(fd_out) {
                    dup2(fd_out[1], 1);
                    close(fd_out[0]);
                }
                if(fd_err) {
                    dup2(fd_err[1], 2);
                }
                if(filefd) {
                    dup2(filefd, 1);
                }
                if(strlen(FIFO_OUT)) {
                    unlink(FIFO_OUT);
                    mknod(FIFO_OUT, S_IFIFO | PERMS, 0);
                    fd_out = FIFO_fd(FIFO_OUT, 1);
                    dup2(fd_out[1], 1);
                }
                if(strlen(FIFO_FILE)) {
                    fd_in = FIFO_fd(FIFO_FILE, 0);
                    dup2(fd_in[0], 0);
                }

                if(execvp(event[0], event) == -1) {
                    std::cerr << "Unknown command: [" << event[0] << "]." << std::endl;
                }

                if(fd_out)
                    close(fd_out[1]);
                if(fd_in) {
                    char buffer[max_command_len];
                    while(read(fd_in[0], buffer, max_command_len));
                    close(fd_in[0]);
                }
                exit(0);
            }
        }
        usleep(5000);
        std::cout << "% " << std::flush;
    }
    return 1;
}

void np_multi::revise_command() {
    int length = strlen(command) - 1;
    while(length >= 0              &&
          (command[length] == '\r' ||
           command[length] == '\n'))
        command[length--] = '\0';
}

int np_multi::yell_cmd() {
    int res = 0;
    while(command[res] == ' ') ++res;
    return res + 5;
}

int np_multi::tell_cmd() {
    int res = 0;
    while(command[res] == ' ') ++res;
    res += 4;
    while(command[res] == ' ') ++res;
    while(command[res] <= '9' && command[res] >= '0') ++res;
    return res + 1;
}

int *np_multi::FIFO_fd(char *path, int mode) {
    int *fd = new int [2];
    if(mode == 1) {         
        if(!(fd[mode] = open(path, O_WRONLY))) {
            delete [] fd;
            return NULL;
        }
    } else { 
        if(!(fd[mode] = open(path, O_RDONLY))) {
            delete [] fd;
            return NULL;
        }
    }
    return fd;
}

char *np_multi::get_addr(struct sockaddr_in pV4Addr) {
    struct in_addr ipAddr = pV4Addr.sin_addr;
    char *str = new char[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
    return str;
}

void np_multi::set_addr(char *addr_in) {
    addr = addr_in;
}