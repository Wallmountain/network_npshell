#include "np_single_proc.h"

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

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << "wrong argc\n";
        exit(0);
    }
    int sockfd, fd, nfds;
    struct sockaddr_in serv_addr, cli_addr;
    fd_set rfds; /* read file descriptor set */
    fd_set afds; /* active file descriptor set */

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "server: can't open stream socket\n";
        exit(0);
    }
    
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

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(sockfd, &afds);

    class np_single npshell;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, child_handler);

    while (1) {
        memcpy(&rfds, &afds, sizeof(rfds));

        while(select(nfds, &rfds, (fd_set *)0, (fd_set *)0,
            (struct timeval *)0) < 0 && errno == EINTR) {
        }

        if (FD_ISSET(sockfd, &rfds)) {
            int ssock;
            int alen = sizeof(cli_addr);
            ssock = accept(sockfd, (struct sockaddr *)&cli_addr,(socklen_t*) &alen);
            if (ssock < 0) {
                std::cout << "accept:error\n";
                exit(0);
            }
            FD_SET(ssock, &afds);
            int id = npshell.insert_user(npshell.build_user(ssock, cli_addr));
            npshell.tell(id, welcome_msg);
            npshell.set_broadcast_msg(id, npshell.enter);
            npshell.broadcast();
            npshell.tell(id, "% ");
        }
        for (fd=0; fd < nfds; ++fd) {
            if (fd != sockfd && FD_ISSET(fd, &rfds)) {
                int id = npshell.find_user_id(fd);
                npshell.set_user_env(id);
                if (!npshell.np_display(id)) {
                    (void) close(fd);
                    FD_CLR(fd, &afds);
                    npshell.remove_user(id);
                    npshell.set_broadcast_msg(id, npshell.left);
                    npshell.broadcast();
                    npshell.close_related_user_pipe(id);
                }
                npshell.restore_fd();
            }
        }
    }
    return 0;
}

np_single::np_single() {
    origin_fdin = dup(0);
    origin_fdout = dup(1);
    origin_fderr = dup(2);
    user_data.resize(max_user_num + 1);
    user_number = 0;
}

struct user np_single::build_user(int fd, struct sockaddr_in addr) {
    struct user res;
    res.fd = fd;
    res.path = "bin:.";
    res.name = "(no name)";
    res.addr = get_addr(addr);
    res.port = ntohs(addr.sin_port);
    res.valid = true;
    res.user_pipe_box.resize(max_user_num + 1);
    return res;
}

int np_single::insert_user(struct user user) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(!user_data[i].valid) {
            ++user_number;
            user_data[i] = user;
            return i;
        }
    }
    return -1;
}

void np_single::set_user_path(std::string path) {
    int length = path.length();
    char temp[length + 1];
    strcpy(temp, path.c_str());
    setenv("PATH", temp, 1);
}

void np_single::restore_fd() {
    dup2(origin_fdin, 0);
    dup2(origin_fdout, 1);
    dup2(origin_fderr, 2);
}

int np_single::find_user_id(int fd) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].fd == fd)
            return i;
    }
    return -1;
}

void np_single::remove_user(int id) {
    user_data[id].valid = false;
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[id].user_pipe_box[i].valid) {
            user_data[id].user_pipe_box[i].valid = false;
            close(user_data[id].user_pipe_box[i].fd[0]);
            close(user_data[id].user_pipe_box[i].fd[1]);
        }
    }
    user_number--;
}

int np_single::np_display(int id) {
    if(std::cin.getline(command, max_command_len)) {
        char msg[max_broadcast_len];
        user_data[id].cmd.read_string(command, 1); // tokenize the command and check if there is any error
        revise_command();
        int *fd_in, *fd_out, *fd_err;
        fd_in = fd_out = fd_err = NULL;
        while(!user_data[id].cmd.isempty()) {
            unsigned int instruction_count = user_data[id].cmd.get_instruction_count();
            int number = -1;
            int filefd = 0;
            char **event = user_data[id].cmd.get_next_cmd(); // get instruction
            fd_in = fd_out; // last output will be the input of current instruction
            if(!*event) { //no instruction now
                fd_out = fd_err = NULL;
                continue;
            }

            if(!fd_in) { // if no input, check if there is number pipe for current instruction
                fd_in = user_data[id].pipe_box.get_fd(instruction_count);
                user_data[id].pipe_box.clean(instruction_count);
                if(fd_in)
                    close(fd_in[1]);
            }
            fd_out = fd_err = NULL;

            int build_in = check_buildin_command(event);
            switch(build_in) {
                case -1: {
                    return 0;
                }
                case 1: {
                    user_data[id].path = event[2];
                }
                    break; 
                case 3: {
                    who(id);
                }
                    break;
                case 4: { 
                    name(id, event[1]);
                }
                    break;
                case 5: {
                    bool err = false;
                    int len = strlen(event[1]);
                    for(int i = 0; i < len; ++i) {
                        if(event[1][i] < '0' || event[1][i] > '9')
                            err = true;
                    }
                    int tell_id = std::stoi(event[1]), tell_len;
                    if(err || !user_data[tell_id].valid) {
                        tell_len = sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", tell_id);
                        msg[tell_len] = '\0';
                        tell(id, msg);
                    } else {
                        int rm_msg = tell_cmd(command);
                        tell_len = sprintf(msg, "*** %s told you ***: %s\n", user_data[id].name.c_str(), command + rm_msg);
                        msg[tell_len] = '\0';
                        tell(tell_id, msg);
                    }
                }
                    break;
                case 6: {
                    int rm_msg = yell_cmd(command);
                    int b_len = sprintf(broadcast_msg, "*** %s yelled ***: %s\n", user_data[id].name.c_str(), command + rm_msg);
                    broadcast_msg[b_len] = '\0';
                    broadcast();
                }
                    break;
            }

            if(build_in) {
                event = user_data[id].cmd.get_next_cmd();
                while(*event)
                    event = user_data[id].cmd.get_next_cmd();
                continue;
            }

            char **in_operator = NULL, **out_operator = NULL;
            char **temp_operator = user_data[id].cmd.get_next_cmd();

            while(*temp_operator) {
                int len = strlen(*temp_operator);
                if((*temp_operator)[0] == '|') {
                    if(len == 1) {
                        fd_out = user_data[id].pipe_box.get_new_fd();
                    } else { // NUMBER PIPE
                        number = std::stoi(&(*temp_operator)[1]);

                        fd_out = user_data[id].pipe_box.get_fd(instruction_count, number); 
                        if(!fd_out) { // if no inherent number pipe
                            fd_out = user_data[id].pipe_box.get_new_fd();
                            user_data[id].pipe_box.set_fd(instruction_count, number, fd_out);
                        }
                    }
                    break;
                } else if((*temp_operator)[0] == '!') {
                    fd_out = user_data[id].pipe_box.get_new_fd();
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
                temp_operator = user_data[id].cmd.get_next_cmd();
            }
            int user_pipe_id;
            if(in_operator) {
                user_pipe_id = std::stoi(&(*in_operator)[1]);
                if(!check_if_user_alive(user_pipe_id)) {
                    int tell_len = sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", user_pipe_id);
                    msg[tell_len] = '\0';
                    tell(id, msg);
                    fd_in = user_data[id].pipe_box.get_new_fd();
                    fd_in[0] = open("/dev/null", O_RDONLY);
                } else if(!check_if_user_pipe_alive(user_pipe_id, id)) {
                    int tell_len = sprintf(msg, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", user_pipe_id, id);
                    msg[tell_len] = '\0';
                    tell(id, msg);
                    fd_in = user_data[id].pipe_box.get_new_fd();
                    fd_in[0] = open("/dev/null", O_RDONLY);
                } else {
                    int b_len = sprintf(broadcast_msg, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", 
                                        user_data[id].name.c_str(), id, user_data[user_pipe_id].name.c_str(), user_pipe_id, command);
                    broadcast_msg[b_len] = '\0';
                    broadcast();
                    fd_in = get_user_pipe(user_pipe_id, id);
                }
            }
            if(out_operator) {
                user_pipe_id = std::stoi(&(*out_operator)[1]);
                if(!check_if_user_alive(user_pipe_id)) {
                    int tell_len = sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", user_pipe_id);
                    msg[tell_len] = '\0';
                    tell(id, msg);
                    fd_out = user_data[id].pipe_box.get_new_fd();
                    fd_out[1] = open("/dev/null", O_WRONLY);
                } else if(check_if_user_pipe_alive(id, user_pipe_id)) {
                    int tell_len = sprintf(msg, "*** Error: the pipe #%d->#%d already exists. ***\n",id, user_pipe_id);
                    msg[tell_len] = '\0';
                    tell(id, msg);
                    fd_out = user_data[id].pipe_box.get_new_fd();
                    fd_out[1] = open("/dev/null", O_WRONLY);
                } else {
                    int b_len = sprintf(broadcast_msg, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n",
                                        user_data[id].name.c_str(), id, command, user_data[user_pipe_id].name.c_str(), user_pipe_id);
                    broadcast_msg[b_len] = '\0';
                    broadcast();
                    fd_out = user_data[id].pipe_box.get_new_fd();
                    set_user_pipe(id, user_pipe_id, fd_out);
                }
            }

            int childpid;
            while(fork_count > max_fork_num) { // limit of the fork number
                usleep(100000);
            }
            if((childpid = fork()) == -1) {
                exit(0);
            }
            ++fork_count;

            if(childpid > 0) {
                /* parent process */
                if(number == -1 && fd_out) {
                    close(fd_out[1]);
                }
                if(filefd > 0) {
                    close(filefd);
                }
                
                process.insert_process(childpid, fd_in, fd_out);

                if(!fd_out) { // no fd_out represent that the end of the instruction 
                    int status;
                    waitpid(childpid, &status, 0);
                    --fork_count;
                    usleep(10000);
                    process.close_process(childpid, status);
                }
            } else if(!childpid) {
                /* child process  */
                
                // close all pipe in PIPE
                close_all_number_pipe(id, instruction_count, number);
                close_all_user_pipe();

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
        usleep(10000);
        std::cout << "% " << std::flush;
    }
    return 1;
}

void np_single::tell(int id, char *msg) {
    write(user_data[id].fd, msg, strlen(msg));
}

void np_single::broadcast() {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) 
            tell(i, broadcast_msg);
    }
}

void np_single::set_user_env(int id) {
    set_user_path(user_data[id].path);
    dup2(user_data[id].fd, 0);
    dup2(user_data[id].fd, 1);
    dup2(user_data[id].fd, 2);
}

char *np_single::get_addr(struct sockaddr_in pV4Addr) {
    struct in_addr ipAddr = pV4Addr.sin_addr;
    char *str = new char[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
    return str;
}

void np_single::set_broadcast_msg(int id, int mode) {
    int len;
    switch(mode) {
        case 1:
            len = sprintf(broadcast_msg, "*** User '%s' entered from %s:%d. ***\n", 
                          user_data[id].name.c_str(), user_data[id].addr, user_data[id].port);
            break;
        case 2:
            len = sprintf(broadcast_msg, "*** User '%s' left. ***\n", user_data[id].name.c_str());
            break;
        case 3:
            len = sprintf(broadcast_msg, "*** User from %s:%d is named '%s'. ***\n",
                          user_data[id].addr, user_data[id].port, user_data[id].name.c_str());
            break;
    }
    broadcast_msg[len] = '\0';
}

void np_single::who(int id) {
    tell(id, who_msg);
    char msg[10000];
    int len = 0;
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) {
            len += sprintf(msg + len, "%d\t%s\t%s:%d",
                           i, user_data[i].name.c_str(), user_data[i].addr, user_data[i].port);
            if(id == i) {
                len += sprintf(msg + len, "\t<-me\n");
            } else
                len += sprintf(msg + len, "\n");
        }
    }
    tell(id, msg);
}

void np_single::name(int id, char *user_name) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(!strcmp(user_data[i].name.c_str(), user_name)) {
            char msg[1000];
            int len = sprintf(msg, "*** User '%s' already exists. ***\n", user_name);
            msg[len] = '\0';
            tell(id, msg);
            return;
        }
    }
    user_data[id].name = user_name;
    set_broadcast_msg(id, rename);
    broadcast();
}

void np_single::close_all_number_pipe(int id, int instruction_count, int add) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) {
            if(i == id && add != -1) 
                    user_data[i].pipe_box.close_all(instruction_count, add);
                else   
                    user_data[i].pipe_box.close_all();
        }
    }
}

void np_single::close_related_user_pipe(int id) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid && user_data[i].user_pipe_box[id].valid) {
            close(user_data[i].user_pipe_box[id].fd[0]);
            user_data[i].user_pipe_box[id].valid = false;
        }
    }
}

bool np_single::check_if_user_alive(int id) {
    return id < max_user_num + 1 &&
           user_data[id].valid;
}

bool np_single::check_if_user_pipe_alive(int s_id, int r_id) {
    return s_id < max_user_num + 1   &&
           r_id < max_user_num + 1   &&
           user_data[s_id].valid     &&
           user_data[r_id].valid     &&
           user_data[s_id].user_pipe_box[r_id].valid;
}

int * np_single::get_user_pipe(int s_id, int r_id) {
    user_data[s_id].user_pipe_box[r_id].valid = false;
    return user_data[s_id].user_pipe_box[r_id].fd;
}

void np_single::set_user_pipe(int s_id, int r_id, int *fd) {
    user_data[s_id].user_pipe_box[r_id].valid = true;
    user_data[s_id].user_pipe_box[r_id].fd = fd;
}

void np_single::close_all_user_pipe() {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) {
            for(int j = 1; j < max_user_num + 1; ++j) {
                if(user_data[i].user_pipe_box[j].valid) {
                    close(user_data[i].user_pipe_box[j].fd[0]);
                }
            }
        }
    }
}

void np_single::revise_command() {
    int length = strlen(command) - 1;
    while(length >= 0              &&
          (command[length] == '\r' ||
           command[length] == '\n'))
        command[length--] = '\0';
}

int np_single::yell_cmd(char *cmd) {
    int res = 0;
    while(cmd[res] == ' ') ++res;
    return res + 5;
}

int np_single::tell_cmd(char *cmd) {
    int res = 0;
    while(cmd[res] == ' ') ++res;
    res += 4;
    while(cmd[res] == ' ') ++res;
    while(cmd[res] <= '9' && cmd[res] >= '0') ++res;
    return res + 1;
}