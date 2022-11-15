#include "pipe.h"

pipe_box::pipe_box() {
    for(int i = 0; i < max_store_num; ++i) {
        struct pipe_info temp;
        temp.valid = false;
        box.push_back(temp);
    } 
}

int *pipe_box::get_fd(unsigned int instruction_number) {
    int pipe_num = instruction_number % max_store_num;
    if(!box[pipe_num].valid)
        return NULL;
    return box[pipe_num].fd;
}

int *pipe_box::get_fd(unsigned int instruction_number, int add) {
    int pipe_num = (instruction_number % max_store_num + add) % max_store_num;
    if(!box[pipe_num].valid)
        return NULL;
    return box[pipe_num].fd;
}

void pipe_box::clean(unsigned int instruction_number) {
    box[instruction_number % max_store_num].valid = false;
}

int *pipe_box::get_new_fd() {
    int *fd = new int[2]; 
    if(pipe(fd) < 0) {
        delete [] fd;
        return NULL;
    }
    return fd;
}

void pipe_box::set_fd(unsigned int instruction_number, int add, int *fd) {
    int number = (instruction_number % max_store_num + add) % max_store_num;
    box[number].valid = true;
    box[number].fd = fd;
}

void pipe_box::close_all() {
    for(int i = 0; i < max_store_num; ++i) {
        if(box[i].valid) {
            close(box[i].fd[0]);
            close(box[i].fd[1]);
        }
    }
}

void pipe_box::close_all(unsigned int instruction_number, int add) {
    int number = (instruction_number % max_store_num + add) % max_store_num;
    for(int i = 0; i < max_store_num; ++i) {
        if(box[i].valid) {
            close(box[i].fd[0]);
            if(number != i) 
                close(box[i].fd[1]);
        }
    }
}

pipe_box::~pipe_box() {
    for(int i = 0; i < max_store_num; ++i) {
        if(box[i].valid) {
            close(box[i].fd[0]);
            close(box[i].fd[1]);
        }
    }
}