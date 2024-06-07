#ifndef KLIENT_H
#define KLIENT_H

#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <queue>
#include <mutex>
#include <array>
#include <cstring>
#include <string>
#include <netdb.h>
#include <cinttypes>
#include <poll.h>
#include <algorithm>
#include <signal.h>

#include "common.h"
#include "regex.h"
#include "senders.h"
#include "klient_printer.h"

using std::string;
using std::thread;
using std::vector;
using std::queue;
using std::mutex;
using std::cerr;
using std::cout;
using std::cin;
using std::find;
using std::system_error;

class Klient
{
public:
    Klient() = delete;
    Klient(const string& host, int32_t port, int16_t ip,
        const string& seat_name, bool AI);
    ~Klient() = default;

    /*
    * Main function used to run the client.
    */
    int16_t run_client();

private:
    /*
    * Wrapper for close_pipe_sockets() that will print error if needed.
    * On result == 1, it will join the interaction_thread.
    */
    void close_main_sockets(ssize_t result, const string& error_message);

    /*
    * Function tha will close all pipes.
    */
    void close_pipe_sockets();

    /*
    * Function that will close thread responsible for interaction with server.
    */
    void close_worker(int32_t socket_fd, const string& error_message,
        const string& fd_msg);

    /*
    * Function that will close thread responsible 
    * for interaction with client and main logic.
    */
    void close_main(const string& error_message, const string& fd_msg);

    /*
    * Function that will check if reading from socket was successful.
    * If not, it will close the socket and return -1.
    */
    int16_t assert_client_read_socket(ssize_t result, int32_t socket_fd);

    /*
    * Function that will check if writing to socket was successful.
    * If not, it will close the socket and return -1.
    */
    int16_t assert_client_write_socket(ssize_t result, ssize_t expected,
        int32_t socket_fd);

    /*
    * Function that will check if reading from pipe was successful.
    * If not, it will close the pipe and return -1.
    */
    int16_t assert_client_read_pipe(ssize_t result,
        int32_t socket_fd, bool is_main);

    /*
    * Function that will check if writing to pipe was successful.
    * If not, it will close the pipe and return -1.
    */
    int16_t assert_client_write_pipe(ssize_t result,
        int32_t socket_fd, bool is_main);

    /*
    * Wrapper for common::print_log() that will acquire mutex to have 
    * exclusive access to the standard stream.
    * Overload for IPv4 addresses.
    */
    void print_log(const struct sockaddr_in6& src_addr,
        const struct sockaddr_in6& dest_addr, const string& message);

    /*
    * Wrapper for common::print_log() that will acquire mutex to have
    * exclusive access to the standard stream.
    * Overload for IPv6 addresses.
    */
    void print_log(const struct sockaddr_in& src_addr,
        const struct sockaddr_in& dest_addr, const string& message);

    /*
    * Wrapper for common::print_error() that will acquire mutex to have
    * exclusive access to the error stream.
    */
    void print_error(const string& message);

    /*
    * Utility function to get the server socket. Depending on the IP version,
    * it will create either IPv4 or IPv6 socket. On soccess, it will return
    * the socket file descriptor. On failure, it will return -1.
    */
    int16_t prepare_client();

    /*
    * Function passed to the interaction_thread. It will handle the interaction
    * with the server.
    */
    void handle_client(int32_t socket_fd);

    /*
    * Function responsible for choosing the card
    * to play in the current trick if the client is AI.
    */
    string strategy(const string& color);

    struct sockaddr_in server_address;
    struct sockaddr_in6 server6_address;
    struct sockaddr_in client_address;
    struct sockaddr_in6 client6_address;

    thread interaction_thread;

    int32_t client_read_pipe[2];
    int32_t client_write_pipe[2];

    string host_name;
    int32_t port_number;
    int16_t ip_version;
    string seat;
    bool is_ai;

    mutex access_mutex;

    queue<string> messages_to_send;
    vector<vector<string>> taken_tricks;

    vector<string> my_cards;
    vector<string> played_cards;
    int16_t trick_number;

    bool got_score;
    bool got_total;
    string expected_color;
};

#endif // KLIENT_H