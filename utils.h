#ifndef MYSHELL_UTILS_H
#define MYSHELL_UTILS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "glob.h"
#include "build_in_commands.h"

struct SARGS{
    std::vector<std::string> args {};
    std::vector<std::vector<std::string>> pipe_args {};
    std::string input_file {};
    std::string output_file {};
    std::string errno_file {};
    bool is_redirect = {false};
    bool back_ground_execution {false};
    bool pipe {false};
};

int process_redirect(SARGS& sargs);

bool check_if_wildcard(std::string& str);

void find_wildcards_matches(std::vector<std::string>& args);

int process_run(std::string& prog, SARGS& sargs);

int pipe_run(SARGS& sargs);

int run_command(SARGS& sargs);

SARGS split_command(char* str);

ssize_t readbuffer(int fd, char* buffer, ssize_t size, int* status);

int export_command_output(std::string& full_command);

int cmd_msh(char* script);


#endif //MYSHELL_UTILS_H
