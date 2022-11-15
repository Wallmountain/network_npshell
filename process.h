#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <unistd.h>
#include <string>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define finish 1

struct process_info {
    int *fd_in, *fd_out;
    int id;
    int status;
    int valid;
    std::string FIFO_file;
};

class process {
    public :
        process(){};
        void insert_process(int id, int *fd_in, int *fd_out);
        void insert_process(int id, int *fd_in, int *fd_out, char *FIFO_in);
        void close_process(int id, int status);
        void close_all_process();

    private :
        std::vector<struct process_info> processes;
};
#endif
