#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <iostream>
#include <unistd.h>

#define MAX_MESSAGE_SIZE 

namespace common 
{
    #define DELIMETER "\r\n"

    using std::string;
    using std::cout;

    ssize_t read_from_socket(int socket_fd, string& buffer);
    ssize_t write_to_socket(int socket_fd, char* buffer, size_t buffer_length);
} // namespace common

#endif // COMMON_H
