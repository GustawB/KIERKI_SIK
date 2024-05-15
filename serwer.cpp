#include "serwer.h"

Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
    : port{port}, timeout{timeout}, game_file_name{game_file_name}, 
    client_threads{}, client_threads_mutex{}, seats_status{{"N", -1}, {"E", -1}, {"S", -1}, {"W", -1}},
    seats_mutex{}, array_mapping{{"N", 0}, {"E", 1}, {"S", 2}, {"W", 3}}, 
    current_message{}, current_message_mutex{}, cards_on_table{}, round_scores{}, total_scores{}
    {}
    

Serwer::~Serwer()
{
    connection_manager_thread.join();
}

void Serwer::run_game()
{
    // Here we should have all 4 clients.
    cout << "Running the game\n";
    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }

    file_reader::FileReader fr(game_file_name);

    // First operation after being waken up should run normally.
    bool b_there_are_four_players = true;
    while (fr.read_next_deal() > 0) 
    {
        //int16_t trick = fr.get_trick_type();
        string seat = fr.get_seat();
        array<string, 4> cards = fr.get_cards();
        array<string, 4> seats = {"N", "E", "S", "W"};
        int beginning = array_mapping[seat];
        for (int i = 0; i < 4; ++i)
        {
            string message = "s";
            common::write_to_socket(server_write_pipes[(beginning + i) % 4][1], message.data(), 1);
            string client_message;
            common::read_from_socket(server_read_pipes[(beginning + i) % 4][0], client_message);
            if (client_message == "c")
            {
                cout << "Client " << seats[(beginning + i) % 4] << " ran away\n";
                // client run away, we should stop the game.
            }
            else if (regex::TRICK_client_check(client_message))
            {
                cout << "Client " << seats[(beginning + i) % 4] << " played a card\n";
                // client played a card
            }
            else
            {
                cout << "Client " << seats[(beginning + i) % 4] << " failed to play a card\n";
                // pipe failed ffs.
            }
        }

        while (!b_there_are_four_players)
        {
            seats_mutex.lock();
            b_there_are_four_players = (occupied == 4);
            seats_mutex.unlock();
            if(!b_there_are_four_players)
            {
                int poll_result = poll(poll_descriptors, 5, -1);
                if (poll_result < 0)
                {
                    cout << "Failed to poll\n";
                    exit(1);
                }
                else if (poll_result == 0)
                {
                    cout << "De fuq?\n";
                }
                else
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        if (poll_descriptors[i].revents & POLLIN)
                        {
                            cout << "Server woken up by " << i << " thread\n";
                        }
                    }
                }
            }
        }

        
    }
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
initializer_list<int> fds, const string& seat)
{
    common::print_error(error_message);
    string server_message = "c";
    ssize_t pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
    if (pipe_write != 1) {common::print_error("Failed to notify server.");}
    for (int fd : fds) { close(fd); }
}

void Serwer::close_thread(const string& error_message, 
initializer_list<int> fds, int position)
{
    common::print_error(error_message);
    string server_message = "c";
    ssize_t pipe_write = common::write_to_pipe(server_read_pipes[position][1], server_message.data());
    if (pipe_write != 1) {common::print_error("Failed to notify server.");}
    for (int fd : fds) { close(fd); }
}

int Serwer::assert_client_read_socket(ssize_t result,
initializer_list<int> fds, const string& seat)
{
    string server_message = "c";
    if (result == 0)
    {
        // Client disconnected.
        close_thread("Client disconnected.", fds, seat);
        return -1;
    }
    else if (result < 0)
    {
        // Error.
        close_thread("Failed to read from client.", fds, seat);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_read_pipe(ssize_t result, initializer_list<int> fds)
{
    if (result == 0)
    {
        // Server disconnected. Shouldn't happen but whatever, close connections.
        close_fds("Server disconnected.", fds);
        return -1;
    }
    else if (result < 0)
    {
        // Error communicating with the server.
        close_fds("Failed to read from server.", fds);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_write_pipe(ssize_t result, initializer_list<int> fds)
{
    if (result != 1)
    {
        close_fds("Failed to notify server.", fds);
        return -1;
    }
    return 0;
}

int Serwer::assert_client_write_socket(ssize_t result,
initializer_list<int> fds, const string& seat)
{
    if (result <= 0)
    {
        close_thread("Failed to send message to the client.", fds, seat);
        return -1;
    }
    return 0;
}

void Serwer::close_server()
{
    for (int i = 0; i < 5; ++i)
    {
        common::
        // Should cause threads to get PIPE error.
        close(server_read_pipes[i][0]);
        // Should cause threads to get POLLHUP.
        close(server_write_pipes[i][1]);
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

void Serwer::start_game()
{
    cout << "Starting the game\n";
    for (int i = 0; i < 5; ++i)
    {
        if (pipe(server_read_pipes[i]) < 0 || pipe(server_write_pipes[i]) < 0)
        {
            common::print_error("Failed to create pipes.");
            --i;
            // Close previously created descriptors.
            while (i >= 0) 
            {
                close(server_read_pipes[i][0]);
                close(server_read_pipes[i][1]);
                close(server_write_pipes[i][0]);
                close(server_write_pipes[i][1]);
                --i;
            }
            return;
        }
    }

    cout << "Pipes created\n";

    struct pollfd poll_descriptors[5];
    for (int i = 0; i < 5; ++i)
    {
        poll_descriptors[i].fd = server_read_pipes[i][0];
        poll_descriptors[i].events = POLLIN;
    }

    cout << "Poll descriptors created\n";

    connection_manager_thread = thread(&Serwer::handle_connections, this);
    
    int poll_result = poll(poll_descriptors, 5, -1);
    if (poll_result <= 0)
    {
        cout << "Failed to poll\n";
        exit(1);
    }
    else if (poll_result == 0)
    {
        cout << "De fuq?\n";
    }
    else
    {
        for (int i = 0; i < 5; ++i)
        {
            if (poll_descriptors[i].revents & POLLIN)
            {
                cout << "Server woken up by " << i << " thread\n";
            }
        }
    }
}

void Serwer::handle_connections()
{
    // Create a socket.
    int socket_fd = common::setup_server_socket(port, QUEUE_SIZE);
    if (socket_fd < 0) {
        string server_message = "c";
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
            close_thread("Failed to poll.", {socket_fd}, 4);
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
                    close_thread("Failed to accept connection.", {socket_fd}, 4);
                }

                cout << "Accepted connection\n";
                thread client_thread(&Serwer::handle_client, this, client_fd);
                client_thread.detach();
            }

            // Handle the server.
            if (poll_descriptors[1].revents & POLLIN)
            {
                string server_message;
                ssize_t pipe_read = common::read_from_pipe(server_write_pipes[4][0], server_message);
                if (assert_client_read_pipe(pipe_read, {socket_fd}) < 0) { return; }
                if (server_message == "c")
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
                string server_message = "c";
                // Notify the server that all seats are taken.
                ssize_t socket_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
                if (assert_client_write_pipe(socket_write, {client_fd}) < 0) {return -1;}
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
            if (assert_client_write_socket(socket_write, {client_fd}, seat) < 0) {return -1;}
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

    close_fds({client_fd});
    return 1;
}

int Serwer::client_poll(int client_fd, const string& seat)
{
    ssize_t pipe_read = -1;
    ssize_t socket_read = -1;
    ssize_t pipe_write = -1;
    ssize_t socket_write = -1;
    int16_t current_trick = 1;
    struct pollfd poll_descriptors[2];
    poll_descriptors[0].fd = client_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = server_write_pipes[array_mapping[seat]][0];
    poll_descriptors[1].events = POLLIN;

    bool b_was_destined_to_play = false;

    // Read messages from the client.
    for(;;)
    {
        // Reset the revents.
        poll_descriptors[0].revents = 0;
        poll_descriptors[1].revents = 0;

        int poll_result = poll(&poll_descriptors[0], 2, 5000);
        if (poll_result == 0)
        { // Timeout.
            socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
            if (assert_client_write_socket(socket_write,
            {client_fd}, seat) < 0) {return -1;}
        }
        else if (poll_result < 0)
        {
            close_thread("Failed to poll.", {client_fd}, seat);
            return -1;  
        }
        else
        {
            if (poll_descriptors[0].revents & POLLIN)
            { // Client sent a message.
                string client_message;
                socket_read = common::read_from_socket(client_fd, client_message);
                if (assert_client_read_socket(socket_read,
                {client_fd}, seat) < 0) {return -1;}
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
                        if (assert_client_write_pipe(pipe_write, {client_fd}) < 0) {return -1;}
                    }
                    else
                    {
                        // Client send a message out of order.
                        socket_write = senders::send_wrong(client_fd, current_trick);
                        if (assert_client_write_socket(socket_write,
                        {client_fd}, seat) < 0) {return -1;}
                    }
                    
                }
                else
                {
                    // Invalid message, close the connection.
                    close_thread("Client send invalid message.",
                    {client_fd}, seat);
                    return -1;
                }
            }
            
            if (poll_descriptors[1].revents & POLLIN)
            { // Server sent a message.
                string server_message;
                pipe_read = common::read_from_pipe(poll_descriptors[1].fd, server_message);
                if (assert_client_read_pipe(pipe_read, {client_fd}) < 0) {return -1;}

                if (server_message == "p")
                {
                    // Server wants the client to play a card.
                    socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
                    if (assert_client_write_socket(socket_write,
                    {client_fd}, seat) < 0) {return -1;}
                    b_was_destined_to_play = true;
                }
                else if (server_message == "c")
                {
                    // Server wants the client to disconnect.
                    close_fds({client_fd});
                    return 0;
                }
                else if(server_message == "s")
                {
                    // Score.
                    socket_write = senders::send_score(client_fd, round_scores);
                    if (assert_client_write_socket(socket_write,
                    {client_fd}, seat) < 0) {return -1;}
                }
                else if(server_message == "t")
                {
                    // Total score.
                    socket_write = senders::send_total(client_fd, total_scores);
                    if (assert_client_write_socket(socket_write,
                    {client_fd}, seat) < 0) {return -1;}
                }
                else
                {
                    // Server send invalid message.
                    close_fds("Server send invalid message.", {client_fd});
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
    if (reserve_spot(client_fd, seat) <= 0) {return;}  
    client_poll(client_fd, seat);
}