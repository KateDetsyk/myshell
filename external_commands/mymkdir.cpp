#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>



int main(int argc, char* argv[]) {
    int STATUS = 0;
    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options() ("help,h", "Print help message.")
                          ("parents,p", "No error if directory already exists.");
    po::options_description invisible("invisible opptions.");
    invisible.add_options() ("dirs", po::value<std::vector<std::string>>(),"dirs to create.");

    po::options_description all("All options");
    all.add(visible).add(invisible);
    po::positional_options_description positional;
    positional.add("dirs", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(positional).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Use : mymkdir [-h|--help] [-p]  <dirname>" << std::endl;
        std::cout << visible << std::endl;
        exit(EXIT_SUCCESS);
    }
    if (!vm.count("dirs")) {
        std::cerr << "mymkdir: no targets provided." << std::endl;
        exit(EXIT_FAILURE);
    }

    // real mkdir can create few dirs and if one target is wrong-path, other correct targets will be created
    for (auto& dir_path : vm["dirs"].as<std::vector<std::string>>()) {
        if (vm.count("parents")) {
            if (!boost::filesystem::create_directories(dir_path) && !boost::filesystem::exists(dir_path)) {
                std::cerr << "mymkdir: directory " << dir_path << " wasn't created." << std::endl;
                STATUS = -1;
            }
        } else if (boost::filesystem::exists(dir_path)) {
            std::cerr << "mymkdir: cannot create directory " << dir_path << ": File exists." << std::endl;
            STATUS = -1;
        } else {
            if (!boost::filesystem::create_directory(dir_path)) { //returns true or false
                std::cerr << "mymkdir: cannot create directory " << dir_path
                          << ": No such file or directory" << std::endl;
                STATUS = -1;
            }
        }
    }
    return STATUS;
}

