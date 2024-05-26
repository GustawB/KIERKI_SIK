#ifndef SERWER_H
#define SERWER_H

#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <array>
#include <map>
#include <cinttypes>
#include <poll.h>
#include <initializer_list>
#include <algorithm>

#include "common.h"
#include "regex.h"
#include "senders.h"
#include "file_reader.h"
#include "points_calculator.h"
#include <sys/time.h>

using std::thread;
using std::mutex;
using std::array;
using std::string;
using std::vector;
using std::cout;
using std::map;
using std::find;
using std::initializer_list;

using poll_size = vector<struct pollfd>::size_type;

class Serwer
{
public:
    Serwer() = delete;
    Serwer(int port, int timeout, const std::string& game_file_name);
    ~Serwer();

    int start_game();
    int run_game();

private:
    void print_log(const struct sockaddr_in6& src_addr, const struct sockaddr_in6& dest_addr, const string& message);
    void print_error(const string& message);

    int run_deal(int32_t trick_type, const string& seat);

    int barrier();

    void handle_connections();
    int reserve_spot(int client_fd, string& seat, const struct sockaddr_in6& client_addr, bool& b_is_my_turn);
    int client_poll(int client_fd, const string& seat, const struct sockaddr_in6& client_addr, bool b_is_my_turn);

    void handle_client(int client_fd, struct sockaddr_in6 client_addr);

    void close_thread(const string& error_message, initializer_list<int> fds, const string& seat, bool b_was_occupying, bool b_was_ended_by_server);
    void close_fds(const std::string& error_message, initializer_list<int> fds);
    void close_fds(initializer_list<int> fds);

    int assert_client_read_socket(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_write_socket(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_read_pipe(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_write_pipe(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);

    int assert_server_read_pipe(ssize_t result);
    int assert_server_write_pipe(ssize_t result);

    int close_server(const string& error_message);

    int handle_disconnections();

    struct sockaddr_in6 server_address;

    mutex memory_mutex;
    mutex print_mutex;

    int port;
    int timeout;
    string game_file_name;

    thread connection_manager_thread;

    int occupied;
    map<string, int> seats_status;

    // 0 - read; 1 - write
    array<int[2], 5> server_read_pipes;
    array<int[2], 5> server_write_pipes;

    map<string, int> seats_to_array;

    string current_message;

    vector<string> cards_on_table;

    map<string, int32_t> round_scores;
    map<string, int32_t> total_scores;
    
    int32_t trick_number;

    array<vector<string>, 4> cards;
    array<vector<string>, 4> deal;
    vector<vector<string>> taken_tricks;
    vector<string> taken_takers;
    string deal_starter;
    int trick_type_global;
    string start_seat_global;

    string last_taker;

    string player_turn;

    int32_t waiting_on_barrier;

    int32_t working_threads;
};

#endif // SERWER_H