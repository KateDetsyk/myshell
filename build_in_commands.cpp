#include "build_in_commands.h"

int last_command_code = 0;
std::map<std::string, std::string> environment_variables;


int help_option(std::vector<std::string>& args) {
    if (args.size() > 1) {
        for(auto& a : args) {
            if ((a.compare("-h") == 0) || (a.compare("--help") == 0)) {
                return 1;
            } else if (a[0] == '-') {
                return -1;
            }
        }
    }
    return 0;
}

int merrno(std::vector<std::string>& args) {
    if (args.size() > 2) {
        std::cerr << "merrno: too many arguments." << std::endl;
        return -1;
    }

    int opt = help_option(args);
    if (opt == 1) {
        std::cout << "Use: merrno [-h|--help]" << std::endl;
        return 0;
    }else if (opt == -1) {
        std::cerr << "Unrecognized option. Available options : -h|--help. " << std::endl;
        return -1;
    } else if (args.size() == 2) {
        std::cerr << "merrno: wrong arguments. Try: merrno [-h|--help]" << std::endl;
        return -1;
    }else {
        std::cout << last_command_code << std::endl;
        return 0;
    }
//    return -1;
}

int mexport(std::vector<std::string>& args) {
// myexport var=somthing
    if (args.size() != 2) {
        std::cerr << "mexport: wrong amount of arguments!" << std::endl;
        return -1;
    }
    char* token = strtok(const_cast<char*>(args[1].c_str()), "=");
    std::vector<std::string> variables;
    while (token != NULL)
    {
        variables.push_back(token);
        token = strtok(NULL, " ");
    }
    if (variables.size() != 2) {
        std::cerr << "mexport: wrong amount of arguments!" << std::endl;
        return -1;
    }

    if (environment_variables.find(variables[0]) != environment_variables.end()) {
        environment_variables[variables[0]] = variables[1];
    } else {
        environment_variables.insert({variables[0], variables[1]});
    }
    return 0;
}

int mexit(std::vector<std::string>& args) {
    int opt = help_option(args);
    if (opt == 1) {
        std::cout << "Use: mexit [finish code] [-h|--help]" << std::endl;
        return 0;
    } else if (opt == -1) {
        std::cerr << "Unrecognized option. Available options : -h|--help. " << std::endl;
        return -1;
    }
    int exit_code;
    if (args.size() == 1) {
        exit(EXIT_SUCCESS);}
    else if (args.size() == 2){
        try {
            exit_code = boost::lexical_cast<int>(args[1]);
        } catch (...) {
            std::cerr << "exit: Invalid argument. Should be integer!" << std::endl;
            exit(EXIT_FAILURE);
        }
        exit(exit_code);
    }
    else {
        std::cerr << "exit: Too many arguments." << std::endl;
        exit(EXIT_FAILURE);
    }
}

int help(std::vector<std::string>& args) {
    std::cout << "Myshell version 0.0.2" << std::endl;
    std::cout << "Type command and arguments." << std::endl;
    std::cout << "\nAvailable commands :" << "\n" << std::endl;
    std::cout << "Build in commands \n";
    for (auto& c : build_in_commands) {
        std::cout << " > " << c << std::endl;
    }
    std::cout << "\nExternal commands \n";
    for (auto& cc : external_commands) {
        std::cout << " > " << cc << std::endl;
    }
    std::cout << "\nSupport:\n" << " > redirections\n" << " > pipes" << std::endl;
    return 0;
}


int mcd(std::vector<std::string>& args) {
    int opt = help_option(args);
    if (opt == 1) {
        std::cout << "Use: mcd <path> [-h|--help]" << std::endl;
        return 0;
    } else if (opt == -1) {
        std::cerr << "Unrecognized option. Available options : -h|--help. " << std::endl;
        return -1;
    }
    if (args.size() < 2) {
        std::cerr << "mcd : give path for mcd." << std::endl;
        return -1;
    }
    else if (args.size() == 2) {
        if (chdir(const_cast<char*>(args[1].c_str())) == 0) {
            return 0;
        }else {
            std::cerr << "mcd : error while change dir, path can be incorrect." << std::endl;
            return -1;
        }
    }
    else {
        std::cerr << "mcd: Too many arguments." << std::endl;
        return -1;
    }
}

int mecho(std::vector<std::string>& args) {
    int opt = help_option(args);
    if (opt == 1) {
        std::cout << "Use: mecho [-h|--help] [text|$<var_name>] [text|$<var_name>]  [text|$<var_name>] ..." << std::endl;
        return 0;
    } else if (opt == -1) {
        std::cerr << "Unrecognized option. Available options : -h|--help. " << std::endl;
        return -1;
    }
    if (args.size() < 2) {
        std::cerr << "mecho: NO arguments provided" << std::endl;
        return -1;
    }else {
        for (int i = 1; i < args.size(); ++i) {
            if (boost::starts_with(args[i], "$")) {
                if (environment_variables.find( args[i].substr(1) ) != environment_variables.end()) {
                    std::cout << environment_variables[args[i].substr(1)] << std::endl;
                    continue;
                } else {
                    std::cerr << "mecho: environmental variable " << args[i] << " doesn't exist." << std::endl;
                    return -1;
                }
            }
            std::cout << args[i];
            if (i != args.size() - 1) {
                std::cout << " ";
            }else {
                std::cout << std::endl;
            }
        }
        return 0;
    }
}


int mpwd(std::vector<std::string>& args) {
    int opt = help_option(args);

    if (opt == 1) {
        std::cout << "Use: mpwd [-h|--help]" << std::endl;
        return 0;
    } else if (opt == -1) {
        std::cerr << "Unrecognized option. Available options : -h|--help. " << std::endl;
        return -1;
    }
    if (args.size() != 1) {
        std::cerr << "mpwd: Too many arguments" << std::endl;
        return -1;
    }else {
        int size_dir = 1024;
        char curr_dir[size_dir];
        if (getcwd(curr_dir, size_dir) != nullptr) {
            std::cout << std::string(curr_dir) << std::endl;
            return 0;
        }
        else {
            std::cerr << "mpwd: Error getting current directory" << std::endl;
            return -1;
        }
    }
}

std::vector<std::string> build_in_commands = {"mexit",
                                              "help",
                                              "mcd",
                                              "mecho",
                                              "mpwd",
                                              "mexport",
                                              "merrno"};


std::map<std::string, int(*)(std::vector<std::string>&)> _build_in_functions = {{"mexit", mexit},
                                                                                {"help", help},
                                                                                {"mcd", mcd},
                                                                                {"mecho", mecho},
                                                                                {"mpwd", mpwd},
                                                                                {"mexport", mexport},
                                                                                {"merrno", merrno}};

std::vector<std::string> external_commands = {"mycat",
                                              "myrls"};