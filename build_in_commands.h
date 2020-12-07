#ifndef MYSHELL_BUILD_IN_COMMANDS_H
#define MYSHELL_BUILD_IN_COMMANDS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>
#include <map>

#include "boost/lexical_cast.hpp"
#include<boost/algorithm/string.hpp>


extern int last_command_code;
extern std::map<std::string, std::string> environment_variables;

int help_option(std::vector<std::string>& args);

int merrno(std::vector<std::string>& args);

int mexport(std::vector<std::string>& args);

int mexit(std::vector<std::string>& args);

int help(std::vector<std::string>& args);

int mcd(std::vector<std::string>& args);

int mecho(std::vector<std::string>& args);

int mpwd(std::vector<std::string>& args);

extern std::vector<std::string> build_in_commands;

extern std::map<std::string, int(*)(std::vector<std::string>&)> _build_in_functions;

extern std::vector<std::string> external_commands;

#endif //MYSHELL_BUILD_IN_COMMANDS_H