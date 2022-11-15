#include "command.h"

bool command::isempty() {
    return cmd_now == cmd_count;
}

bool command::check_network(char *input) {
    std::string str(input);
    std::string token;
    std::stringstream read_str(str);

    const std::regex STRING ("[0-9a-zA-Z\\.\\-:]+");
    const std::regex SYMBOL ("[>|]");
    const std::regex NUMPIPE ("[|!][1-9][0-9]*");
    const std::regex USERPIPE ("[<>][0-9]*");

    int type = 0, last_type = -1;

    while(read_str >> token) {
        if(check_format(token, STRING)) {
            if(last_type == TOKEN::USERPIPE)
                return false;
            type = TOKEN::STRING;
        } else if(check_format(token, SYMBOL)) {
            type = TOKEN::SYMBOL;
            if(last_type == TOKEN::SYMBOL || last_type == -1)
                return false;
        } else if(check_format(token, NUMPIPE)) {
            type = TOKEN::NUMPIPE;
            if(last_type == TOKEN::SYMBOL || last_type == -1)
                return false;
            type = -1;
        }else if(check_format(token, USERPIPE)) {
            type = TOKEN::USERPIPE;
            if(last_type == TOKEN::SYMBOL || last_type == -1)
                return false;
        } else {
            return false;
        }
        last_type = type;
    }

    return last_type != TOKEN::SYMBOL;
}

bool command::check(char *input) {
    std::string str(input);
    std::string token;
    std::stringstream read_str(str);

    const std::regex STRING ("[0-9a-zA-Z\\.\\-:]+");
    const std::regex SYMBOL ("[>|]");
    const std::regex NUMPIPE ("[|!][1-9][0-9]*");

    int type = 0, last_type = 0;

    while(read_str >> token) {
        if(check_format(token, STRING)) {
            type = TOKEN::STRING;
        } else if(check_format(token, SYMBOL)) {
            type = TOKEN::SYMBOL;
            if(last_type != TOKEN::STRING)
                return false;
        } else if(check_format(token, NUMPIPE)) {
            type = TOKEN::NUMPIPE;
            if(last_type != TOKEN::STRING)
                return false;
            type = 0;
        } else {
            return false;
        }
        last_type = type;
    }

    return last_type != TOKEN::SYMBOL;
}

bool command::check_format(const std::string& str, const std::regex& format) {
    std::smatch smt;

    if(!std::regex_search(str, smt, format))
        return false;

    return smt[0].str().length() == str.length();
}

int command::read_string(char *input, int mode) {
    std::string str(input);
    std::string token;
    std::stringstream read_str(str);
    int instrution_end = 0;

    int check_result = 0;

    if(mode == 0)
        check_result = check(input);
    else
        check_result = check_network(input);

    if(!check_result) {
        std::vector<std::string> tokens; 
        while(read_str >> token)
            tokens.push_back(token);
        if(tokens.size()) {
            cmds.push_back(tokens);
            ++cmd_count;
        }
    }

    while(read_str >> token) {
        instrution_end = 0;
        ++cmd_count;

        std::vector<std::string> tokens;
        tokens.push_back(token);
        while(!instrution_end && read_str >> token) {
            if(token[0] == '|' || token[0] == '!') {
                if(token.length() == 1)
                    instrution_end = 1;
                else
                    instrution_end = 2;
            } else if(token[0] == '>' && token.length() == 1) {
                cmds.push_back(tokens);
                
                while(tokens.size())
                    tokens.pop_back();

                tokens.push_back(token);
                read_str >> token;

                ++cmd_count;
                instrution_end = 0;
            } else if((token[0] == '>' || token[0] == '<') && token.length() > 1) {
                cmds.push_back(tokens);
                while(tokens.size())
                    tokens.pop_back();

                ++cmd_count;
                instrution_end = 0;
            }
            if(!instrution_end)
                tokens.push_back(token);
        }

        cmds.push_back(tokens);
        if(instrution_end)
            cmds.push_back({token});

        if(instrution_end == 2)
            cmds.push_back({});
        cmd_count += instrution_end;
    }
    if(instrution_end < 2) {
        cmds.push_back({});
        ++cmd_count;
    }
    return 1;
}

void command::print(){
    std::cout << cmd_count << std::endl;
    for(auto cmd : cmds) {
        for(auto token : cmd) {
            std::cout << token << " ";
        }
        std::cout << std::endl;
    }
}

char **command::get_next_cmd() {
    if(isempty())
        return NULL;
    if(cmd_now + 2 <= cmd_count && cmds[cmd_now + 1].size() == 0) {
        ++instrusion_count;
    }
    return change_cmd_type(cmds[cmd_now++]);
}

char **command::get_last_cmd() {
    if(isempty())
        return NULL;
    return change_cmd_type(cmds[last_cmd_count++]);
}

void command::cal_last_cmd_count() {
    last_cmd_count = cmd_now;
    while(last_cmd_count != 0 && cmds[last_cmd_count - 1].size() != 0)
        --last_cmd_count;
}

unsigned command::get_instruction_count() {
    return instrusion_count;
}

char **command::change_cmd_type(std::vector<std::string> cmd) {
    char **res = new char*[cmd.size() + 1];
    int i = 0;
    for(std::string x : cmd) {
        res[i] = new char[x.length()];
        strcpy(res[i++], x.c_str());
    }
    res[i] = NULL;
    return res;
}

bool command::tail_of_instruction() {
    return cmd_now + 1 == cmd_count;
}