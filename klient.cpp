#include "klient.h"

Klient::Klient(const string& host, int port, int ip, const string& seat_name, bool AI)
    : host_name(host), port_number(port), ip_version(ip), seat(seat_name), is_ai(AI), trick_number{0} {}

void Klient::get_server_address(char const *host, uint16_t port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0)
    {
        cout << "getaddrinfo: " << gai_strerror(errcode) << "\n";
    }

    server_address.sin_family = AF_INET;   // IPv4
    server_address.sin_addr.s_addr =       // IP address
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
    server_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);
}

void Klient::close_worker_sockets(int socket_fd)
{
    // Close my ends of pipes.
    close(client_read_pipe[1]);
    close(client_write_pipe[0]);
    close(socket_fd);
}

void Klient::close_main_sockets(ssize_t resultconst string& error_message = "")
{
    if (error_message != "") 
    { 
        printing_mutex.lock();
        common::print_error(error_message);
        printing_mutex.unlock(); 
    }
    // Close my ends of pipes.
    if (result == 1) { interaction_thread.join(); }
    close(client_read_pipe[0]);
    close(client_write_pipe[1]);
}

void Klient::close_worker(int socket_fd, const string& error_message, const string& fd_msg)
{
    string msg = fd_msg;
    ssize_t pipe_result = common::write_to_pipe(client_read_pipe[1], msg);
    printing_mutex.lock();
    if (pipe_result != 1)
    {
        common::print_error("Failed to notify client.");
    }
    if (error_message != "") {common::print_error(error_message);}
    printing_mutex.unlock();
    // Close my ends of pipes.
    close_worker_sockets(socket_fd);
}

void Klient::close_main(const string& error_message, const string& fd_msg)
{
    string msg = fd_msg;
    ssize_t pipe_result;
    if (fd_msg != DISCONNECTED) { pipe_result = common::write_to_pipe(client_read_pipe[1], msg);}
    printing_mutex.lock();
    if (fd_msg != ERROR && pipe_result != 1)
    {
        common::print_error("Failed to notify worker.");
    }
    common::print_error(error_message);
    printing_mutex.unlock();
    close_main_sockets(pipe_result);
}

int Klient::assert_client_read_socket(ssize_t result, int socket_fd)
{
    if (result < 0)
    {
        if (!is_main) {close_worker(socket_fd, "Failed to read from socket.", DISCONNECTED);}
        else {close_main("Failed to read from socket.", DISCONNECTED);}
        return -1;
    }
    else if (result == 0)
    {
        close_worker(socket_fd, "Server disconnected.", SERVER_DISCONNECT);
        return -1;
    }
    return 0;
}

int Klient::assert_client_write_socket(ssize_t result, int socket_fd, bool is_main = false)
{
    if (result != 1)
    {
        close_worker(socket_fd, "Failed to send trick.", DISCONNECTED);
        return -1;
    }
    return 0;
}

int Klient::assert_client_read_pipe(ssize_t result, int socket_fd, bool is_main = false)
{
    if (result < 0)
    {
        if (!is_main) {close_worker(socket_fd, "Failed to read from pipe.", DISCONNECTED);}
        else {close_main("Failed to read from pipe.", DISCONNECTED);}
        return -1;
    }
    else if (result == 0)
    {
        if (!is_main) {close_worker(socket_fd, "Client disconnected.", DISCONNECTED);}
        else {close_main("Client disconnected.", DISCONNECTED);}
        return -1;
    }
    return 0;
}

int Klient::assert_client_write_socket(ssize_t result, int socket_fd, bool is_main = false)
{
    if (result != 1)
    {
        close_worker(socket_fd, "Failed to notify client.", DISCONNECTED);
        return -1;
    }
}

int Klient::prepare_client()
{
    get_server_address(host_name.c_str(), port_number);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) 
    {
        common::print_error("Failed to create socket.");
        return -1;
    }

    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) &server_address,
                (socklen_t) sizeof(server_address)) < 0)
    {
        common::print_error("Failed to connect to server.");
        close(socket_fd);
        return -1;
    }

    if (senders::send_iam(socket_fd, seat) < 0)
    {
        common::print_error("Failed to send seat name.");
        close(socket_fd);
        return -1;
    }

    interaction_thread = thread(&Klient::handle_client, this, socket_fd);
    return 0;
}

int Klient::run_client()
{
    if (prepare_client() < 0) { return -1; }

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
        int poll_result = 0;
        if (is_ai) { poll_result = poll(poll_fds, 1, -1); }
        else { poll_result = poll(poll_fds, 2, -1); }
        poll(poll_fds, 1, -1);
        if (poll_result <= 0)
        {
            close_main("Failed to poll in main client.", DISCONNECTED);
            return -1;
        }
        if (!is_ai && (poll_fds[1].revents & POLLIN))
        { // Standard stream.
            string message;
            getline(cin, message);
            if (message.substr(0, 1) == "!")
            {
                string card = message.substr(1, message.size() - 1);
                messages_mutex.lock();
                messages_to_send.push(card);
                messages_mutex.unlock();
                ssize_t pipe_result = common::write_to_pipe(client_write_pipe[1], CARD_PLAY);
                if (assert_client_read_pipe(pipe_result, socket_fd, true) < 0) { return -1; }
            }
            else if (message == "cards")
            {
                printing_mutex.lock();
                client_printer::print_my_cards(my_cards);
                printing_mutex.unlock();
            }
            else if (message == "tricks")
            {
                printing_mutex.lock();
                client_printer::print_my_tricks(taken_tricks);
                printing_mutex.unlock();
            }
            else
            {
                printing_mutex.lock();
                cout << "Wrong command.\n";
                printing_mutex.unlock();
            }
        }

        if (poll_fds[0].revents & POLLIN)
        { // Worker thread.
            string message;
            ssize_t pipe_result = common::read_from_pipe(client_read_pipe[0], message);
            if (assert_client_read_pipe(pipe_result, socket_fd, true) < 0) { return; }
            if (message == SERVER_DISCONNECT || message == DISCONNECTED || message == NORMAL_END)
            {
                close_main_sockets(pipe_result);
                if (message == NORMAL_END) { return 0; }
                else { return -1; }
            }
            else
            {
                // Some garbage. Kill myself.
                close_main("Wrong message from worker.", DISCONNECTED);
                return -1;
            }
        }
        else if (poll_fds[1].revents & POLLHUP)
        {
            close_main_sockets(1, "WORKER_POLLHUP");
            return -1;
        }
        else if (poll_fds[1].revents & POLLERR)
        {
            close_main(1, "WORKER_POLLERR");
            return -1;
        }
    }
}

void Klient::handle_client(int socket_fd)
{
    printing_mutex.lock();
    cout << "Welcome to the trick game!!!\n";
    printing_mutex.unlock();

    struct pollfd poll_fds[2];
    poll_fds[0].fd = client_write_pipe[0];
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = socket_fd;
    poll_fds[1].events = POLLIN;
    bool b_got_server_resp = true;
    while (true)
    {
        int poll_result = poll(poll_fds, 2, -1);
        if (poll_result <= 0)
        {
            close_worker(socket_fd, "Failed to poll in client worker.", DISCONNECTED);
            return;
        }
        else
        {
            if (poll_fds[0].revents & POLLIN)
            { // Message form the client.
                string message;
                ssize_t pipe_result = common::read_from_pipe(client_write_pipe[0], message);
                if (assert_client_read_pipe(pipe_result, socket_fd) < 0) { return; }
                
                if (message == DISCONNECTED)
                {
                    close_worker_sockets(socket_fd);
                    return;
                }
                else if (message == CARD_PLAY)
                {
                    messages_mutex.lock();
                    string card = messages_to_send.front();
                    messages_to_send.pop();
                    messages_mutex.unlock();

                    ssize_t send_result = senders::send_trick(socket_fd, trick_number, {card});
                    if (send_result < 0) { return; }
                }
                else
                {
                    // Some garbage. Kill myself.
                    close_worker(socket_fd, "Wrong message from client.", DISCONNECTED);
                    return;
                }
            
            }
            else if (poll_fds[0].revents & POLLHUP)
            {
                // Client disconnected.
                close_worker(socket_fd, "POLLHUP", DISCONNECTED);
                return;
            }
            else if (poll_fds[0].revents & POLLERR)
            {
                close_worker(socket_fd, "POLLERR", DISCONNECTED);
                return;
            }


            if (poll_fds[1].revents & POLLIN)
            {
                string message;
                ssize_t socket_result = common::read_from_socket(socket_fd, message);
                if (assert_client_read_socket(socket_result, socket_fd) < 0) { return; }
                if (is_ai)
                {
                    printing_mutex.lock();
                    common::print_log(host_name, port_number, server_address, message);
                    printing_mutex.unlock();
                }

                if (regex::BUSY_check())
                {
                    if (!is_ai)
                    {
                        printing_mutex.lock();
                        client_printer::print_busy(message);
                        printing_mutex.unlock();
                    }

                    // End game.
                    close_worker(socket_fd, "", NORMAL_END);
                    return;
                }
                else if(regex::DEAL_check(message))
                {
                    vector<string> cards = regex::get_cards(message);
                    printing_mutex.lock();
                    got_score = false;
                    got_total = false;
                    my_cards = cards;
                    if (!is_ai) { client_printer::print_deal(message); }
                    printing_mutex.unlock();
                }
                else if (regex::WRONG_check(message))
                {
                    if (!is_ai)
                    {
                        printing_mutex.lock();
                        client_printer::print_wrong(trick_number);
                        printing_mutex.unlock();
                    }
                }
                else if (regex::TAKEN_check(message))
                {
                    printing_mutex.lock();
                    if (!is_ai) { client_printer::print_taken(message); }
                    if (message[message.size() - 1] == seat[0])
                    {
                        string cards;
                        if (trick_number < 10) { cards = message.substr(6, message.size() - 9); }
                        else { cards = message.substr(7, message.size() - 10); }
                        taken_tricks.push_back(regex::extract_cards(cards));
                    }
                    printing_mutex.unlock();
                }
                else if (regex::SCORE_check(message))
                {
                    printing_mutex.lock();
                    got_score = true;
                    if (!is_ai) { client_printer::print_score(message); }
                    printing_mutex.unlock();
                }
                else if (regex::TOTAL_check(message))
                {
                    printing_mutex.lock();
                    got_total = true;
                    if (!is_ai) { client_printer::print_total(message); }
                    printing_mutex.unlock();
                }
                else if (regex::TRICK_check(message))
                {
                    printing_mutex.lock();
                    ++trick_number;
                    if (!is_ai) { client_printer::print_trick(message, trick_number, my_cards); }
                    printing_mutex.unlock();
                }
                // else: ignore messages.
            }
            else if(poll_fds[1].revents & POLLHUP)
            {
                printing_mutex.lock();
                printing_mutex.unlock();
                if (got_score && got_total) {close_worker(socket_fd, "", NORMAL_END);}
                else {close_worker(socket_fd, "SERVER POLLHUP", SERVER_DISCONNECT);}
                return;
            }
            else if(poll_fds[1].revents & POLLERR)
            {
                close_worker(socket_fd, "SERVER POLLERR", DISCONNECTED);
                return;
            }
        }
    }
}