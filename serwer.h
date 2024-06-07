#ifndef SERWER_H
#define SERWER_H

#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <queue>
#include <mutex>
#include <array>
#include <map>
#include <cinttypes>
#include <poll.h>
#include <initializer_list>
#include <algorithm>
#include <signal.h>

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
using std::queue;
using std::cout;
using std::map;
using std::find;
using std::initializer_list;
using std::system_error;
using std::stoi;

using poll_size = vector<struct pollfd>::size_type;

class Serwer
{
public:
    Serwer() = delete;
    Serwer(int32_t port, int32_t timeout, const std::string& game_file_name);
    ~Serwer();

    /*
    * Starts the game by creating pipes and starting the connection thread.
    * Returns 0 if successful, 1 otherwise.
    */
    int16_t start_game();

    /*
    * Runs the game by handling connections and messages.
    * Returns 0 if successful, 1 otherwise.
    */
    int16_t run_game();

private:
    /*
    * Function that runs a logic for one deal.
    * Returns 0 if successful, -1 otherwise.
    */
    int16_t run_deal(int16_t trick_type, const string& seat);

    /*
    * Utility function to handle barriers; they occur
    * both when we need to synchronize everyone and when
    * we need to wait for a specific player to return.
    * Returns 0 if successful, 1 if card was received
    * during the barrier, -1 otherwise.
    */
    int16_t barrier();

    /*
    * FUnction used by connection_thread to handle incoming clients.
    */
    void handle_connections();

    /*
    * Used by the new client to check if the seat is available.
    * If so, it reserves it and returns 1, 0 if there is no seat,
    * -1 on error.
    */
    int16_t reserve_spot(int client_fd, string& seat,
        const struct sockaddr_in6& client_addr,
        bool& b_is_my_turn, bool& b_is_barrier);

    /*
    * Used by the client to check if he received a TRICK message.
    * Returns -1 on error otherwise 0.
    */
    int16_t parse_message(string& message, int32_t client_fd,
        const string& seat, const struct sockaddr_in6& client_addr,
        bool& b_was_destined_to_play, int16_t current_trick,
        int32_t& timeout_copy);

    /*
    * Used by the client thread to wathc for messages from the server
    * and main server thread. Returns -1 on error, 0 otherwise.
    */
    int16_t client_poll(int32_t client_fd, const string& seat,
        const struct sockaddr_in6& client_addr, bool b_is_my_turn,
        bool b_is_barrier);

    /*
    * Function called by client thread to run reserverd spot and 
    * client_poll if the first one was successful.
    */
    void handle_client(int32_t client_fd, struct sockaddr_in6 client_addr,
        uint64_t thread_id);

    /*
    * Utility function to close a thread (client or connection_handler,
    * depending on the arguments).
    */
    void close_thread(const string& error_message,
        const initializer_list<int>& fds, const string& seat,
        bool b_was_occupying, bool b_was_ended_by_server);

    /*
    * Utility function to close passed file descriptors on error
    * and print an error message.
    */
    void close_fds(const std::string& error_message,
        const initializer_list<int>& fds);

    /*
    * Utility function to close passed file descriptors.
    */
    void close_fds(const initializer_list<int32_t>& fds);

    /*
    * Utility to check if reading from socket succedeed. If not,
    * closes the thread that failed and returns -1.
    */
    int16_t assert_client_read_socket(ssize_t result,
        const initializer_list<int32_t>& fds, const string& seat,
        bool b_was_occupying);

    /*
    * Utility to check if writing to socket succedeed. If not,
    * closes the thread that failed and returns -1.
    */
    int16_t assert_client_write_socket(ssize_t result, ssize_t expected,
        const initializer_list<int32_t>& fds, const string& seat,
        bool b_was_occupying);

    /*
    * Utility to check if reading from pipe succedeed. If not,
    * closes the thread that failed and returns -1.
    */
    int16_t assert_client_read_pipe(ssize_t result,
        const initializer_list<int32_t>& fds, const string& seat,
        bool b_was_occupying);

    /*
    * Utility to check if writing to pipe succedeed. If not,
    * closes the thread that failed and returns -1.
    */  
    int16_t assert_client_write_pipe(ssize_t result,
        const initializer_list<int32_t>& fds, const string& seat,
        bool b_was_occupying);

    /*
    * Utility to check if reading from pipe succedeed. If not,
    * closes the server and returns -1.
    */
    int16_t assert_server_read_pipe(ssize_t result);

    /*
    * Utility to check if writing to pipe succedeed. If not,
    * closes the server and returns -1.
    */
    int16_t assert_server_write_pipe(ssize_t result);

    /*
    * Utility to close the server and print error if specified.
    * Return 0 if everything went smoothly, 1 otherwise.
    */
    int16_t close_server(const string& error_message);

    struct sockaddr_in6 server_address;

    uint64_t thread_id;
    map<uint64_t, thread> client_threads;
    vector<uint64_t> joinable_threads;

    mutex memory_mutex;
    mutex print_mutex;

    int32_t port;
    int32_t timeout;
    string game_file_name;

    thread connection_manager_thread;

    int16_t occupied;
    map<string, int32_t> seats_status;

    // 0 - read; 1 - write
    array<int32_t[2], 5> server_read_pipes;
    array<int32_t[2], 5> server_write_pipes;

    map<string, int16_t> seats_to_array;

    string current_message;

    vector<string> cards_on_table;

    map<string, int32_t> round_scores;
    map<string, int32_t> total_scores;
    
    int16_t trick_number;

    array<vector<string>, 4> cards;
    array<vector<string>, 4> deal;
    vector<vector<string>> taken_tricks;
    vector<string> taken_takers;
    string deal_starter;
    int16_t trick_type_global;
    string start_seat_global;

    string last_taker;

    string player_turn;

    int16_t waiting_on_barrier;
    bool b_is_barrier_ongoing;

    array<queue<string>, 4> barrier_messages;
};

#endif // SERWER_H