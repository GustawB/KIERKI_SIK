#include "klient.h"

Klient::Klient(const string& host, int32_t port, int16_t ip,
    const string& seat_name, bool AI) : server_address{}, 
    server6_address{}, client_address{}, client6_address{}, host_name{host},
    port_number{port}, ip_version{ip}, seat{seat_name}, is_ai{AI}, 
    access_mutex{}, messages_to_send{}, taken_tricks{}, 
    played_cards{}, trick_number{1}, got_score{false}, got_total{false},
    expected_color{"none"}
    { signal(SIGPIPE, SIG_IGN); }

void Klient::print_logs(const string& msg, bool b_is_sender)
{
    if (b_is_sender)
    {
        if (ip_version == 4) { common::print_log(client_address,
            server_address, msg, access_mutex, is_ai); }
        else { common::print_log(client6_address, server6_address,
            msg, access_mutex, is_ai); }
    }
    else
    {
        if (ip_version == 4) { common::print_log(server_address,
            client_address, msg, access_mutex, is_ai); }
        else { common::print_log(server6_address, client6_address,
            msg, access_mutex, is_ai); }
    }
}

void Klient::close_pipe_sockets()
{
    common::assert_close(client_read_pipe[0]);
    common::assert_close(client_read_pipe[1]);
    common::assert_close(client_write_pipe[0]);
    common::assert_close(client_write_pipe[1]);
}

void Klient::close_main_sockets(ssize_t result,
    const string& error_message = "")
{
    if (error_message != "") { common::print_error(error_message); }
    // Close my ends of pipes.
    if (result == 1) 
    {   
        try { interaction_thread.join(); }
        catch (const system_error& e) { common::print_error(e.what()); }
    }
    close_pipe_sockets();
}

void Klient::close_worker(int32_t socket_fd, const string& error_message,
    const string& fd_msg)
{
    ssize_t pipe_result = common::write_to_pipe(client_read_pipe[1], fd_msg);
    if (pipe_result != 1) { common::print_error("Failed to notify client."); }
    if (error_message != "") {common::print_error(error_message);}
    // Close my ends of pipes.
    common::assert_close(socket_fd);
}

void Klient::close_main(const string& error_message, const string& fd_msg)
{
    string msg = fd_msg;
    ssize_t pipe_result = 0;
    if (fd_msg != DISCONNECTED) { pipe_result = 
        common::write_to_pipe(client_read_pipe[1], msg);}
    if (pipe_result != 1) { common::print_error("Failed to notify worker."); }
    common::print_error(error_message);
    close_main_sockets(pipe_result);
}

int16_t Klient::assert_client_read_socket(ssize_t result, int32_t socket_fd)
{
    if (result < 0)
    {
        close_worker(socket_fd, "Failed to read from socket.", DISCONNECTED);
        return -1;
    }
    else if (result == 0)
    {
        access_mutex.lock();
        bool got_score_loc = got_score;
        bool got_total_loc = got_total;
        access_mutex.unlock();
        if (got_score_loc && got_total_loc) 
        {
            close_worker(socket_fd, "", NORMAL_END);
        }
        else 
        {
            close_worker(socket_fd, "Server disconnected.", SERVER_DISCONNECT);
        }
        return -1;
    }
    return 0;
}

int16_t Klient::assert_client_write_socket(ssize_t result,
    ssize_t expected, int32_t socket_fd)
{
    if (result != expected)
    {
        close_worker(socket_fd, "Failed to send message to server.",
            DISCONNECTED);
        return -1;
    }
    return 0;
}

int16_t Klient::assert_client_read_pipe(ssize_t result, 
    int32_t socket_fd, bool is_main = false)
{
    if (result < 0)
    {
        if (!is_main) {close_worker(socket_fd, "Failed to read from pipe.",
            DISCONNECTED);}
        else {close_main("Failed to read from pipe.", DISCONNECTED);}
        return -1;
    }
    else if (result == 0)
    {
        if (!is_main) {close_worker(socket_fd, "Client disconnected.",  
            DISCONNECTED);}
        else {close_main("Client disconnected.", DISCONNECTED);}
        return -1;
    }
    return 0;
}

int16_t Klient::assert_client_write_pipe(ssize_t result,
    int32_t socket_fd, bool is_main = false)
{
    if (result != 1)
    {
        if (!is_main) {close_worker(socket_fd, "Failed to write to pipe.",
            DISCONNECTED);}
        else {close_main("Failed to write to pipe.", DISCONNECTED);}
        return -1;
    }
    return 0;
}

int16_t Klient::prepare_client()
{
    if (pipe(client_read_pipe))
    {
        common::print_error("Failed to create pipes.", access_mutex);
        return -1;
    }
    else if (pipe(client_write_pipe) < 0)
    {
        close(client_read_pipe[0]);
        close(client_read_pipe[1]);
        common::print_error("Failed to create pipes.", access_mutex);
        return -1;
    }

    ssize_t result = -1;
    if (ip_version == 4) { result = common::get_server_ipv4_addr
        (host_name.c_str(), port_number, server_address); }
    else if (ip_version == 6) { result = common::get_server_ipv6_addr
        (host_name.c_str(), port_number, server6_address); }
    else 
    {
        result = common::get_server_unknown_addr
            (host_name.c_str(), port_number, server_address, server6_address);
        if (result == 0) { ip_version = 4; }
        else { ip_version = 6; }
    }
 
    if (result < 0)
    {
        close_pipe_sockets();
        return -1;
    }

    // Create a socket.
    int32_t socket_fd = -1;
    if (ip_version == 4) { socket_fd = common::create_socket(); }
    else { socket_fd = common::create_socket6(); }
    if (socket_fd < 0) { return -1; }

    if (ip_version == 4) 
    {
        if (connect(socket_fd, (struct sockaddr *) &server_address,
                    (socklen_t) sizeof(server_address)) < 0)
        {
            common::print_error("Failed to connect to server.", access_mutex);
            common::assert_close(socket_fd);
            close_pipe_sockets();
            return -1;
        }
    }
    else
    {
        if (connect(socket_fd, (struct sockaddr *) &server6_address,
                    (socklen_t) sizeof(server6_address)) < 0)
        {
            common::print_error("Failed to connect to server.", access_mutex);
            common::assert_close(socket_fd);
            close_pipe_sockets();
            return -1;
        }
    }


    socklen_t client_address_len = sizeof(client_address);
    socklen_t client6_address_len = sizeof(client6_address);
    if (ip_version == 4) { getsockname(socket_fd,
        (struct sockaddr *) &client_address, &client_address_len); }
    else { getsockname(socket_fd,
        (struct sockaddr *) &client6_address, &client6_address_len); }

    string msg;
    ssize_t send_result = senders::send_iam(socket_fd, seat, msg);
    print_logs(msg, true);
    if (send_result != (ssize_t)msg.size())
    {
        common::print_error("Failed to send seat name.", access_mutex);
        common::assert_close(socket_fd);
        return -1;
    }

    try 
    {
        interaction_thread = thread(&Klient::handle_client, this, socket_fd);
    }
    catch (const system_error& e)
    {
        common::print_error(e.what(), access_mutex);
        common::assert_close(socket_fd);
        return -1;
    }
    return socket_fd;
}

int16_t Klient::run_client()
{
    int32_t socket_fd = prepare_client();
    if (socket_fd < 0) { return 1; }

    struct pollfd poll_fds[2];
    poll_fds[0].fd = client_read_pipe[0];
    poll_fds[0].events = POLLIN;
    if (!is_ai) 
    {
        poll_fds[1].fd = STDIN_FILENO;
        poll_fds[1].events = POLLIN;
    }
    while (true)
    {
        poll_fds[0].revents = 0;
        poll_fds[1].revents = 0;
        int32_t poll_result = 0;
        if (is_ai) { poll_result = poll(poll_fds, 1, -1); }
        else { poll_result = poll(poll_fds, 2, -1); }
        if (poll_result <= 0)
        {
            close_main("Failed to poll in main client.", DISCONNECTED);
            return 1;
        }
        if (!is_ai && (poll_fds[1].revents & POLLIN))
        { // Standard stream.
            string message;
            getline(cin, message);
            if (message.substr(0, 1) == "!")
            {
                string card = message.substr(1, message.size() - 1);
                access_mutex.lock();
                messages_to_send.push(card);
                access_mutex.unlock();
                ssize_t pipe_result = 
                    common::write_to_pipe(client_write_pipe[1], CARD_PLAY);
                if (assert_client_read_pipe(pipe_result, socket_fd, true) < 0)
                {
                    return -1; 
                }
            }
            else 
            {
                access_mutex.lock();
                if (message == "cards") 
                {
                    client_printer::print_my_cards(my_cards);
                }
                else if (message == "tricks") 
                {
                    client_printer::print_my_tricks(taken_tricks);
                }
                else { cout << "Wrong command.\n"; }
                access_mutex.unlock();
            }
        }

        if (poll_fds[0].revents & POLLIN)
        { // Worker thread.
            string message;
            ssize_t pipe_result = 
                common::read_from_pipe(client_read_pipe[0], message);
            if (assert_client_read_pipe(pipe_result, socket_fd, true) < 0)
            {
                return 1;
            }
            if (message == SERVER_DISCONNECT ||
                message == DISCONNECTED || message == NORMAL_END)
            {
                close_main_sockets(pipe_result);
                if (message == NORMAL_END) { return 0; }
                else { return 1; }
            }
            else
            {
                // Some garbage. Kill myself.
                close_main("Wrong message from worker.", DISCONNECTED);
                return 1;
            }
        }
        else if (poll_fds[1].revents & POLLERR)
        {
            close_main_sockets(1, "WORKER_POLLERR");
            return 1;
        }
    }
}

string Klient::strategy(const string& color)
{
    if (my_cards.size() == 0) { return ""; }
    if (color == "")
    {
        string result = my_cards[my_cards.size() - 1];
        my_cards.pop_back();
        played_cards.push_back(result);
        return result;
    }

    for (auto iter = my_cards.begin(); iter != my_cards.end(); ++iter)
    {
        if ((*iter)[iter->size() - 1] == color[0])
        {
            string result = (*iter);
            my_cards.erase(iter);
            played_cards.push_back(result);
            return result;
        }
    }
    
    string result = my_cards[my_cards.size() - 1];
    my_cards.pop_back();
    played_cards.push_back(result);
    return result;
}

void Klient::handle_client(int32_t socket_fd)
{
    struct pollfd poll_fds[2];
    poll_fds[0].fd = client_write_pipe[0];
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = socket_fd;
    poll_fds[1].events = POLLIN;
    while (true)
    {
        int32_t poll_result = poll(poll_fds, 2, -1);
        if (poll_result <= 0)
        {
            close_worker(socket_fd,
                "Failed to poll in client worker.", DISCONNECTED);
            return;
        }
        else
        {
            if (poll_fds[0].revents & POLLIN)
            { // Message form the client.
                string message;
                ssize_t pipe_result = 
                    common::read_from_pipe(client_write_pipe[0], message);
                if (assert_client_read_pipe(pipe_result, socket_fd) < 0)
                {
                    return; 
                }
                
                if (message == DISCONNECTED)
                {
                    common::assert_close(socket_fd);
                    return;
                }
                else if (message == CARD_PLAY)
                {
                    access_mutex.lock();
                    string card = messages_to_send.front();
                    messages_to_send.pop();
                    if (is_ai)
                    {
                        auto iter = find(my_cards.begin(),
                            my_cards.end(), card);
                        if (iter != my_cards.end() && (card[card.size() - 1] ==
                            expected_color[0] || expected_color == ""))
                        {
                            my_cards.erase(iter);
                            played_cards.push_back(card);
                            expected_color = "none";
                        }
                    }
                    access_mutex.unlock();

                    string msg;
                    ssize_t send_result = senders::send_trick
                        (socket_fd, trick_number, {card}, msg);
                    print_logs(msg, true);
                    if (assert_client_write_socket
                        (send_result, msg.length(), socket_fd) < 0) { return; }
                }
                else
                {
                    // Some garbage. Kill myself.
                    close_worker(socket_fd, 
                        "Wrong message from client.", DISCONNECTED);
                    return;
                }
            
            }
            else if (poll_fds[0].revents & POLLERR)
            {
                close_worker(socket_fd, "POLLERR", DISCONNECTED);
                return;
            }


            if (poll_fds[1].revents & POLLIN)
            { // Message from the server.
                string message;
                ssize_t socket_result = 
                    common::read_from_socket(socket_fd, message);
                print_logs(message, false);
                if (assert_client_read_socket
                    (socket_result, socket_fd) < 0) { return; }
                access_mutex.lock();
                int16_t trick_loc = trick_number;

                if (regex::BUSY_check(message))
                {
                    if (!is_ai)
                    {
                        client_printer::print_busy(message);
                        access_mutex.unlock();
                    }

                    // End game.
                    close_worker(socket_fd, "", NORMAL_END);
                    return;
                }
                else if(regex::DEAL_check(message))
                {
                    trick_number = 1;
                    got_score = false;
                    got_total = false;
                    taken_tricks.clear();
                    if (!is_ai) { client_printer::print_deal(message); }
                    message = message.substr(6, message.size() - 8);
                    my_cards = regex::extract_cards(message);
                    access_mutex.unlock();
                }
                else if (regex::WRONG_check(message))
                {
                    if (!is_ai) { client_printer::print_wrong(trick_number); }
                    access_mutex.unlock();
                }
                else if (regex::TAKEN_check(message, trick_loc))
                {
                    if (!is_ai) { client_printer::print_taken
                        (message, trick_loc); }
                    string cards;
                    char taker = message[message.size() - 3];
                    if (trick_number < 10) 
                    {
                        cards = message.substr(6, message.size() - 9);
                    }
                    else 
                    {
                        cards = message.substr(7, message.size() - 10);
                    }
                    vector<string> extracted_cards
                        {regex::extract_cards(cards)};
                    for (const string& card : played_cards)
                    {
                        my_cards.push_back(card);
                    }
                    played_cards.clear();
                    for (const string& card : extracted_cards) 
                    {
                        auto iter = find(my_cards.begin(),
                            my_cards.end(), card);
                        if (iter != my_cards.end()) { my_cards.erase(iter); }
                    }
                    if (taker == seat[0]) { taken_tricks
                        .emplace_back(extracted_cards); }
                    ++trick_number;
                    access_mutex.unlock();
                }
                else if (regex::SCORE_check(message))
                {
                    got_score = true;
                    if (!is_ai) { client_printer::print_score(message); }
                    access_mutex.unlock();
                }
                else if (regex::TOTAL_check(message))
                {
                    got_total = true;
                    if (!is_ai) { client_printer::print_total(message); }
                    access_mutex.unlock();
                }
                else if (regex::TRICK_check(message, trick_loc))
                {
                    string msg_copy{message};
                    string color;
                    if (message.size() <= 9) { color = ""; }
                    else
                    {
                        if (trick_loc < 10) { message =
                                message.substr(6, message.size() - 8); }
                        else { message = 
                            message.substr(7, message.size() - 9); }
                        string first_card
                            {regex::extract_cards(message)[0]};
                        color = first_card[first_card.size() - 1];
                    }
                    expected_color = color;
                    if (!is_ai) 
                    {
                        client_printer::print_trick(msg_copy,
                            trick_number, my_cards);
                        access_mutex.unlock();
                    }
                    else
                    {
                        // Time to send a card back to the server.
                        string card_to_play = strategy(color);
                        string msg;
                        ssize_t send_result = senders::send_trick
                            (socket_fd, trick_number, {card_to_play}, msg);
                        access_mutex.unlock();
                        print_logs(msg, true);

                        if (assert_client_write_socket
                            (send_result, msg.length(), socket_fd) < 0) 
                        {
                            return;
                        }
                    }
                }
                // else: ignore messages.
                else { access_mutex.unlock(); }
            }
            else if(poll_fds[1].revents & POLLERR)
            {
                close_worker(socket_fd, "SERVER POLLERR", DISCONNECTED);
                return;
            }
        }
    }
}