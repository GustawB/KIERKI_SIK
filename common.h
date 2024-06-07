#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <arpa/inet.h>
#include <netdb.h>

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
#define BARRIER_END "r"
#define DELIMETER "\r\n"

using std::string;
using std::cout;
using std::cerr;
using std::mutex;

namespace common
{
    /* 
     * Read from socket until DELIMETER is found.
     * Returns number of bytes read.
     */
    ssize_t read_from_socket(int32_t socket_fd, string& buffer);

    /* 
     * Read one character from the pipe.
     * Returns number of bytes read.
     */
    ssize_t read_from_pipe(int32_t pipe_fd, string& buffer);

    /* 
     * Write buffer to socket.
     * Returns number of bytes written.
     */
    ssize_t write_to_socket(int32_t socket_fd, char* buffer, 
        size_t buffer_length);

    /* 
     * Write buffer to pipe.
     * Returns number of bytes written.
     */
    ssize_t write_to_pipe(int32_t pipe_fd, const string& buffer);

    /*
    * Utility function to setup IPv4 socket.
    */
    int32_t create_socket();

    /*
    * Utility function to setup IPv6 socket.
    */
    int32_t create_socket6();

    /*
    * Utility function to setup server socket.
    * It will be able to accept IPv4 and IPv6 connections.
    * Returns socket file descriptor.
    */
    int32_t setup_server_socket(int32_t port, int32_t queue_size, 
        struct sockaddr_in6& server_addr);

    /*
    * Utility function to accept client connection.
    * Returns client file descriptor.
    */
    int32_t accept_client(int32_t socket_fd, struct sockaddr_in6& client_addr);

    /*
    * Utility function to setup client socket.
    * It will try to get IPv4 server address.
    * Returns 0 on success, -1 on failure.
    */
    ssize_t get_server_ipv4_addr(char const *host, int32_t port, 
        struct sockaddr_in& server_address);

    /*
    * Utility function to setup client socket.
    * It will try to get IPv6 server address.
    * Returns 0 on success, -1 on failure.
    */
    ssize_t get_server_ipv6_addr(char const *host, int32_t port, 
        struct sockaddr_in6& server_address);

    /*
    * Utility function to setup client socket.
    * It will try to get IPv4 or IPv6 server address.
    * Returns 0 on success, -1 on failure.
    */
    ssize_t get_server_unknown_addr(char const *host, int32_t port, 
        struct sockaddr_in& v4_addr, struct sockaddr_in6& v6_addr);

    /*
    * Utility function to check return value of the close().
    * On error prints error message.
    */
    void assert_close(int32_t fd);

    /*
    * Utility function to print error message.
    * If errno is set, it will print it.
    */
    void print_error(const string& error_message);

    /*
    * Utility function to print error message.
    * If errno is set, it will print it. Memory-safe version.
    */
    void print_error(const string& error_message, mutex& error_mutex);

    /*
    * Utility function to print log message for IPv4 addresses.
    */
    void print_log(const struct sockaddr_in& source_addr, 
        const struct sockaddr_in& dest_addr, const string& message, 
        mutex& log_mutex, bool is_ai = true);

    /*
    * Utility function to print log message for IPv6 addresses.
    */
    void print_log(const struct sockaddr_in6& source_addr, 
        const struct sockaddr_in6& dest_addr, const string& message,
        mutex& log_mutex, bool is_ai = true);
} // namespace common

#endif // COMMON_H
