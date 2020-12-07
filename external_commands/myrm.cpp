#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>


//Y[es]/N[o]/A[ll]/C[cancel]
char process_answer(const std::string& file) {
    std::cout << "> Do you want to remove file \'" << file << "\' ?" << std::endl;
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


// myrm [-h|--help] [-f] [-R] <file1> <file2> <file3>
int main(int argc, char* argv[]) {
    int STATUS = 0;
    bool remove_all = false;
    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options() ("help,h", "Print help message.")
            ("recursion,R", "Delete all dir entries recursively.")
            (",f", "Ask user to overwrite file : Y[es]/N[o]/A[ll]/C[cancel]");

    po::options_description invisible("invisible opptions.");
    invisible.add_options() ("files", po::value<std::vector<std::string>>(),
                             "files to delete.");

    po::options_description all("All options");
    all.add(visible).add(invisible);
    po::positional_options_description positional;
    positional.add("files", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(positional).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Use : myrm [-h|--help] [-f] [-R] <file1> <file2> <file3>" << std::endl;
        std::cout << visible << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (!vm.count("files")) {
        std::cerr << "myrm: incorrect amount of arguments. Try 1 or more." << std::endl;
        exit(EXIT_FAILURE);
    }

    for (auto& file : vm["files"].as<std::vector<std::string>>()) {
        if (!boost::filesystem::exists(file)) {
            std::cerr << "myrm:  cannot remove \'" << file << "\' : No such file or directory." <<  std::endl;
            STATUS = -1;
        } else if (vm.count("recursion")) {
            // no f
            if (!vm.count("-f") && !remove_all) {
                char answer = process_answer(file);
                if (answer == 'y') {
                    boost::filesystem::remove_all(file);
                } else if (answer == 'n') {
                    continue;
                } else if (answer == 'a') {
                    boost::filesystem::remove_all(file);
                    remove_all = true;
                } else if (answer == 'c') {
                    break;
                }
            } else {
                // f or remove all
                boost::filesystem::remove_all(file);
            }
        } else  {
            if (boost::filesystem::is_directory(file)) {
                std::cerr << "myrm:  cannot remove \'" << file << "\' : Is a directory." << std::endl;
                STATUS = -1;
            } else {
                //no f
                if (!vm.count("-f") && !remove_all) {
                    char answer = process_answer(file);
                    if (answer == 'y') {
                        boost::filesystem::remove(file);
                    } else if (answer == 'n') {
                        continue;
                    } else if (answer == 'a') {
                        remove_all = true;
                        boost::filesystem::remove(file);
                    } else if (answer == 'c') {
                        break;
                    }
                } else {
                    // f or remove_all
                    boost::filesystem::remove(file);
                }
            }
        }
    }
    return STATUS;
}
