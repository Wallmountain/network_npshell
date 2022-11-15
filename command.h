#ifndef COMMAND_H
#define COMMAND_H
#include <cstring>
#include <vector>
#include <sstream>
#include <iostream>
#include <regex>

#define IsDigit(ch) \
    (ch <= '9' && ch >= '0')

class command {
    public:

        command():cmd_now(0), cmd_count(0), instrusion_count(0) {};
        int read_string(char *input, int mode);
        bool isempty();
        char **get_next_cmd();
        char **get_last_cmd();
        void cal_last_cmd_count();
        void print();
        bool tail_of_instruction();
        unsigned get_instruction_count();
        char **change_cmd_type(std::vector<std::string> cmd);
    private:
        unsigned int cmd_now, cmd_count, last_cmd_count, instrusion_count;
        std::vector<std::vector<std::string>> cmds;
    
        enum TOKEN { STRING = 1, SYMBOL = 2, NUMPIPE = 3, USERPIPE = 4};

        bool check(char *input);
        bool check_network(char *input);
        bool check_format(const std::string& str,const std::regex& format);
};
#endif
