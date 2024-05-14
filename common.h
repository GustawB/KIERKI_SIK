#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_MESSAGE_SIZE 

namespace common 
{
    #define DELIMETER "\r\n"

    using std::string;
    using std::cout;
    using std::cerr;

    ssize_t read_from_socket(int socket_fd, string& buffer);
    ssize_t read_from_pipe(int pipe_fd, string& buffer);
    ssize_t write_to_socket(int socket_fd, char* buffer, size_t buffer_length);
    ssize_t write_to_pipe(int pipe_fd, char* buffer);
    ssize_t create_socket();
    ssize_t setup_server_socket(int port, int queue_size);
    ssize_t accept_client(int socket_fd);

    void print_error(const string& error_message);
} // namespace common

#endif // COMMON_H
