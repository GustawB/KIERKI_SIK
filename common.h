#include <string>
#include <iostream>
#include <unistd.h>

namespace common 
{
    #define DELIMETER "\r\n"

    using std::string;
    using std::cout;

    void read_from_socket(int socket_fd, string& buffer);
    void write_to_socket(int socket_fd, char* buffer, size_t buffer_length);
} // namespace common