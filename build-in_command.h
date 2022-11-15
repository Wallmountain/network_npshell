#ifndef BUILDIN_COMMAND_H
#define BUILDIN_COMMAND_H
#include <cstring>
#include <vector>
#include <iostream>

#define string_len(s, l)  \
    do {                  \
        l = 0;            \
        while(s[l])       \
            ++l;          \
    }while(0);    

int check_buildin_command(char **cmd);


#endif