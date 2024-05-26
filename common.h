#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define QUEUE_SIZE 69

#define MAX_BUFFER_SIZE 90 // for uint64_t max

#define MISSING_CLIENT_BARRIER 0
#define END_OF_TRICK_BARRIER 1

#define CONNECTIONS_THREAD "K"

#define DISCONNECTED "c"
#define CARD_PLAY "p"
#define BARRIER_RESPONSE "b"
#define SCORES "s"
#define TAKEN "t"
#define SERVER_DISCONNECT "d"
#define NORMAL_END "n"
#define ERROR "e"
#define DEAL "l"
#define END "f"

namespace common 
{
    #define DELIMETER "\r\n"

    using std::string;
    using std::cout;
    using std::cerr;

    ssize_t read_from_socket(int socket_fd, string& buffer);
    ssize_t read_from_pipe(int pipe_fd, string& buffer);
    ssize_t write_to_socket(int socket_fd, char* buffer, size_t buffer_length);
    ssize_t write_to_pipe(int pipe_fd, const string& buffer);
    ssize_t create_socket();
    ssize_t create_socket6();
    ssize_t setup_server_socket(int port, int queue_size, struct sockaddr_in6& server_addr);
    ssize_t accept_client(int socket_fd, struct sockaddr_in6& client_addr);

    ssize_t get_server_ipv4_addr(char const *host, uint16_t port, struct sockaddr_in& server_address);
    ssize_t get_server_ipv6_addr(char const *host, uint16_t port, struct sockaddr_in6& server_address);
    ssize_t get_server_unknown_addr(char const *host, uint16_t port, struct sockaddr_in& v4_addr, struct sockaddr_in6& v6_addr);

    void print_error(const string& error_message);

    void print_log(const struct sockaddr_in& source_addr, const struct sockaddr_in& dest_addr, const string& message);
    void print_log(const struct sockaddr_in6& source_addr, const struct sockaddr_in6& dest_addr, const string& message);
} // namespace common

#endif // COMMON_H
