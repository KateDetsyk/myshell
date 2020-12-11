#include <iostream>
#include <vector>
#include <ftw.h>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>


std::vector<std::string> FILES;
int STATUS = 0;

//Y[es]/N[o]/A[ll]/C[cancel]
char process_answer(const std::string& file) {
    std::cout << "> File \'" << file << "\' already exits. Do you want to overwrite it?" << std::endl;
    std::cout << "Type Y[es]/N[o]/A[ll]/C[cancel]" <<  std::endl << ">> ";
    std::string answer;
    char c = 0;
    while (std::cin >> answer) {
        if (answer.empty()) { continue; }
        c = std::tolower(answer[0]);
        if (c != 'y' && c != 'n' && c != 'a' && c != 'c') {
            std::cout << "Type Y[es]/N[o]/A[ll]/C[cancel]" <<  std::endl << ">> ";
            continue;
        }
        break;
    }
    return c;
}

std::string get_base_name(std::string& file) {
    return boost::filesystem::path(file).filename().string();
}

static int get_entries(const char *fpath, const struct stat *st, int tflag, struct FTW *ftwbuf) {
    if (tflag == FTW_DNR) {
        std::cerr << "mycp: No permission to read " << fpath << std::endl;
        STATUS = -1;
    }
    if (tflag == FTW_F || ftwbuf->level > 0) {
        FILES.emplace_back(fpath);
    }
    return 0;
}

size_t check_long_path(std::string& file) {
    while (boost::starts_with(file, "/")) { file.erase(0, 1); }
    while (boost::algorithm::ends_with(file, "/")) { file.pop_back(); } // mb no sense to do it
    std::string base_name = get_base_name(file);
    return base_name != file ? file.size() - base_name.size() : 0;
}

void copy_dir_recursively(std::string& file, std::string& destination) {
    //make dict with files then -- copy every entry to the target in correct order
    int flags = FTW_MOUNT | FTW_PHYS | FTW_ACTIONRETVAL;
    if (nftw(const_cast<char *>(file.c_str()), get_entries, 1, flags) == -1) {
        perror("nftw");
        STATUS = -1;
    } else {
        std::string path = destination;
        path += "/";
        size_t len = check_long_path(file);
        // if target to copy is a long path to file "/files/dir/d/target", when we add this path to destination
        // we need just "target" -- "destination/target"; not "destination/files/dir/d/target"
        (len) ? path += file.substr(len - 1, file.size() - 1) : path += file;

        //if we enter this function we can overwrite
        if (boost::filesystem::exists(path)) { boost::filesystem::remove_all(path); }
        boost::filesystem::copy_directory(file, path); // we know that it's dir
        for (auto &entry : FILES) {
            path = destination;
            path += "/";
            (len) ? path += entry.substr(len - 1, entry.size() - 1) : path += entry;

            if (boost::filesystem::is_directory(entry)) {
                boost::filesystem::copy_directory(entry, path);
            } else {
                boost::filesystem::copy_file(entry, path, boost::filesystem::copy_option::overwrite_if_exists);
            }
        }
    }
    FILES.clear(); //clean vector for the next target
}

int main(int argc, char* argv[]) {
    bool copy_all = false;
    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options() ("help,h", "Print help message.")
            ("recursion,R", "Copy all dir entries recursively.")
            (",f", "Ask user to overwrite file : Y[es]/N[o]/A[ll]/C[cancel]");

    po::options_description invisible("invisible opptions.");
    invisible.add_options() ("files", po::value<std::vector<std::string>>(),
                             "files to copy and destination.");

    po::options_description all("All options");
    all.add(visible).add(invisible);
    po::positional_options_description positional;
    positional.add("files", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(positional).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Use : mycp [-h|--help] [-f] -R  <dir_or_file_1> <dir_or_file_2> <dir_or_file_3>... <dir>"
                  << std::endl;
        std::cout << visible << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (!vm.count("files") || (vm["files"].as<std::vector<std::string>>().size() < 2)) {
        std::cerr << "mycp: incorrect amount of arguments. Try 2 or more." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> files = vm["files"].as<std::vector<std::string>>();
    std::string destination = files.back(); // last one is always a destination
    files.pop_back();

    if (!boost::filesystem::exists(destination)) {
        std::cerr << "mycp: incorrect argument. Destination \'" << destination << "\' doesn't exist." << std::endl;
        exit(EXIT_FAILURE);
    }
    if (files.size() > 1 && !boost::filesystem::is_directory(destination)) {
        std::cerr << "mycp: incorrect argument. Destination \'" << destination << "\' isn't a directory." << std::endl;
        exit(EXIT_FAILURE);
    }

    // mycp [-h|--help] [-f] <file1> <file2>
    std::string path;
    if (files.size() == 1) {
        if (boost::filesystem::is_directory(files[0]) && !boost::filesystem::is_directory(destination)) {
            std::cerr << "mycp:  cannot overwrite non-directory \'" << destination << "\' with directory \'" <<
                      files[0] << "\'." << std::endl;
            return -1;
        }
    }
    if (!vm.count("recursion")) { //NOOOOO recursion
        for (auto& file : files) {
            if (boost::filesystem::is_directory(file)) {
                std::cerr << "mycp: -R not specified; omitting directory \'" << file << "\'." << std::endl;
                STATUS = -1;
                continue;
            } else if (!boost::filesystem::exists(file)) {
                std::cerr << "mycp: cannot stat \'" << file << "\': No such file or directory." << std::endl;
                STATUS = -1;
                continue;
            }
            path = destination;
            if (boost::filesystem::is_directory(destination)) {
                path += "/";
                path += get_base_name(file);
            }
            if (boost::filesystem::exists(path) && !vm.count("-f") && !copy_all) {
                char answer = process_answer(path);
                if (answer == 'y') {
                    boost::filesystem::copy_file(file, path,
                                                 boost::filesystem::copy_option::overwrite_if_exists);
                } else if (answer == 'a') {
                    copy_all = true;
                    boost::filesystem::copy_file(file, path,
                                                 boost::filesystem::copy_option::overwrite_if_exists);
                } else if (answer == 'n') {
                    continue;
                } else {
                    break;
                }
            } else {
                boost::filesystem::copy_file(file, path,
                                             boost::filesystem::copy_option::overwrite_if_exists);
            }
        }
    } else {
        for (auto& file : files) {
            if (!boost::filesystem::exists(file)) {
                std::cerr << "mycp: cannot stat \'" << file << "\': No such file or directory." << std::endl;
                STATUS = -1;
                continue;
            }
            path = destination;
            if (boost::filesystem::is_directory(destination)) {
                path += "/";
                path += get_base_name(file);
            }
            if (boost::filesystem::exists(path) && !vm.count("-f") && !copy_all) {
                char answer = process_answer(path);
                if (answer == 'y') {
                    if (boost::filesystem::is_directory(file)) {
                        copy_dir_recursively(file, destination);
                    } else {
                        boost::filesystem::copy_file(file, path,
                                                     boost::filesystem::copy_option::overwrite_if_exists);
                    }
                } else if (answer == 'a') {
                    copy_all = true;
                    if (boost::filesystem::is_directory(file)) {
                        copy_dir_recursively(file, destination);
                    } else {
                        boost::filesystem::copy_file(file, path,
                                                     boost::filesystem::copy_option::overwrite_if_exists);
                    }
                } else if (answer == 'n') {
                    continue;
                } else {
                    break;
                }
            } else {
                if (boost::filesystem::is_directory(file)) {
                    copy_dir_recursively(file, destination);
                } else {
                    boost::filesystem::copy_file(file, path,
                                                 boost::filesystem::copy_option::overwrite_if_exists);
                }
            }
        }
    }
    return STATUS;
}