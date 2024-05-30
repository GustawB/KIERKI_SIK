#include "serwer.h"
#include <arpa/inet.h>
#include <netdb.h>

Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
    : server_address{}, thread_id{0}, client_threads{}, joinable_threads{}, memory_mutex{}, print_mutex{}, port{port}, timeout{timeout * 1000}, game_file_name{game_file_name},
    occupied{0}, seats_status{{"N", -1}, {"E", -1}, {"S", -1}, {"W", -1}},
    seats_to_array{{"N", 0}, {"E", 1}, {"S", 2}, {"W", 3}, {"K", 4}}, 
    current_message{}, cards_on_table{}, round_scores{{{"N", 0}, {"E", 0}, {"S", 0}, {"W", 0}}}, total_scores{{"N", 0}, {"E", 0}, {"S", 0}, {"W", 0}},
    trick_number{0}, cards{}, deal{}, taken_tricks{}, taken_takers{}, 
    last_taker{}, player_turn{"x"}, waiting_on_barrier{0}
    {}
    

Serwer::~Serwer() {}

void Serwer::print_log(const struct sockaddr_in6& src_addr, const struct sockaddr_in6& dest_addr, const string& message)
{
    print_mutex.lock();
    common::print_log(src_addr, dest_addr, message);
    print_mutex.unlock();
}

void Serwer::print_error(const string& message)
{
    print_mutex.lock();
    common::print_error(message);
    print_mutex.unlock();
}

void Serwer::close_fds(const std::string& error_message, initializer_list<int> fds)
{
    common::print_error(error_message);
    for (int fd : fds) { common::assert_close(fd); }
}

void Serwer::close_fds(initializer_list<int> fds)
{
    for (int fd : fds) { common::assert_close(fd); }
}

void Serwer::close_thread(const string& error_message,
initializer_list<int> fds, const string& seat, bool b_was_occupying, bool b_was_ended_by_server = false)
{
    if (error_message != "") { print_error(error_message); }
    if (b_was_occupying)
    {
        memory_mutex.lock();
        --occupied;
        if (occupied < 0) {occupied = 0;}
        // Basically, I'm protecting myself from the case when I'm closing everything
        // but one last connection manages to grab free seat. Now it will be like 
        // "Damn, seat is already taken, I'm outta here."
        if (!b_was_ended_by_server) {seats_status[seat] = -1;}
        memory_mutex.unlock();
    }
    if ((!b_was_ended_by_server) && (b_was_occupying || seat == CONNECTIONS_THREAD))
    {
        ssize_t pipe_write = common::write_to_pipe(server_read_pipes[seats_to_array[seat]][1], DISCONNECTED);
        if (pipe_write != 1) { print_error("Failed to notify server."); }
    }
    for (int fd : fds) { common::assert_close(fd); }
}

int Serwer::assert_client_read_socket(ssize_t result,
initializer_list<int> fds, const string& seat, bool b_was_occupying)
{
    if (result == 0)
    {
        // Client disconnected.
        close_thread("Client disconnected.", fds, seat, b_was_occupying);
        return -1;
    }
    else if (result < 0)
    {
        // Error.
        close_thread("Failed to read from client.", fds, seat, b_was_occupying);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_write_socket(ssize_t result,
initializer_list<int> fds, const string& seat, bool b_was_occupying)
{
    if (result <= 0)
    {
        close_thread("Failed to send message to the client.", fds, seat, b_was_occupying);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_read_pipe(ssize_t result, initializer_list<int> fds,
const string& seat, bool b_was_occupying)
{
    if (result == 0)
    {
        // Server disconnected. Shouldn't happen but whatever, close connections.
        close_thread("Server disconnected (pipe closed).", fds, seat, b_was_occupying);
        return -1;
    }
    else if (result < 0)
    {
        // Error communicating with the server.
        close_thread("Failed to read from server pipe.", fds, seat, b_was_occupying);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_write_pipe(ssize_t result, initializer_list<int> fds,
const string& seat, bool b_was_occupying)
{
    if (result != 1)
    {
        close_thread("Failed to notify server.", fds, seat, b_was_occupying);
        return -1;
    }
    return 0;
}

int Serwer::assert_server_read_pipe(ssize_t result)
{
    if (result == 0)
    {
        close_server("Thread closed its pipes (illegal).");
        return 0;
    }
    else if (result < 0)
    {
        close_server("Failed to read from server thread.");
        return -1;
    }
    return 1;
}

int Serwer::assert_server_write_pipe(ssize_t result)
{
    if (result != 1)
    {
        close_server("Failed to notify thread.");
        return -1;
    }
    return 0;
}

int Serwer::close_server(const string& error_message = "")
{
    bool b_did_something_fail = false;
    try 
    {
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[4][1], END);
        if (pipe_write != 1) {std::runtime_error("Failed to close connection thread.");}
        
        try { connection_manager_thread.join(); }
        catch (const std::system_error& e) 
        {
            print_error(e.what());
            b_did_something_fail = true;
        }
    }
    catch (const std::exception& e) 
    { 
        print_error(e.what()); 
        b_did_something_fail = true;
    }

    for (int i = 0; i < 4; ++i)
    {
        // Send close message to threads.
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], DISCONNECTED);
        if (pipe_write != 1) 
        {
            print_error("Failed to notify thread on server close.");
            b_did_something_fail = true;
        }
    }

    for (auto iter = client_threads.begin(); iter != client_threads.end(); ++iter)
    {
        try { iter->second.join(); }
        catch (const std::system_error& e) 
        {
            print_error(e.what());
            b_did_something_fail = true;
        }
        cout << "Thread " << iter->first << " joined.\n";
    }

    for (int i = 0; i < 5; ++i)
    {
        close_fds({server_read_pipes[i][0], server_read_pipes[i][1], server_write_pipes[i][0], server_write_pipes[i][1]});
    }

    if (error_message != "") 
    {
        print_error(error_message);
        b_did_something_fail = true;
    }
    return b_did_something_fail;
}

int Serwer::barrier()
{
    int32_t waiting = 0;
    memory_mutex.lock();
    occupied = 0;
    waiting = 4 - occupied;
    memory_mutex.unlock();

    for (int i = 0; i < 4; ++i)
    {
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], BARRIER_RESPONSE);
        if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
    }

    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }

    bool b_received_card = false;

    while(waiting > 0)
    {
        for (int i = 0; i < 5; ++i) {poll_descriptors[i].revents = 0;}
        // Wait for clients to join.
        int poll_result = poll(&poll_descriptors[0], 5, -1);
        if (poll_result <= 0)
        {
            close_server("Failed to poll while waiting for clients.");
            return -1;
        }
        else
        {
            for (int i = 0; i < 5; ++i)
            {
                if (poll_descriptors[i].revents & POLLIN)
                {
                    string wake_msg;
                    ssize_t pipe_read = common::read_from_pipe(server_read_pipes[i][0], wake_msg);
                    if (assert_server_read_pipe(pipe_read) <= 0) {return -1;}
                    if (wake_msg == DISCONNECTED && i == 4)
                    {
                        // Connection thread run away.
                        connection_manager_thread.join();
                        connection_manager_thread = thread(&Serwer::handle_connections, this);
                    }
                    else if (wake_msg == DISCONNECTED)
                    {
                        // We want a new client to be able to participate in the barrier.
                        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], BARRIER_RESPONSE);
                        if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
                    }
                    else if (wake_msg == CARD_PLAY)
                    {
                        // Client played a card.
                        b_received_card = true;
                    }
                    else if (wake_msg != BARRIER_RESPONSE && wake_msg != DISCONNECTED) 
                    {
                        // Invalid message.
                        close_server("Invalid message in barrier poll.");
                        return -1;
                    }
                }
            }
        }
        memory_mutex.lock();
        waiting = 4 - occupied;
        memory_mutex.unlock();
    }

    return b_received_card;
}

int Serwer::start_game()
{
    for (int i = 0; i < 5; ++i)
    {
        if (pipe(server_read_pipes[i]) < 0)
        {
            print_error("Failed to create pipes.");
            for (int j = 0; j < i; ++j)
            {
                close_fds({server_read_pipes[j][0], server_read_pipes[j][1], server_write_pipes[j][0], server_write_pipes[j][1]});
            }
            return 1;
        }
        else if (pipe(server_write_pipes[i]) < 0)
        {
            print_error("Failed to create pipes.");
            close_fds({server_read_pipes[i][0], server_read_pipes[i][1]});
            for (int j = 0; j < i; ++j)
            {
                close_fds({server_read_pipes[j][0], server_read_pipes[j][1], server_write_pipes[j][0], server_write_pipes[j][1]});
            }
            return 1;
        }
    }

    connection_manager_thread = thread(&Serwer::handle_connections, this);

    if (barrier() < 0) {return 1;}
    return 0;
}


int Serwer::run_deal(int32_t trick_type, const string& seat)
{
    memory_mutex.lock();
    trick_type_global = trick_type;
    start_seat_global = seat;
    last_taker = seat;
    trick_number = 0;
    memory_mutex.unlock();

    // Send DEAL
    for (int i = 0; i < 4; ++i)
    {
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], DEAL);
        if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
    }

    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }
    array<string, 4> seats = {"N", "E", "S", "W"};
    map<string, int32_t> scores{{"N", 0}, {"E", 0}, {"S", 0}, {"W", 0}};
    for (int i = 0; i < 13; ++i)
    {
        memory_mutex.lock();
        cards_on_table.clear();
        ++trick_number;
        int beginning = seats_to_array[last_taker];

        // Join joinable threads.
        for (uint64_t id : joinable_threads)
        {
            // find in in map
            auto iter = client_threads.find(id);
            if (iter != client_threads.end())
            {
                client_threads[id].join();
                client_threads.erase(iter);
                joinable_threads.erase(find(joinable_threads.begin(), joinable_threads.end(), id));
                cout << "Thread " << id << " joined.\n";
            }
        }
        memory_mutex.unlock();
        for (int i = 0; i < 4; ++i)
        {
            memory_mutex.lock();
            player_turn = seats[(beginning + i) % 4];
            memory_mutex.unlock();
            ssize_t pipe_write = common::write_to_pipe(server_write_pipes[(beginning + i) % 4][1], CARD_PLAY);
            if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
            bool b_received_card = false;
            while (!b_received_card)
            {
                for (int j = 0; j < 5; ++j) { poll_descriptors[j].revents = 0; }
                int poll_result = poll(poll_descriptors, 5, -1);
                if (poll_result <= 0)
                {
                    close_server("Failed to poll while waiting for a card.");
                    return -1;
                }
                else
                {
                    for (int j = 0; j < 5; ++j)
                    {
                        if (poll_descriptors[j].revents & POLLIN)
                        {
                            string thread_message;
                            ssize_t pipe_read = common::read_from_pipe(server_read_pipes[j][0], thread_message);
                            if (assert_server_read_pipe(pipe_read) < 0) {return -1;}
                            if (thread_message == CARD_PLAY && j == (beginning + i) % 4)
                            {
                                // Client played a card.
                                b_received_card = true;
                            }
                            else if (thread_message == DISCONNECTED)
                            {
                                // Thread run away.
                                if (j == 4) 
                                {
                                    // Connection thread. Recreate the fucker.
                                    connection_manager_thread.join();
                                    connection_manager_thread = thread(&Serwer::handle_connections, this);
                                }
                                else
                                {
                                    // Wait for threads.
                                    int result = barrier();
                                    if (result < 0) {return -1;}
                                    else if (result > 0) {b_received_card = true;}
                                }
                            }
                            else
                            {
                                // Invalid message.
                                cout << "Message: " << thread_message << '\n';
                                close_server("Invalid message in main game server poll from client thread.");
                                return -1;
                            }
                        }
                    }
                }
            }
        }

        // Got four cards.
        memory_mutex.lock();
        string queue;
        PointsCalculator calculator(cards_on_table, seats[beginning], trick_type, i + 1);
        pair<string, int32_t> result = calculator.calculate_points();
        last_taker = result.first;
        scores[result.first] += result.second;
        taken_tricks.push_back({cards_on_table});
        taken_takers.push_back(result.first);
        memory_mutex.unlock();
        for (int i = 0; i < 4; ++i)
        {
            ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], TAKEN);
            if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
        }

        // Barrier.
        if (barrier() < 0) {return -1;}
    }

    // End of the deal.
    memory_mutex.lock();
    for (const auto& [key, value] : scores) { total_scores[key] += value; }
    for (const auto& [key, value] : scores) { round_scores[key] = value; }
    memory_mutex.unlock();
    for (int i = 0; i < 4; ++i)
    {
        string message = SCORES;
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], message.data());
        if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
    }

    if (barrier() < 0) {return -1;}
    return 0;
}

int Serwer::run_game()
{
    // Here we should have all 4 clients.
    FileReader fr(game_file_name);
    // First operation after being waken up should run normally.
    while (fr.read_next_deal() > 0) 
    {
        int16_t trick_type = fr.get_trick_type();
        string starting_seat = fr.get_seat();
        array<string, 4> raw_cards = fr.get_cards();
        memory_mutex.lock();
        deal_starter = starting_seat;
        taken_tricks.clear();
        taken_takers.clear();
        for (int i = 0; i < 4; ++i) 
        { 
            cards[i] = regex::extract_cards(raw_cards[i]); 
            deal[i] = regex::extract_cards(raw_cards[i]);
        }
        memory_mutex.unlock();
        if (run_deal(trick_type, starting_seat) < 0) {return 1;}
    }

    return close_server();
}

void Serwer::handle_connections()
{
    // Create a socket.
    int socket_fd = common::setup_server_socket(port, QUEUE_SIZE, server_address);
    if (socket_fd < 0) {
        string server_message = DISCONNECTED;
        ssize_t pipe_write = common::write_to_pipe(server_read_pipes[4][1], server_message.data());
        if (pipe_write != 1) { print_error("Failed to notify server."); }
        return;
    }

    // One fd for server, one for new connection.
    struct pollfd poll_descriptors[2];
    poll_descriptors[0].fd = socket_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = server_write_pipes[4][0];
    poll_descriptors[1].events = POLLIN;

    for (;;) 
    {
        // Reset the revents.
        poll_descriptors[0].revents = 0;
        poll_descriptors[1].revents = 0;

        int poll_result = poll(&poll_descriptors[0], 2, -1);
        if (poll_result <= 0)
        {
            // Poll failed (we don't expect timeout here).
            close_thread("Failed to poll.", {socket_fd}, CONNECTIONS_THREAD, false);
            return;
        }
        else
        {
            // Handle the new connection.
            if (poll_descriptors[0].revents & POLLIN)
            {
                struct sockaddr_in6 client_address;
                int client_fd = common::accept_client(socket_fd, client_address);
                if (client_fd < 0) { close_thread("Failed to accept connection.", {socket_fd}, CONNECTIONS_THREAD, false); }
                else
                {
                    memory_mutex.lock();
                    try 
                    {
                        thread client_thread(&Serwer::handle_client, this, client_fd, client_address, thread_id);
                        client_threads[thread_id] = move(client_thread); 
                    }
                    catch (const std::system_error& e) 
                    {
                        memory_mutex.unlock();
                        close_thread(e.what(), {socket_fd, client_fd}, CONNECTIONS_THREAD, false);
                        return;
                    }
                    ++thread_id;
                    memory_mutex.unlock();
                    cout << "Thread " << thread_id << " created.\n";
                }
            }

            // Handle the server.
            if (poll_descriptors[1].revents & POLLIN)
            {
                string server_message;
                ssize_t pipe_read = common::read_from_pipe(server_write_pipes[4][0], server_message);
                if (assert_client_read_pipe(pipe_read, {socket_fd}, CONNECTIONS_THREAD, false) < 0) { return; }
                if (server_message == DISCONNECTED || server_message == END)
                {
                    // Server wants to close the connection.
                    close_fds({socket_fd});
                    return;
                }
                else
                {
                    // Server send invalid message.
                    close_fds("Server send invalid message.", {socket_fd});
                    return;
                }
            }
        }
    }
}

int Serwer::reserve_spot(int client_fd, string& seat, const struct sockaddr_in6& client_addr, bool& b_is_my_turn)
{
    cout << "Reserving spot.\n";
    // Read the message from the client.
    string message;
    struct timeval timeout_val;

    timeout_val.tv_sec = timeout / 1000;
    timeout_val.tv_usec = timeout % 1000;
    
    if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val)) < 0) 
    {
        close_thread("Failed to set timeout.", {client_fd}, seat, false);
        return -1;
    }
    ssize_t socket_read = common::read_from_socket(client_fd, message);
    if (assert_client_read_socket(socket_read, {client_fd}, seat, false) < 0) {return -1;}
    print_log(client_addr, server_address, message);

    // Turn off the timeout.
    timeout_val.tv_sec = 0;
    timeout_val.tv_usec = 0;
    if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val)) < 0) 
    {
        close_thread("Failed to disable timeout.", {client_fd}, seat, false);
        return -1;
    }
    
    if (regex::IAM_check(message))
    {
        seat = message.substr(3, 1);
        memory_mutex.lock();
        if (seats_status[seat] == -1) 
        {
            seats_status[seat] = client_fd;
            array<vector<string>, 4> deal_loc{deal};
            int trick_type_loc = trick_type_global;
            string deal_starter_loc = deal_starter;
            string last_taker_loc = last_taker;

            memory_mutex.unlock();

            // Send data from the game.
            string msg;
            if (deal_loc[0].size() != 0)
            {
                socket_read = senders::send_deal(client_fd, trick_type_loc, deal_starter_loc, deal[seats_to_array[seat]] , msg);
                print_log(server_address, client_addr, msg);
                if (assert_client_write_socket(socket_read, {client_fd}, seat, false) < 0) {return -1;}
                memory_mutex.lock();
                auto trick_iter = taken_tricks.begin();
                auto taker_iter = taken_takers.begin();
                for (uint32_t i = 0; i < taken_tricks.size(); ++i)
                {
                    memory_mutex.unlock();
                    socket_read = senders::send_taken(client_fd, i+1, *trick_iter, *taker_iter, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_read, {client_fd}, seat, false) < 0) {return -1;}
                    memory_mutex.lock();
                    ++trick_iter;
                    ++taker_iter;
                }
                if (player_turn == seat)
                {
                    b_is_my_turn = true;
                    int trick_nr_loc = trick_number;
                    vector<string> cards_on_table_loc{cards_on_table};
                    memory_mutex.unlock();
                    socket_read = senders::send_trick(client_fd, trick_nr_loc, cards_on_table_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_read, {client_fd}, seat, false) < 0) {return -1;}
                }
                else {memory_mutex.unlock();}
            }
        }
        else
        {
            string occupied_seats;
            for (auto& [key, value] : seats_status)
            {
                if (value != -1) {occupied_seats += key;}
            }
            memory_mutex.unlock();
            string msg;
            ssize_t socket_write = senders::send_busy(client_fd, occupied_seats, msg);
            print_log(server_address, client_addr, msg);

            if (assert_client_write_socket(socket_write, {client_fd}, seat, false) < 0) { return -1; }
            else 
            {
                close_fds({client_fd});
                return 0;
            }
        }
    }
    else
    {
        // Invalid message, close the connection.
        close_fds("Client send invalid message.", {client_fd});
        return -1;
    }

    return 1;
}

int Serwer::client_poll(int client_fd, const string& seat, const struct sockaddr_in6& client_addr, bool b_is_my_turn)
{
    ssize_t pipe_read = -1;
    ssize_t socket_read = -1;
    ssize_t pipe_write = -1;
    ssize_t socket_write = -1;
    std::array<struct pollfd, 2> poll_descriptors{};
    poll_descriptors[0].fd = client_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = server_write_pipes[seats_to_array[seat]][0];
    poll_descriptors[1].events = POLLIN;
    int timeout_copy = timeout;

    bool b_was_destined_to_play = false;
    if (b_is_my_turn) {b_was_destined_to_play = true;}

    // Read messages from the client.
    for(;;)
    {
        // Reset the revents.
        poll_descriptors[0].revents = 0;
        poll_descriptors[1].revents = 0;

        memory_mutex.lock();
        int16_t current_trick = trick_number;
        memory_mutex.unlock();
        int poll_result = -1;
        struct timeval start, end;
        if (!b_was_destined_to_play) { poll_result = poll(&poll_descriptors[0], 2, -1); }
        else 
        { 
            gettimeofday(&start, NULL);
            poll_result = poll(&poll_descriptors[0], 2, timeout_copy);
        }
        if (poll_result == 0 && b_was_destined_to_play)
        { // Timeout.
            string msg;
            memory_mutex.lock();
            vector<string> cards_on_table_loc{cards_on_table};
            timeout_copy = timeout;
            memory_mutex.unlock();
            socket_write = senders::send_trick(client_fd, current_trick, cards_on_table_loc, msg);
            print_log(server_address, client_addr, msg);
            if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
        }
        else if (poll_result < 0)
        {
            close_thread("Failed to poll.", {client_fd}, seat, true);
            return -1;  
        }
        else
        {
            gettimeofday(&end, NULL);
            if (poll_descriptors[0].revents & POLLIN)
            { // Client sent a message.
                string client_message;
                socket_read = common::read_from_socket(client_fd, client_message);
                print_log(client_addr, server_address, client_message);
                if (assert_client_read_socket(socket_read, {client_fd}, seat, true) < 0) {return -1;}
                if (regex::TRICK_client_check(client_message, current_trick))
                {
                    if (b_was_destined_to_play)
                    {
                        // Set current message;
                        memory_mutex.lock();
                        timeout_copy = timeout;
                        if (current_trick < 10) {client_message = client_message.substr(6, client_message.size() - 8);}
                        else {client_message = client_message.substr(7, client_message.size() - 9);}
                        // Check if the client has the card.
                        auto received_card = find(cards[seats_to_array[seat]].begin(), cards[seats_to_array[seat]].end(), client_message);
                        bool b_played_right_color = true;
                        if(cards_on_table.size() > 0)
                        {
                            char main_color = cards_on_table[0][cards_on_table[0].size() - 1];
                            b_played_right_color = (main_color == client_message[client_message.size() - 1]);
                            if (!b_played_right_color) 
                            {
                                b_played_right_color = true;
                                // Didn't play the right color. Check if he had it.
                                for (const string& card : cards[seats_to_array[seat]])
                                {
                                    if (card[card.size() - 1] == main_color)
                                    {
                                        b_played_right_color = false;
                                        break;
                                    }
                                }
                            }
                        }

                        if (received_card == cards[seats_to_array[seat]].end() || !b_played_right_color)
                        {
                            if (received_card == cards[seats_to_array[seat]].end()) {print_error("Client send a card he didn't have.");}
                            else {print_error("Client send a card with a wrong color.");}
                            memory_mutex.unlock();
                            // Client send something he didn't have; send back wrong.
                            string msg;
                            socket_write = senders::send_wrong(client_fd, current_trick, msg);
                            print_log(server_address, client_addr, msg);
                            if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                        }
                        else
                        {
                            // We received a valid card. Noice.
                            cards[seats_to_array[seat]].erase(received_card);
                            cards_on_table.push_back(client_message);
                            player_turn = "x";
                            memory_mutex.unlock();
                            // Notify server that the client played a card.
                            pipe_write = common::write_to_pipe(server_read_pipes[seats_to_array[seat]][1], CARD_PLAY);
                            if (assert_client_write_pipe(pipe_write, {client_fd}, seat, true) < 0) {return -1;}
                            b_was_destined_to_play = false;
                        }
                    }
                    else
                    {
                        // Client send a message out of order.
                        string msg;
                        socket_write = senders::send_wrong(client_fd, current_trick, msg);
                        print_log(server_address, client_addr, msg);
                        if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                    }
                    
                }
                else
                {
                    // Invalid message, close the connection.
                    close_thread("Client send invalid message.", {client_fd}, seat, true);
                    return -1;
                }
            }
            else if (b_was_destined_to_play)
            {
                // Update timeout; if <= 0, resend a request for a card.
                int passed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
                timeout_copy -= passed_ms;
                if (timeout_copy <= 0)
                {
                    string msg;
                    memory_mutex.lock();
                    vector<string> cards_on_table_loc{cards_on_table};
                    timeout_copy = timeout;
                    memory_mutex.unlock();
                    socket_write = senders::send_trick(client_fd, current_trick, cards_on_table_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                }
            }
            
            if (poll_descriptors[1].revents & POLLIN)
            { // Server sent a message.
                string server_message;
                pipe_read = common::read_from_pipe(poll_descriptors[1].fd, server_message);
                if (assert_client_read_pipe(pipe_read, {client_fd}, seat, true) < 0) {return -1;}

                if (server_message == CARD_PLAY)
                {
                    // Server wants the client to play a card.
                    string msg;
                    memory_mutex.lock();
                    vector<string> cards_on_table_loc{cards_on_table};
                    current_trick = trick_number;
                    memory_mutex.unlock();
                    socket_write = senders::send_trick(client_fd, current_trick, cards_on_table_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                    b_was_destined_to_play = true;
                }
                else if(server_message == DEAL)
                {
                    // Server wants the client to play a deal.
                    string msg;
                    memory_mutex.lock();
                    vector<string> cards_loc{cards[seats_to_array[seat]]};
                    string seat_loc = last_taker;
                    int trick_loc = trick_type_global;
                    memory_mutex.unlock();
                    socket_write = senders::send_deal(client_fd, trick_loc, seat_loc, cards_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                }
                else if (server_message == DISCONNECTED)
                {
                    // Server wants the client to disconnect.
                    close_thread("", {client_fd}, seat, true, true);
                    return 0;
                }
                else if (server_message == TAKEN)
                {
                    // Server wants the client to send "TAKEN".
                    memory_mutex.lock();
                    string taker_loc{last_taker};
                    vector<string> cards_on_table_loc{cards_on_table};
                    memory_mutex.unlock();
                    string msg;
                    socket_write = senders::send_taken(client_fd, current_trick, cards_on_table_loc, taker_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                }
                else if(server_message == SCORES)
                {
                    memory_mutex.lock();
                    map<string, int32_t> round_scores_loc{round_scores};
                    map<string, int32_t> total_scores_loc{total_scores};
                    memory_mutex.unlock();

                    string msg;
                    // Score.
                    socket_write = senders::send_score(client_fd, round_scores_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}

                    // Total score.
                    socket_write = senders::send_total(client_fd, total_scores_loc, msg);
                    print_log(server_address, client_addr, msg);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                }
                else if (server_message == BARRIER_RESPONSE)
                {
                    // Barrier response.
                    memory_mutex.lock();
                    ++occupied;
                    int local_occ = occupied;
                    memory_mutex.unlock();
                    if (local_occ == 4)
                    {
                        pipe_write = common::write_to_pipe(server_read_pipes[seats_to_array[seat]][1], BARRIER_RESPONSE);
                        if (assert_client_write_pipe(pipe_write, {client_fd}, seat, true) < 0) {return -1;}
                    }
                }
                else
                {
                    // Invalid message, close the connection.
                    close_thread("Server send invalid message.", {client_fd}, seat, true);
                    return -1;
                }
            }
        }
    }

    return 1;
}

void Serwer::handle_client(int client_fd, struct sockaddr_in6 client_addr, uint64_t thread_id)
{
    string seat;
    bool b_is_my_turn = false;
    // Reserve a spot at the table.
    cout << "Hello. I'm thread " << thread_id << ".\n";
    if (reserve_spot(client_fd, seat, client_addr, b_is_my_turn) > 0) 
    {   
        client_poll(client_fd, seat, client_addr, b_is_my_turn);
    }
    memory_mutex.lock();
    joinable_threads.push_back(thread_id);
    memory_mutex.unlock();
}