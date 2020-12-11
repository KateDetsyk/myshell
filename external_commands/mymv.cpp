#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>


std::string get_base_name(std::string& file) {
    return boost::filesystem::path(file).filename().string();
}

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

/*
mymv [-h|--help] [-f] <oldfile> <newfile>
mymv [-h|--help] [-f] <oldfile_or_dir_1> <oldfile_or_dir_oldfile2> <oldfile_or_dir_oldfile3>.... <dir>
*/
int main(int argc, char* argv[]) {
    int STATUS = 0;
    bool move_all = false;

    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options() ("help,h", "Print help message.")
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
        std::cout << "Use : mymv [-h|--help] [-f] <oldfile_or_dir_1> <oldfile_or_dir_oldfile2> "
                     "<oldfile_or_dir_oldfile3>.... <dir>" << std::endl;
        std::cout << visible << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (!vm.count("files") || (vm["files"].as<std::vector<std::string>>().size() < 2)) {
        std::cerr << "mymv: incorrect amount of arguments. Try 2 or more." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> files = vm["files"].as<std::vector<std::string>>();
    std::string destination = files.back(); // last one is always a destination
    files.pop_back();

    if (!boost::filesystem::exists(destination)) {
        std::cerr << "mymv: incorrect argument. Destination \'" << destination << "\' doesn't exist." << std::endl;
        exit(EXIT_FAILURE);
    }
    if (files.size() > 1 && !boost::filesystem::is_directory(destination)) {
        std::cerr << "mymv: incorrect argument. Destination \'" << destination << "\' isn't a directory." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string path;
    if (files.size() == 1) {
        if (boost::filesystem::is_directory(files[0]) && !boost::filesystem::is_directory(destination)) {
            std::cerr << "mymv:  cannot overwrite non-directory \'" << destination << "\' with directory \'" <<
                      files[0] << "\'." << std::endl;
            return -1;
        }
    }
    for (auto& file : files) {
        if (!boost::filesystem::exists(file)) {
            std::cerr << "mymv: cannot stat \'" << file << "\': No such file or directory." << std::endl;
            STATUS = -1;
            continue;
        }
        path = destination;
        if (boost::filesystem::is_directory(destination)) {
            path += "/";
            path += get_base_name(file);
        }
        if (boost::filesystem::exists(path) && !vm.count("-f") && !move_all) {
            char answer = process_answer(path);
            if (answer == 'y') {
                boost::filesystem::remove_all(path);
                boost::filesystem::rename(file, path);
            } else if (answer == 'n') {
                continue;
            } else if (answer == 'a') {
                move_all = true;
                boost::filesystem::remove_all(path);
                boost::filesystem::rename(file, path);
            } else {
                break;
            }
        } else {
            if (boost::filesystem::exists(path)) { boost::filesystem::remove_all(path); }
            boost::filesystem::rename(file, path);
        }
    }
    return STATUS;
}

