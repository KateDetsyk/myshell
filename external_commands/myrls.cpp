#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <pwd.h>
#include <ctype.h>

#include <boost/program_options.hpp>

struct MyStats{
    std::string name;
    struct stat file_stats;
};

void print_file(MyStats& obj) {
    std::vector<int> permission_bits = { S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                                         S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH };
    std::vector<std::string> permissions = {"r", "w", "x", "r", "w", "x", "r", "w", "x"};
    std::string pass = "-";
    for (int i = 0; i < permission_bits.size(); ++i) {
        printf("%s", (obj.file_stats.st_mode & permission_bits[i]) ? permissions[i].c_str() : pass.c_str() );
    }

    char date[20];
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", gmtime(&(obj.file_stats.st_ctime)));
    struct passwd *pw = getpwuid(obj.file_stats.st_uid);
    printf(" %-11s %8jd  %-20s ", pw->pw_name, (intmax_t) obj.file_stats.st_size, date);

    if (S_ISDIR(obj.file_stats.st_mode)){
        printf("/");
    } else if ((obj.file_stats.st_mode & S_IEXEC) != 0){
        printf("*");
    } else if(S_ISLNK(obj.file_stats.st_mode)){
        printf("@");
    } else if(S_ISFIFO(obj.file_stats.st_mode)) {
        printf("|");
    }else if(S_ISSOCK(obj.file_stats.st_mode)) {
        printf("=");
    } else if (!S_ISREG(obj.file_stats.st_mode)){
        printf("?");
    }
    printf("%s\n",  obj.name.c_str());
}

// Small letters before big letters
// c, cmake, cT, cTz
bool compare_alphabeticaly(MyStats& a, MyStats& b) {
    bool a_smaller = false;
    int size;
    (a.name.size() < b.name.size()) ? size = a.name.size(), a_smaller = true : size = b.name.size();
    for (size_t i = 0; i < size; ++i) {
        if (a.name[i] != b.name[i]) {
            if (islower(a.name[i])) {
                return (isupper(b.name[i])) ? true : a.name[i] < b.name[i];
            } else {
                return (islower(b.name[i])) ? false : a.name[i] < b.name[i];
            }
        }
    }
    return a_smaller;
}

int recursive_read(std::string& path) {
    std::vector<MyStats> dirs;
    std::vector<MyStats> all_files_in_dir;
    DIR *dir;
    struct dirent *entry;
    struct stat states;

    dir = opendir(const_cast<char*>(path.c_str()));
    if (!dir) {
        // can be file
        if (stat(path.c_str(), &states) == -1) {
            fprintf(stderr, "%s",  "Myrls: incorrect argument.\n");
//            std::cerr << "Myrls: incorrect argument." << std::endl;
            return -1;
        }

        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
        MyStats st;
        st.name = base_filename;
        st.file_stats = states;
        print_file(st);
        return 0;
    };

    while ( (entry = readdir(dir)) != NULL) {
        stat(entry->d_name,&states);
        if(!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)){ continue; }

        MyStats st;
        st.name = entry->d_name;
        st.file_stats = states;

        all_files_in_dir.push_back(st);

        if(entry->d_type == DT_DIR) {
            dirs.push_back(st);
        }
    }
    if (!all_files_in_dir.empty()) {
        printf("%s:\n", path.c_str());
        std::sort(all_files_in_dir.begin(), all_files_in_dir.end(), compare_alphabeticaly);
        for (auto &f: all_files_in_dir) {
            print_file(f);
        }
        printf("\n");
    }

    std::sort(dirs.begin(), dirs.end(), compare_alphabeticaly); //sort the vector
    for (auto& d: dirs) {
        std::string new_path = path;
        new_path += "/";
        new_path += d.name;
        recursive_read(new_path);
    }
    closedir(dir);
    return 0;
}

int main(int argc, char *argv[]) {
    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options() ("help,h", "Print this help message.");
    po::options_description all("All options");
    all.add(visible);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << "Usage : \n myrls [-h|--help] [-A] <directory_name> | <path/dir_name> | <file> |"
                     " <path/file>." << std::endl;
        return EXIT_SUCCESS;
    }
    if (argc > 2) {
        fprintf(stderr, "%s",  "Myrls: incorrect argument.\n");
        return -1;
    }
    std::string dir = (argc < 2) ? "." : argv[1];
    if (recursive_read(dir)) { return -1; };
    return 0;
};