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

#include "common.h"
#include "regex.h"
#include "senders.h"
#include "retrievers.h"
#include "file_reader.h"
#include "points_calculator.h"

using std::thread;
using std::mutex;
using std::array;
using std::string;
using std::vector;
using std::cout;
using std::map;
using std::initializer_list;

using poll_size = vector<struct pollfd>::size_type;

class Serwer
{
public:
    Serwer() = delete;
    Serwer(int port, int timeout, const std::string& game_file_name);
    ~Serwer();

    void start_game();
    void run_game();

private:
    int run_deal(int32_t trick_type, const string& seat);

    int barrier(int16_t type);

    void handle_connections();
    int reserve_spot(int client_fd, string& seat);
    int client_poll(int client_fd, const string& seat);
    void handle_client(int client_fd);
    void close_thread(const string& error_message, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    void close_fds(const std::string& error_message, initializer_list<int> fds);
    void close_fds(initializer_list<int> fds);

    int assert_client_read_socket(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_write_socket(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_read_pipe(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);
    int assert_client_write_pipe(ssize_t result, initializer_list<int> fds, const string& seat, bool b_was_occupying);

    int assert_server_read_pipe(ssize_t result);
    int assert_server_write_pipe(ssize_t result);

    void close_server();

    int handle_disconnections();

    int port;
    int timeout;
    string game_file_name;

    thread connection_manager_thread;

    vector<thread> client_threads;
    mutex client_threads_mutex;

    int occupied;
    map<string, int> seats_status;
    mutex seats_mutex;

    // 0 - read; 1 - write
    array<int[2], 5> server_read_pipes;
    array<int[2], 5> server_write_pipes;

    map<string, int> array_mapping;

    string current_message;
    mutex current_message_mutex;

    vector<string> cards_on_table;
    mutex cards_on_table_mutex;

    map<string, int32_t> round_scores;
    map<string, int32_t> total_scores;
    mutex scores_mutex;

    string last_played_card;
    mutex last_played_card_mutex;

    array<vector<string>, 4> cards;
    mutex cards_mutex;

    string last_taker;
    mutex last_taker_mutex;

    int32_t waiting_on_barrier;
    mutex barrier_mutex;
};

#endif // SERWER_H