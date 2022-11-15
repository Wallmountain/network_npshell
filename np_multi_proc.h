#include <fstream>
#include <ostream>
#include <cstring>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <cstddef>
#include <netdb.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/resource.h>
#include "command.h"
#include "pipe.h"
#include "process.h"
#include "build-in_command.h"
#include "shm.h"
#include "system_broadcast.h"

#define max_fork_num 400
#define max_command_len 16000

class process process;
int fork_count = 0;
char *shm_ptr;
int clisem, clienting, serving, servsem, call_system, call_system_end;
int shmid;
int system_pid;

class np_multi {
    public:
    np_multi() {};
    np_multi(int sockfd, char const *path);
    int display();
    void revise_command();
    int yell_cmd();
    int tell_cmd();
    int *FIFO_fd(char *path, int mode);
    char *get_addr(struct sockaddr_in pV4Addr);
    void set_addr(char* addr_in);
    
    private:
    class command cmd;
    class pipe_box pipe_box;
    char command[max_command_len + 1];
    std::string addr;
};