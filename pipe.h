#ifndef PIPE_H
#define PIPE_H

#include <vector>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>

#define max_store_num 1500

struct pipe_info {
    bool valid;
    int *fd;
};

class pipe_box {
    public:
        pipe_box();
        ~pipe_box();
        int *get_fd(unsigned int instruction_number);
        int *get_fd(unsigned int instruction_number, int add);
        void set_fd(unsigned int instruction_number, int add, int *fd);
        void clean(unsigned int instruction_number);
        int *get_new_fd();
        void close_all();
        void close_all(unsigned int instruction_number, int add);
    private:
        std::vector<struct pipe_info> box;
};

#endif