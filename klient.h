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

class Klient
{
public:
    Klient() = delete;
    Klient(const string& host, int port, int ip, const string& seat_name, bool AI);
    ~Klient() = default;

    int run_client();

private:
    void get_server_address(char const *host, uint16_t port);

    void close_worker_sockets(int socket_fd);
    void close_main_sockets(ssize_t result, const string& error_message);

    void close_worker(int socket_fd, const string& error_message, const string& fd_msg);
    void close_main(const string& error_message, const string& fd_msg);

    int assert_client_read_socket(ssize_t result, int socket_fd);
    int assert_client_write_socket(ssize_t result, int socket_fd);

    int assert_client_read_pipe(ssize_t result, int socket_fd, bool is_main);
    int assert_client_write_pipe(ssize_t result, int socket_fd, bool is_main);

    int prepare_client();
    void handle_client(int socket_fd);

    struct sockaddr_in server_address;

    thread interaction_thread;

    int client_read_pipe[2];
    int client_write_pipe[2];

    string host_name;
    int port_number;
    int ip_version;
    string seat;
    bool is_ai;

    queue<string> messages_to_send;
    vector<vector<string>> taken_tricks;
    int16_t trick_number;
    mutex messages_mutex;

    vector<string> my_cards;
    mutex printing_mutex;

    bool got_score;
    bool got_total;
};

#endif // KLIENT_H