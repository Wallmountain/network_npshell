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

#define max_fork_num 400
#define max_command_len 16000
#define max_broadcast_len 60000
#define max_user_num 30

#define welcome_msg \
"****************************************\n\
** Welcome to the information server. **\n\
****************************************\n"

#define who_msg \
"<ID>\t<nickname>\t<IP:port>\t<indicate me>\n"

class process process;
int fork_count = 0;

struct user_pipe
{
    bool valid;
    int *fd;    
};

struct user {
    bool valid;
    int fd;
    std::string name;
    char *addr;
    int port;
    std::string path;
    class command cmd;
    class pipe_box pipe_box;
    std::vector<struct user_pipe> user_pipe_box;
};

class np_single {
    public:
    np_single();
    int np_display(int id);
    int insert_user(struct user user);
    struct user build_user(int fd, struct sockaddr_in addr);
    void remove_user(int id);
    int find_user_id(int fd);
    void restore_fd();
    void tell(int id, char *msg);
    void broadcast();
    void who(int id);
    void name(int id, char *user_name);
    void set_user_env(int id);
    char *get_addr(struct sockaddr_in pV4Addr);
    void set_broadcast_msg(int id, int mode);
    void set_user_name(int id, char *name);
    void close_all_number_pipe(int id, int instruction_count, int add);
    void close_related_user_pipe(int id);
    void close_all_user_pipe();
    bool check_if_user_alive(int id);
    bool check_if_user_pipe_alive(int s_id, int r_id);
    int *get_user_pipe(int s_id, int r_id);
    void set_user_pipe(int s_id, int r_id, int *fd);
    void revise_command();
    int yell_cmd(char *cmd);
    int tell_cmd(char *cmd);

    enum system_msg {
        enter   = 1,
        left    = 2,
        rename  = 3
    };

    private:
    void set_user_path(std::string path);
    std::vector<struct user> user_data;

    int user_number;
    int origin_fdin, origin_fdout, origin_fderr;
    char command[max_command_len + 1];
    char broadcast_msg[max_broadcast_len];
};