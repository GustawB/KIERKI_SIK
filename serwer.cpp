#include "serwer.h"
#include <fcntl.h>

Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
    : port{port}, timeout{timeout}, game_file_name{game_file_name}, 
    client_threads{}, client_threads_mutex{}, occupied{0}, seats_status{{"N", -1}, {"E", -1}, {"S", -1}, {"W", -1}},
    seats_mutex{}, array_mapping{{"N", 0}, {"E", 1}, {"S", 2}, {"W", 3}, {"K", 4}}, 
    current_message{}, current_message_mutex{}, cards_on_table{}, cards_on_table_mutex{}, round_scores{}, total_scores{}, scores_mutex{},
    last_played_card{}, last_played_card_mutex{}, cards{}, cards_mutex{}, last_taker{}, last_taker_mutex{},
    waiting_on_barrier{0}, barrier_mutex{}
    {}
    

Serwer::~Serwer()
{
    connection_manager_thread.join();
}

void Serwer::close_fds(const std::string& error_message, initializer_list<int> fds)
{
    common::print_error(error_message);
    for (int fd : fds) { close(fd); }
}

void Serwer::close_fds(initializer_list<int> fds)
{
    for (int fd : fds) { close(fd); }
}

void Serwer::close_thread(const string& error_message,
initializer_list<int> fds, const string& seat, bool b_was_occupying)
{
    common::print_error(error_message);
    string server_message = DISCONNECTED;
    if (b_was_occupying)
    {
        seats_mutex.lock();
        --occupied;
        seats_status[seat] = -1;
        seats_mutex.unlock();
    }
    if (b_was_occupying || seat == CONNECTIONS_THREAD)
    {
        ssize_t pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
        if (pipe_write != 1) {common::print_error("Failed to notify server.");}
    }
    for (int fd : fds) { close(fd); }
    close_fds({server_read_pipes[array_mapping[seat]][1], server_write_pipes[array_mapping[seat]][0]});
}

int Serwer::assert_client_read_socket(ssize_t result,
initializer_list<int> fds, const string& seat, bool b_was_occupying)
{
    string server_message = DISCONNECTED;
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

int Serwer::assert_client_read_pipe(ssize_t result, initializer_list<int> fds,
const string& seat, bool b_was_occupying)
{
    if (result == 0)
    {
        // Server disconnected. Shouldn't happen but whatever, close connections.
        close_thread("Server disconnected.", fds, seat, b_was_occupying);
        return -1;
    }
    else if (result < 0)
    {
        // Error communicating with the server.
        close_thread("Failed to read from server.", fds, seat, b_was_occupying);
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

int Serwer::assert_server_read_pipe(ssize_t result)
{
    if (result == 0)
    {
        common::print_error("Thread disconnected.");
        close_server();
        return 0;
    }
    else if (result < 0)
    {
        common::print_error("Failed to read from server.");
        close_server();
        return -1;
    }
    return 1;
}

int Serwer::assert_server_write_pipe(ssize_t result)
{
    if (result != 1)
    {
        common::print_error("Failed to notify thread.");
        close_server();
        return -1;
    }
    return 0;
}

void Serwer::close_server()
{
    for (int i = 0; i < 5; ++i)
    {
        // Send close message to threads.
        string server_message = "c";
        ssize_t pipe_write = common::write_to_pipe(server_read_pipes[i][1], server_message.data());
        if (pipe_write != 1) {common::print_error("Failed to notify thread on server close.");}
    }

    // Join client threads.
    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 4; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }

    int poll_result = poll(poll_descriptors, 4, -1);
    if (poll_result <= 0) 
    {
        for (int i = 0; i < 4; ++i) 
        {
            // Close my ends of pipes.
            close(server_read_pipes[i][0]);
            close(server_write_pipes[i][1]);
            common::print_error("Failed to poll on server exit.");
        }
    }
    else
    {
        for (int i = 0; i < 4; ++i)
        {
            if (poll_descriptors[i].revents & POLLIN)
            {
                // Server woken up by a client.
                break;
            }
        }
        // Close my ends of pipes.
        for (int i = 0; i < 4; ++i) 
        {
            // Close my ends of pipes.
            close(server_read_pipes[i][0]);
            close(server_write_pipes[i][1]);
        }
    }

    // Await connection thread.
    try 
    {
        connection_manager_thread.join();
    }
    catch (const std::system_error& e)
    {
        common::print_error("Failed to join connection manager thread.");
    }

    // Close the connection_thread-related pipes socket.
    close(server_read_pipes[4][0]);
    close(server_write_pipes[4][1]);
}

int Serwer::barrier(int16_t type)
{
    int32_t waiting = 0;
    seats_mutex.lock();
    if (type == MISSING_CLIENT_BARRIER) { waiting = 4 - occupied; }
    else if (type == END_OF_TRICK_BARRIER) { waiting = waiting_on_barrier; }
    seats_mutex.unlock();

    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }

    while(waiting > 0)
    {
        for (int i = 0; i < 5; ++i) {poll_descriptors[i].revents = 0;}
        // Wait for clients to join.
        int poll_result = poll(&poll_descriptors[0], 5, -1);
        if (poll_result <= 0)
        {
            common::print_error("Failed to poll while waiting for clients.");
            close_server();
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
                    else if (wake_msg != BARRIER_RESPONSE && wake_msg != DISCONNECTED) 
                    {
                        // Invalid message.
                        common::print_error("Invalid message in barrier poll.");
                        close_server();
                        return -1;
                    }
                }
            }
        }
        seats_mutex.lock();
        if (type == MISSING_CLIENT_BARRIER) { waiting = 4 - occupied; }
        else if (type == END_OF_TRICK_BARRIER) { waiting = waiting_on_barrier; }
        seats_mutex.unlock();
    }

    return 0;
}

void Serwer::start_game()
{
    cout << "Starting the game\n";
    for (int i = 0; i < 5; ++i)
    {
        if (pipe(server_read_pipes[i]) < 0 || pipe(server_write_pipes[i]) < 0)
        {
            common::print_error("Failed to create pipes.");
            close_server();
            return;
        }
    }

    connection_manager_thread = thread(&Serwer::handle_connections, this);

    barrier(MISSING_CLIENT_BARRIER);
}


int Serwer::run_deal(int32_t trick_type, const string& seat)
{
    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }
    array<string, 4> seats = {"N", "E", "S", "W"};
    array<string, 4> played_this_round = {"", "", "", ""};
    map<string, int32_t> scores{{"N", 0}, {"E", 0}, {"S", 0}, {"W", 0}};
    int beginning = array_mapping[seat];
    for (int i = 0; i < 13; ++i)
    {
        for (int i = 0; i < 4; ++i)
        {
            string message = CARD_PLAY;
            ssize_t pipe_write = common::write_to_pipe(server_write_pipes[(beginning + i) % 4][1], message.data());
            if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
            bool b_received_card = false;
            while (!b_received_card)
            {
                for (int j = 0; j < 5; ++j) { poll_descriptors[j].revents = 0; }
                int poll_result = poll(poll_descriptors, 5, -1);
                if (poll_result <= 0)
                {
                    common::print_error("Failed to poll while waiting for a card.");
                    close_server();
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
                                last_played_card_mutex.lock();
                                played_this_round[(beginning + i) % 4] = last_played_card;
                                cards_on_table.push_back(last_played_card);
                                last_played_card_mutex.unlock();
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
                                    if (barrier(MISSING_CLIENT_BARRIER) < 0) {return -1;}
                                }
                            }
                            else
                            {
                                // Invalid message.
                                common::print_error("Invalid message in main game server poll from client thread.");
                                close_server();
                                return -1;
                            }
                        }
                    }
                }
            }
        }

        // Got four cards.
        PointsCalculator calculator(played_this_round, seats[beginning], trick_type, i + 1);
        pair<string, int16_t> result = calculator.calculate_points();
        last_taker_mutex.lock();
        last_taker = result.first;
        last_taker_mutex.unlock();
        scores[result.first] += result.second;
        last_taker = result.first;
        for (int i = 0; i < 4; ++i)
        {
            string message = TAKEN;
            ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], message.data());
            if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
        }

        // Barrier.
        if (barrier(END_OF_TRICK_BARRIER) < 0) {return -1;}
    }

    // End of the deal.
    scores_mutex.lock();
    for (const auto& [key, value] : scores) { total_scores[key] += value; }
    round_scores = scores;
    scores_mutex.unlock();
    for (int i = 0; i < 4; ++i)
    {
        string message = SCORES;
        ssize_t pipe_write = common::write_to_pipe(server_write_pipes[i][1], message.data());
        if (assert_server_write_pipe(pipe_write) < 0) {return -1;}
    }

    // Barrier.
    if (barrier(END_OF_TRICK_BARRIER) < 0) {return -1;}
    return 0;
}

void Serwer::run_game()
{
    // Here we should have all 4 clients.
    cout << "Running the game\n";
    file_reader::FileReader fr(game_file_name);
    cards_on_table_mutex.lock();
    cards_on_table.clear();
    cards_on_table_mutex.unlock();
    // First operation after being waken up should run normally.
    //bool b_there_are_four_players = true;
    //int32_t hand = 1;
    while (fr.read_next_deal() > 0) 
    {
        int16_t trick_type = fr.get_trick_type();
        string seat = fr.get_seat();
        array<string, 4> raw_cards = fr.get_cards();
        cards_mutex.lock();
        for (int i = 0; i < 4; ++i) { cards[i] = regex::extract_cards(raw_cards[i]); }
        cards_mutex.unlock();
        if (run_deal(trick_type, seat) < 0) {return;}
    }
    cout << "Game ended\n";
    close_server();
}

void Serwer::handle_connections()
{
    // Create a socket.
    int socket_fd = common::setup_server_socket(port, QUEUE_SIZE);
    if (socket_fd < 0) {
        string server_message = DISCONNECTED;
        ssize_t pipe_write = common::write_to_pipe(server_read_pipes[4][1], server_message.data());
        if (pipe_write != 1) {common::print_error("Failed to notify server.");}
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
                int client_fd = common::accept_client(socket_fd);
                if (client_fd < 0) 
                {
                    close_thread("Failed to accept connection.", {socket_fd}, CONNECTIONS_THREAD, false);
                }

                cout << "Accepted connection; fd: " << client_fd << "\n";
                thread client_thread(&Serwer::handle_client, this, client_fd);
                client_thread.detach();
            }

            // Handle the server.
            if (poll_descriptors[1].revents & POLLIN)
            {
                string server_message;
                ssize_t pipe_read = common::read_from_pipe(server_write_pipes[4][0], server_message);
                if (assert_client_read_pipe(pipe_read, {socket_fd}, CONNECTIONS_THREAD, false) < 0) { return; }
                if (server_message == DISCONNECTED)
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

int Serwer::reserve_spot(int client_fd, string& seat)
{
    // Read the message from the client.
    cout << "Handling client " << client_fd << "\n";
    string message;
    ssize_t socket_read = common::read_from_socket(client_fd, message);
    if (socket_read == 0) 
    {
        // Client disconnected. Here we don't have to worry about anything, end execution.
        close_fds({client_fd});
        return 0; 
    }
    else if (socket_read < 0)
    {
        close_fds("Failed to read IAM message.", {client_fd});
        return -1;
    }
    if (regex::IAM_check(message))
    {
        seat = message.substr(3, 1);
        int local_occ = -1;
        seats_mutex.lock();
        if (seats_status[seat] == -1) 
        {
            cout << "Seat " << seat << " is free\n";
            seats_status[seat] = client_fd;
            ++occupied;
            local_occ = occupied;
            seats_mutex.unlock();
            if (local_occ == 4)
            {
                string server_message = "b";
                // Notify the server that all seats are taken.
                ssize_t socket_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
                if (assert_client_write_pipe(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                cout << "All seats taken\n";
            }
        }
        else
        {
            cout << "Seat " << message << " is already taken\n";
            string occupied_seats;
            for (auto& [key, value] : seats_status)
            {
                if (value != -1) {occupied_seats += key;}
            }
            seats_mutex.unlock();
            ssize_t socket_write = senders::send_busy(client_fd, occupied_seats);
            if (assert_client_write_socket(socket_write, {client_fd}, seat, false) < 0) {return -1;}
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
        std::cout << "IAM bad message: " << message;
        close_fds("Client send invalid message.", {client_fd});
        return -1;
    }

    return 1;
}

int Serwer::client_poll(int client_fd, const string& seat)
{
    ssize_t pipe_read = -1;
    ssize_t socket_read = -1;
    ssize_t pipe_write = -1;
    ssize_t socket_write = -1;
    int16_t current_trick = 1;
    std::array<struct pollfd, 2> poll_descriptors{};
    poll_descriptors[0].fd = client_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = server_write_pipes[array_mapping[seat]][0];
    poll_descriptors[1].events = POLLIN;
    
    cout << poll_descriptors[0].fd << " " << poll_descriptors[1].fd << "\n";

    bool b_was_destined_to_play = false;

    // Read messages from the client.
    for(;;)
    {
        // Reset the revents.
        poll_descriptors[0].revents = 0;
        poll_descriptors[1].revents = 0;

        int poll_result = poll(&poll_descriptors[0], 2, -1);
        if (poll_result == 0 && b_was_destined_to_play)
        { // Timeout.
            socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
            if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
        }
        else if (poll_result < 0)
        {
            close_thread("Failed to poll.", {client_fd}, seat, true);
            return -1;  
        }
        else
        {
            if (poll_descriptors[0].revents & (POLLHUP | POLLIN))
            { // Client sent a message.
                cout << "Client " << seat << " sent a message\n";
                string client_message;
                socket_read = common::read_from_socket(client_fd, client_message);
                if (assert_client_read_socket(socket_read, {client_fd}, seat, true) < 0) {return -1;}
                if (regex::TRICK_client_check(client_message))
                {
                    cout << "Client " << seat << " played a card\n";
                    if (b_was_destined_to_play)
                    {
                        // Set current message;
                        current_message_mutex.lock();
                        current_message = client_message;
                        current_message_mutex.unlock();
                        // Notify server that the client played a card.
                        string server_message = "p";
                        pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
                        if (assert_client_write_pipe(pipe_write, {client_fd}, seat, true) < 0) {return -1;}
                        b_was_destined_to_play = false;
                    }
                    else
                    {
                        // Client send a message out of order.
                        socket_write = senders::send_wrong(client_fd, current_trick);
                        if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                    }
                    
                }
                else
                {
                    // Invalid message, close the connection.
                    std::cout << "trick bad message: " << client_message;
                    close_thread("Client send invalid message.", {client_fd}, seat, true);
                    return -1;
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
                    socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                    b_was_destined_to_play = true;
                }
                else if (server_message == DISCONNECTED)
                {
                    // Server wants the client to disconnect.
                    close_fds({client_fd});
                    return 0;
                }
                else if (server_message == TAKEN)
                {
                    // Server wants the client to send "TAKEN".
                    last_taker_mutex.lock();
                    string taker_loc{last_taker};
                    last_taker_mutex.unlock();
                    cards_on_table_mutex.lock();
                    vector<string> cards_on_table_loc{cards_on_table};
                    cards_on_table_mutex.unlock();
                    socket_write = senders::send_taken(client_fd, current_trick, cards_on_table_loc, taker_loc);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}

                    // Barrier response.
                    string barrier_response = BARRIER_RESPONSE;
                    pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], barrier_response.data());
                    if (assert_client_write_pipe(pipe_write, {client_fd}, seat, true) < 0) {return -1;}
                }
                else if(server_message == SCORES)
                {
                    scores_mutex.lock();
                    map<string, int32_t> round_scores_loc{round_scores};
                    map<string, int32_t> total_scores_loc{total_scores};
                    scores_mutex.unlock();

                    // Score.
                    socket_write = senders::send_score(client_fd, round_scores_loc);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}

                    // Total score.
                    socket_write = senders::send_total(client_fd, total_scores_loc);
                    if (assert_client_write_socket(socket_write, {client_fd}, seat, true) < 0) {return -1;}
                }
                else
                {
                    // Invalid message, close the connection.
                    std::cout << "Server bad message: " << server_message;
                    close_thread("Server send invalid message.", {client_fd}, seat, true);
                    return -1;
                }
            }
        }
    }

    return 1;
}

void Serwer::handle_client(int client_fd)
{
    string seat;
    // Reserve a spot at the table.
    if (reserve_spot(client_fd, seat) <= 0) { return; }
    client_poll(client_fd, seat);
}