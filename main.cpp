#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include<boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "build_in_commands.h"
#include "utils.h"


int myshell_run_build_func(std::vector<std::string>& args) {                           // -c option
    if (_build_in_functions.find(args[0]) != _build_in_functions.end()) {
        return _build_in_functions[args[0]](args);
    }
    return -1;
}

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;                              // -c option
    po::options_description visible("Supported options");
    visible.add_options() ("command,c", "Run build in command.");
    po::options_description all("All options");
    all.add(visible);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).run(), vm);
    po::notify(vm);
    if (vm.count("command")) {
        std::vector<std::string> args = {};
        for (int i = 0; i < argc; ++i) {
            if (strcmp(argv[i], "myshell") && strcmp(argv[i], "-c") && strcmp(argv[i], "./myshell")) {
                args.push_back(argv[i]);
            }
        }
        return myshell_run_build_func(args);
    }

    std::string c_path = boost::filesystem::current_path().c_str();
    auto path_ptr = getenv("PATH");
    std::string path_var;
    if (path_ptr != nullptr)
        path_var = path_ptr;
    path_var += ":";
    path_var += c_path;
    setenv("PATH", path_var.c_str(), 1);

    if ((argc > 1) && (boost::algorithm::ends_with(argv[1] , ".msh"))) { cmd_msh(argv[1]); }

    char* buf;
    std::cout << boost::filesystem::current_path().c_str();

    while ((buf = readline("$ ")) != nullptr) {
        if (strlen(buf) > 0) { add_history(buf); }

        //check for var=$()
        std::string command_test(buf);
        if (command_test.find("=$(") != std::string::npos) {
            std::cout << "here" << std::endl;
            last_command_code = export_command_output(command_test);
            free(buf);
            std::cout << boost::filesystem::current_path().c_str();
            continue;
        }

        SARGS sargs = split_command(buf);
        find_wildcards_matches(sargs.args);

        if (sargs.args.size() != 0 and (!boost::starts_with(sargs.args[0], "#"))) {
            if (boost::starts_with(sargs.args[0], "./")) {
                sargs.args[0].erase (0, 2);
                cmd_msh(const_cast<char*>(sargs.args[0].c_str()));
            } else if (sargs.args[0][0] == '.') {
                cmd_msh(const_cast<char*>(sargs.args[1].c_str()));
            } else if (sargs.pipe) {
                last_command_code = pipe_run(sargs);
            } else {
                last_command_code = run_command(sargs);
            }
        }
        free(buf);
        std::cout << boost::filesystem::current_path().c_str();
    }
    return 0;
}
