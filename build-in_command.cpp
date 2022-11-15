#include "build-in_command.h"

int check_buildin_command(char **cmd) {
    int len = 0;
    string_len(cmd, len);
    if(!len)
        return 0;
    if(!strcmp(cmd[0],"exit") && len == 1)
        return -1;
    
    if(!strcmp(cmd[0], "setenv") && len == 3) {
        setenv(cmd[1], cmd[2], 1);
        return 1;
    } else if(!strcmp(cmd[0], "printenv") && len == 2) {
        char *temp = getenv(cmd[1]);
        if(temp)
            std::cout << temp << std::endl;
        return 2;
    } else if(!strcmp(cmd[0], "who") && len == 1) {
        return 3;
    } else if(!strcmp(cmd[0], "name") && len == 2) {
        return 4;
    } else if(!strcmp(cmd[0], "tell") && len >= 3) {
        return 5;
    } else if(!strcmp(cmd[0], "yell") && len >= 2) {
        return 6;
    }
    return 0;
}