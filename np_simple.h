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
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/resource.h>
#include "command.h"
#include "pipe.h"
#include "process.h"
#include "build-in_command.h"

#define max_fork_num 400
#define max_command_len 16000
#define max_user_num 30

class process process;
int fork_count = 0;

class np_simple {
    public:
    np_simple(){};
    np_simple(int sockfd, char const *path);
    void np_display();
    private:
    class command cmd;
    class pipe_box pipe_box;
    char command[max_command_len + 1];
};