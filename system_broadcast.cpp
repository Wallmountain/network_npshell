#include "system_broadcast.h"
#include <iostream>
system_broadcast::system_broadcast(char *ptr) {
    user_pipe_count = 1;
    user_data.resize(max_user_num + 1);
    for(int i = 1; i < max_user_num + 1; ++i) {
        user_data[i].user_pipe_box.resize(max_user_num + 1);
    }
    shm_ptr = ptr;
}

struct user system_broadcast::build_user(std::string addr) {
    struct user res;
    res.name = "(no name)";
    res.valid = true;
    res.addr = addr;
    res.user_pipe_box.resize(max_user_num + 1);
    return res;
}

int system_broadcast::insert_user(struct user user) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(!user_data[i].valid) {
            user_data[i] = user;
            return i;
        }
    }
    return -1;
}

void system_broadcast::system_display() {
    std::string str(shm_ptr);
    std::string addr, cmd;
    std::stringstream read_str(str);

    read_str >> addr;
    int id = find_user(addr);

    if(id < 0) {
        id = insert_user(build_user(addr));
    }

    read_str >> cmd;
    int len = 0;
    if(!cmd.compare("enter")) {
        len = sprintf(shm_ptr, "all %d *** User '%s' entered from %s. ***\n", 
                      id, user_data[id].name.c_str(), user_data[id].addr.c_str());
    } else if(!cmd.compare("left")) {
        len = sprintf(shm_ptr, "all %d *** User '%s' left. ***\n", id, user_data[id].name.c_str());
        for(int i = 1; i < max_user_num + 1; ++i) {
            user_data[id].user_pipe_box[i].valid = false;
            user_data[i].user_pipe_box[id].valid = false;
        }
        user_data[id].valid = false;
    } else if(!cmd.compare("name")) {
        std::string name;
        read_str >> name;
        if(dup_name(name)) {
            len = sprintf(shm_ptr, "%s 1 *** User '%s' already exists. ***\n",
                          user_data[id].addr.c_str(), name.c_str());
        } else {
            user_data[id].name = name;
            len = sprintf(shm_ptr, "all %d *** User from %s is named '%s'. ***\n",
                          id, user_data[id].addr.c_str(), user_data[id].name.c_str());
        }
    } else if(!cmd.compare("yell")) {
        int idx = addr.length() + 6;
        int cmd_len = strlen(shm_ptr) - idx;
        char temp[cmd_len];
        strcpy(temp, shm_ptr + idx);
        len = sprintf(shm_ptr, "all %d *** %s yelled ***: %s\n",
                      id, user_data[id].name.c_str(), temp);
    } else if(!cmd.compare("tell")) {
        std::string receiver;
        read_str >> receiver;
        int r_id = stoi(receiver);
        if(!user_data[r_id].valid) {
            len = sprintf(shm_ptr, "%s 1 *** Error: user #%d does not exist yet. ***\n",
                          user_data[id].addr.c_str(), r_id);
        } else {
            int idx = addr.length() + receiver.length() + 7;
            int cmd_len = strlen(shm_ptr) - idx;
            char temp[cmd_len];
            strcpy(temp, shm_ptr + idx);
            len = sprintf(shm_ptr, "%s 1 *** %s told you ***: %s\n",
                          user_data[r_id].addr.c_str(), user_data[id].name.c_str(), temp);
        } 
    } else if(!cmd.compare("who")) {
        len = sprintf(shm_ptr, "%s 1 ", user_data[id].addr.c_str());
        len += sprintf(shm_ptr + len, who_msg);
        for(int i = 1; i < max_user_num + 1; ++i) {
            if(user_data[i].valid) {
                len += sprintf(shm_ptr + len, "%d\t%s\t%s",
                               i, user_data[i].name.c_str(), user_data[i].addr.c_str());
                if(id == i) {
                    len += sprintf(shm_ptr + len, "\t<-me\n");
                } else
                    len += sprintf(shm_ptr + len, "\n");
            }
        }
    } else if(!cmd.compare("pipe_to")) {
        std::string receiver;
        read_str >> receiver;
        int r_id = std::stoi(receiver);
        if(r_id > max_user_num || !user_data[r_id].valid) {
            len = sprintf(shm_ptr, "%s -1 *** Error: user #%d does not exist yet. ***\n",
                          user_data[id].addr.c_str(), r_id);
        } else if(user_data[id].user_pipe_box[r_id].valid) {
            len = sprintf(shm_ptr, "%s -2 *** Error: the pipe #%d->#%d already exists. ***\n", 
                          user_data[id].addr.c_str(), id, r_id);
        } else {
            user_data[id].user_pipe_box[r_id].valid = true;
            user_data[id].user_pipe_box[r_id].count = ++user_pipe_count;
            int idx = addr.length() + receiver.length() + cmd.length() + 3;
            int cmd_len = strlen(shm_ptr) - idx + 1;
            char temp[cmd_len];
            strcpy(temp, shm_ptr + idx);
            temp[cmd_len - 1] = '\0';
            int r_id = stoi(receiver);
            len = sprintf(shm_ptr, "all %u *** %s (#%d) just piped '%s' to %s (#%d) ***\n",
                          user_data[id].user_pipe_box[r_id].count, user_data[id].name.c_str(), id, temp, user_data[r_id].name.c_str(), r_id);
        }
    } else if(!cmd.compare("pipe_from")) {
        std::string send;
        read_str >> send;
        int s_id = std::stoi(send);
        if(s_id > max_user_num || !user_data[s_id].valid) {
            len = sprintf(shm_ptr, "%s -1 *** Error: user #%d does not exist yet. ***\n",
                          user_data[id].addr.c_str(), s_id);
        } else if(!user_data[s_id].user_pipe_box[id].valid) {
            len = sprintf(shm_ptr, "%s -2 *** Error: the pipe #%d->#%d does not exist yet. ***\n", 
                      user_data[id].addr.c_str(), s_id, id);
        } else {
            user_data[s_id].user_pipe_box[id].valid = false;
            int idx = addr.length() + send.length() + cmd.length() + 3;
            int cmd_len = strlen(shm_ptr) - idx + 1;
            char temp[cmd_len];
            strcpy(temp, shm_ptr + idx);
            temp[cmd_len - 1] = '\0';
            int s_id = stoi(send);
            len = sprintf(shm_ptr, "all %u *** %s (#%d) just received from %s (#%d) by '%s' ***\n", 
                          user_data[s_id].user_pipe_box[id].count, user_data[id].name.c_str(), id, user_data[s_id].name.c_str(), s_id, temp);
        }
    }
    shm_ptr[len] = '\0';
}

int system_broadcast::find_user(std::string addr) {
    for(int i = 1; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) {
            if(user_data[i].addr.compare(addr) == 0)
                return i;
        }
    }
    return -1;
}

bool system_broadcast::dup_name(std::string name) {
    for(int i = 0; i < max_user_num + 1; ++i) {
        if(user_data[i].valid) {
            if(user_data[i].name.compare(name) == 0)
                return true;
        }
    }
    return false;
}