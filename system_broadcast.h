#ifndef SYSTEM_BROADCAST_H
#define SYSTEM_BROADCAST_H
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>

#define welcome_msg \
"****************************************\n\
** Welcome to the information server. **\n\
****************************************"

#define who_msg \
"<ID>\t<nickname>\t<IP:port>\t<indicate me>\n"

#define max_broadcast_len 50000
#define max_user_num 30

struct user_pipe
{
    bool valid;
    unsigned count;
};

struct user {
    bool valid;
    std::string name;
    std::string addr;
    std::vector<struct user_pipe> user_pipe_box;
};

class system_broadcast {
    public:
    system_broadcast() {};
    system_broadcast(char *ptr);
    void system_display();
    struct user build_user(std::string addr);
    int insert_user(struct user user);
    int find_user(std::string addr);
    bool dup_name(std::string name);
    private:
    std::vector<struct user> user_data;
    char *shm_ptr;
    unsigned int user_pipe_count;
};

#endif