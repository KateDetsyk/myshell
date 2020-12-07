#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <boost/program_options.hpp>


int writebuffer(int fd, const char* buffer, ssize_t size, int* status) {
    ssize_t written_bytes = 0;
    while( written_bytes < size ) {
        ssize_t written_now = write(fd, buffer + written_bytes, size - written_bytes);
        if(written_now == -1) {
            if(errno == EINTR)
                continue;
            else{
                if(status) {*status = errno;}
                return -1;
            }
        } else
            written_bytes += written_now;
    }
    return 0;
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



void transform_to_hex(char* buffer, size_t n, char* hex_buffer) {
    size_t c = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!isspace(buffer[i]) && !isprint(buffer[i])) {
            sprintf((char *) (hex_buffer + c), "\\x%02X", (unsigned char) buffer[i]);
            c += 4;
        } else {
            hex_buffer[c] = buffer[i];
            c += 1;
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "Usage : \n"
                     "mycat [-h|--help] [-A] <file1> <file2> ... <fileN>"
                     << std::endl;
        return 0;
    }
    const size_t BUFFSIZE = 1000000;
    namespace po = boost::program_options;
    po::options_description visible("Supported options");
    visible.add_options()
            ("help,h", "Print this help message.")
            ("alternate,A", "Display text like hex code.");
    po::options_description all("All options");
    all.add(visible);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Usage : \n mycat [-h|--help] [-A] <file1> <file2> ... <fileN>" << std::endl;
        return EXIT_SUCCESS;
    }
    int to_hex = 0;
    if (vm.count("alternate")) { to_hex = 1; }

    // check if all files can be open
    std::vector<int> fds;
    for (int i = 1; i < argc; i++) {
        if(strcmp("-A", argv[i]) == 0) { continue; }
        int fd = open(argv[i], O_RDONLY);
        fds.emplace_back(fd);
        if (fd == -1) {
            std::cerr << "mycat: can't open " << argv[i] << std::endl;
            return -1;
        }
    }
    
    ssize_t n;
    int indexR;
    int indexW;
    for (auto& fd : fds) {
        char buf[BUFFSIZE];
        while ((n = readbuffer(fd, buf, BUFFSIZE, &indexR)) > 0) {
            if (n == -1) {
                std::cerr << "mycat: error while reading : " << indexR << std::endl;
                return -1;
            }
            if (to_hex) {
                // CREATE BUF WITH PROPER SIZE
                size_t hex_buf_size = 0;
                for (size_t j = 0; j < n; ++j) {
                    if (!isspace(buf[j]) && !isprint(buf[j])) { hex_buf_size += 4; }
                    else { hex_buf_size += 1; }
                }
                char hex_buf[hex_buf_size];

                transform_to_hex(buf, n, hex_buf);

                if (writebuffer(1, hex_buf, hex_buf_size, &indexW) != 0) {
                    std::cerr << "mycat: error while writing : " << indexW << std::endl;
                    return -1;
                }
            } else {
                if (writebuffer(1, buf, n, &indexW) != 0) {
                    std::cerr << "mycat: error while writing : " << indexW << std::endl;
                    return -1;
                }
            }
        }
        if (close(fd) == -1){
            std::cerr << "Failed to close file." << std::endl;
            return -1;
        }
    }
    return 0;
}
