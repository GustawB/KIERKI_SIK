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

class Klient
{
public:
    Klient() = delete;
    Klient(const string& host, int port, int ip, const string& seat_name, bool AI);
    ~Klient() = default;

    int run_client();

private:
    void close_worker_sockets(int socket_fd);
    void close_main_sockets(ssize_t result, const string& error_message);
    void close_pipe_sockets();

    void close_worker(int socket_fd, const string& error_message, const string& fd_msg);
    void close_main(const string& error_message, const string& fd_msg);

    int assert_client_read_socket(ssize_t result, int socket_fd);
    int assert_client_write_socket(ssize_t result, ssize_t expected, int socket_fd);

    int assert_client_read_pipe(ssize_t result, int socket_fd, bool is_main);
    int assert_client_write_pipe(ssize_t result, int socket_fd, bool is_main);

    void print_log(const struct sockaddr_in6& src_addr, const struct sockaddr_in6& dest_addr, const string& message);
    void print_log(const struct sockaddr_in& src_addr, const struct sockaddr_in& dest_addr, const string& message);
    void print_error(const string& message);

    int prepare_client();
    void handle_client(int socket_fd);

    string strategy(const string& color);

    struct sockaddr_in server_address;
    struct sockaddr_in6 server6_address;
    struct sockaddr_in client_address;
    struct sockaddr_in6 client6_address;

    thread interaction_thread;

    int client_read_pipe[2];
    int client_write_pipe[2];

    string host_name;
    int port_number;
    int ip_version;
    string seat;
    bool is_ai;

    mutex memory_mutex;
    mutex printing_mutex;

    queue<string> messages_to_send;
    vector<vector<string>> taken_tricks;

    vector<string> my_cards;
    int16_t trick_number;

    bool got_score;
    bool got_total;
};

#endif // KLIENT_H