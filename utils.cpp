#include "utils.h"

extern char **environ;


int process_redirect(SARGS& sargs) {
    if (!sargs.output_file.empty() && !sargs.errno_file.empty()) {
        int fd = creat(sargs.output_file.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        if ( fcntl(fd, F_GETFD) < 0 || errno == EBADF ) { return -1; }
        dup2(fd, STDOUT_FILENO);
        dup2(STDOUT_FILENO, STDERR_FILENO);
        close(fd);
    }
    else if (!sargs.output_file.empty()) {
        int fd = creat(sargs.output_file.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        if ( fcntl(fd, F_GETFD) < 0 || errno == EBADF) { return -1; }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    else if (!sargs.errno_file.empty()) {
        int fd = creat(sargs.output_file.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        if ( fcntl(fd, F_GETFD) < 0 || errno == EBADF) { return -1; }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
    else if (!sargs.input_file.empty()) {
        int fd = open(const_cast<char*>(sargs.input_file.c_str()), O_RDONLY);
        if ( fcntl(fd, F_GETFD) < 0 || errno == EBADF) { return -1; }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    return 0;
}

bool check_if_wildcard(std::string& str) {
    bool star = false;
    bool bracket = false;
    for (auto& c : str) {
        if (c == '?') { return true; }
        else if (c == '*') { star = true; }
        else if (star && (c != '/')) { return true; }
        else if (c == '[') { bracket = true; }
        else if (c == ']' && bracket) { return true; }
    }
    return false;
}

void find_wildcards_matches(std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (check_if_wildcard(args[i])) {
            std::vector<std::string> filenames;

            glob_t glob_result;
            memset(&glob_result, 0, sizeof(glob_result));

            int return_value = glob(args[i].c_str(), GLOB_TILDE, NULL, &glob_result);
            if(return_value != 0) {
                std::cerr << "wildcards: no matches found for " << args[i] << std::endl;
                globfree(&glob_result);
            }
            for(size_t j = 0; j < glob_result.gl_pathc; ++j) {
                filenames.push_back(std::string(glob_result.gl_pathv[j]));
            }
            args.erase(args.begin() + i);
            args.insert(args.end(), filenames.begin(), filenames.end());
            globfree(&glob_result);
        }
    }
}

int process_run(std::string& prog, SARGS& sargs) {
    pid_t parent = getpid();
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork()");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        if(sargs.back_ground_execution) {
            signal(SIGCHLD,SIG_IGN);
            return 0;
        }
        int status;
        waitpid(pid, &status, 0);
        last_command_code = status;
    }
    else {
        std::vector<const char*> arg_for_c;
        for(auto& s: sargs.args) {
            arg_for_c.push_back(s.c_str());
        }
        arg_for_c.push_back(nullptr);

        int status = process_redirect(sargs);
        if (status != 0 ) { return -1; }

        if(sargs.back_ground_execution) {
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }

        execvp(prog.c_str(), const_cast<char* const*>(arg_for_c.data()));

        std::cerr << "Parent: Failed to execute " << prog << " \n\tCode: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

int pipe_run(SARGS& sargs) {                                   // support redirects
    int pipe_number = sargs.pipe_args.size();                  // support build in commands (mpwd, mecho)
    int fds[2 * pipe_number]; // read/write for everyone
    for (int i = 0; i < pipe_number; ++i) {
        if (pipe(fds + (i * 2)) == -1) {
            std::cerr << "Pipe error." << std::endl;
            return -1;
        }
    }
    pid_t pid;
    // [r, w, r, w]
    for ( size_t i = 0, j = 0; i < pipe_number; ++i, j+=2 ) {
        pid = fork();
        if ( pid == -1 ) {
            perror ("Failed to fork()");
            return -1;
        }
        else if ( pid == 0 ) {
            if (i < pipe_number - 1) { //not last elem
                if (dup2(fds[j + 1], STDOUT_FILENO) < 0) {
                    perror ("dup2 fail.");
                    return -1;
                }
            }
            if (i > 0) { // not first elem
                if (dup2(fds[j - 2], STDIN_FILENO) < 0) {
                    perror ("dup2 fail.");
                    return -1;
                }
            }
            for (size_t n = 0; n < 2 * pipe_number; ++n) {
                close(fds[n]);
            }
            find_wildcards_matches(sargs.pipe_args[i]);
            // if build in command
            if (_build_in_functions.find(sargs.pipe_args[i][0]) != _build_in_functions.end()) {
                sargs.pipe_args[i].insert(sargs.pipe_args[i].begin(), {"myshell", "-c"} );
            }
            std::vector<const char*> arg_for_c;
            for(auto& s: sargs.pipe_args[i]) {
                arg_for_c.push_back(s.c_str());
            }
            arg_for_c.push_back(nullptr);

            if (i == pipe_number-1) { // redirect only the final output
                int state_redirect = process_redirect(sargs);
                if (state_redirect != 0 ) { return -1; }
            }
            execvp(arg_for_c[0], const_cast<char* const*>(arg_for_c.data()));
        }
    }
    for ( size_t i = 0; i < 2 * pipe_number; ++i) {
        close(fds[i]);
    }
    int status;
    for (size_t i = 0; i < pipe_number; ++i) {
        wait(&status);
    }
    last_command_code = status;
    return 0;
}

int run_command(SARGS& sargs) {
    if (_build_in_functions.find(sargs.args[0]) != _build_in_functions.end()) {
        if(sargs.is_redirect) {
            sargs.args.insert(sargs.args.begin(), {"myshell", "-c"} );
        } else {
            return _build_in_functions[sargs.args[0]](sargs.args);
        }
    }
    return process_run(sargs.args[0], sargs);
}

//myls > file      # stdout to file
//myls 2> file    # stderr to file
//myls > file 2>&1 # both stdout and stderr to same file (use dup() )
//myls &> file     # both stdout and stderr to same file
SARGS split_command(char* str) {
    SARGS sargs;
    char* token = strtok(str, " \t\r\n\a");
    std::vector<std::string> args;
    bool err_redirect = false;
    bool out_redirect = false;
    bool both_redirect = false;
    bool input_redirect = false;
    while (token != NULL) {
        token[strcspn(token, "\n\r\a\t ")] = 0;
        // token can become empty after this step
        std::string arg = token;
        if (arg.size() > 0) {
            if (both_redirect) {
                sargs.errno_file = arg;
                sargs.output_file = arg;
            } else if (out_redirect) {
                if (sargs.output_file.empty()) {sargs.output_file = arg;}
                if (arg.compare("2>&1") == 0) {both_redirect = true;}
            } else if (input_redirect) {sargs.input_file = arg;}
            else if (err_redirect) {sargs.errno_file = arg;}
            else if (arg.compare(">") == 0) {out_redirect = true;}
            else if (arg.compare("2>") == 0) {err_redirect = true;}
            else if (arg.compare("&>") == 0) {both_redirect = true;}
            else if (arg.compare("<") == 0) {input_redirect = true;}
            else if (arg.compare("&") == 0) {sargs.back_ground_execution = true;}
            else if (arg.compare("|") == 0) {
                sargs.pipe = true;
                sargs.pipe_args.push_back(sargs.args);
                sargs.args = {};
            }
            else {
                sargs.args.push_back(arg);
            }
        }
        token = strtok(nullptr, " ");
    }
    if (out_redirect || err_redirect || both_redirect || input_redirect) { sargs.is_redirect = true; }
    if (both_redirect && out_redirect) {sargs.errno_file = sargs.output_file;}
    if (sargs.pipe && !sargs.args.empty()) {
        sargs.pipe_args.push_back(sargs.args);
//        sargs.args = {};
    }
    return sargs;
}


ssize_t readbuffer(int fd, char* buffer, ssize_t size, int* status) {
    ssize_t read_bytes = 0;
    ssize_t read_now = 1;
    while( (read_bytes < size) && (read_now > 0) ) {
        read_now = read(fd, buffer + read_bytes, size - read_bytes);
        if (read_now == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                if(status) {*status = errno;}
                return -1;
            }
        } else
            read_bytes += read_now;
    }
    return read_bytes;
}

int export_command_output(std::string& full_command) {
    // строгий запис без пробілів VAR=$(<command>) ???
    // split command
    std::string delimiter = "=$(";
    std::string var = full_command.substr(0, full_command.find(delimiter));
    std::string command = full_command.substr(full_command.find(delimiter) + delimiter.length());
    command.pop_back();  // remove ")"

    SARGS sargs = split_command(const_cast<char*>(command.c_str()));

    //check if it build in command
    if (_build_in_functions.find(sargs.args[0]) != _build_in_functions.end()) {
        sargs.args.insert(sargs.args.begin(), {"myshell", "-c"} );
    }
    // run
    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork()" << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child
        close(pipefd[0]);    // close reading end in the child
        dup2(pipefd[1], STDOUT_FILENO);  // send stdout to the pipe
        dup2(pipefd[1], STDERR_FILENO);  // send stderr to the pipe
        close(pipefd[1]);    // this descriptor is no longer needed

        std::vector<const char*> arg_for_c;
        for(auto& s: sargs.args) {
            arg_for_c.push_back(s.c_str());
        }
        arg_for_c.push_back(nullptr);

        int status_redirect = process_redirect(sargs);
        if (status_redirect != 0 ) { exit(EXIT_FAILURE); }
        execvp(sargs.args[0].c_str(), const_cast<char* const*>(arg_for_c.data()));

        std::cerr << "Parent: Failed to execute " << " \n\tCode: " << errno << std::endl;
        exit(EXIT_FAILURE);
    } else {
        // parent
        int status;
        waitpid(pid, &status, 0);
        last_command_code = status;

        close(pipefd[1]);
        const int BUFFSIZE = 1024;
        int indexR;
        char buf[BUFFSIZE];
        std::string data = "";
        ssize_t n;
        size_t total_len = 0;
        while ((n = readbuffer(pipefd[0], buf, BUFFSIZE, &indexR)) > 0) {
            if (n == -1) {
                std::cerr << "Setting variable: error while reading : " << indexR << std::endl;
                return -1;
            }
            total_len += n;
//            std::cout << "t" << std::endl;
            std::string current_data (buf);
            current_data.erase(n, BUFFSIZE - n);
            data = data + current_data;
        }
        std::cout << total_len << "    " << data.size() << std::endl;
        // add to environ
        if (environment_variables.find(var) != environment_variables.end()) {
            environment_variables[var] = data;
        } else {
            environment_variables.insert({var, data});
        }
    }
    return 0;
}

int cmd_msh(char* script) {                                     // support redirects (extern, build in)
    FILE* fp = fopen(script, "r");                              // support pipes (extern, build in)
    if (fp == NULL) {                                           // support var settings (extern, build in)
        std::cerr << "Can't open file." << std::endl;
        return EXIT_FAILURE;
    }
    char* line = NULL;
    size_t len = 0;
    while ((getline(&line, &len, fp)) != -1) {
        //check for var=$()
        std::string command_test(line);
        if (command_test.find("=$(") != std::string::npos) {
            std::cout << "here" << std::endl;
            last_command_code = export_command_output(command_test);
            continue;
        }
        SARGS sargs = split_command(line);
        if ((sargs.args.size() == 0) || boost::starts_with(sargs.args[0], "#")) { continue; }
        find_wildcards_matches(sargs.args);
        sargs.pipe ? pipe_run(sargs) : last_command_code = run_command(sargs);
    }
    fclose(fp);
    if (line)
        free(line);
    return 0;
}
