#include "klient.h"

Klient::Klient(const string& host, int port, int ip, const string& seat_name, bool AI)
    : host_name(host), port_number(port), ip_version(ip), seat(seat_name), is_ai(AI), trick_number{0} {}

struct sockaddr_in Klient::get_server_address(char const *host, uint16_t port)
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

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET;   // IPv4
    send_address.sin_addr.s_addr =       // IP address
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
    send_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);

    return send_address;
}

void close_worker(int socket_fd, const string& error_message, const string& fd_msg)
{
    string msg = fd_msg;
    ssize_t pipe_result = common::write_to_pipe(client_read_pipe[1], msg);
    if (pipe_result != 1)
    {
        common::print_error("Failed to notify client.");
    }
    common::print_error(error_message);
    // Close my ends of pipes.
    close(client_read_pipe[1]);
    close(client_write_pipe[0]);
    close(socket_fd);
}

int Klient::assert_client_read_socket(ssize_t result, int socket_fd)
{
    if (result < 0)
    {
        close_worker(socket_fd, "Failed to read from socket.", DISCONNECTED);
        return -1;
    }
    else if (result == 0)
    {
        close_worker(socket_fd, "Server disconnected.", SERVER_DISCONNECT);
        return -1;
    }
    return 0;
}

int Klient::assert_client_write_socket(ssize_t result, int socket_fd)
{
    if (result != 1)
    {
        close_worker(socket_fd, "Failed to send trick.", DISCONNECTED);
        return -1;
    }
    return 0;
}

int Klient::assert_client_read_pipe(ssize_t result, int socket_fd)
{
    if (result < 0)
    {
        close_worker(socket_fd, "Failed to read from pipe.", DISCONNECTED);
        return -1;
    }
    else if (result == 0)
    {
        close_worker(socket_fd, "Client disconnected.", DISCONNECTED);
        return -1;
    }
    return 0;
}

int Klient::assert_client_write_socket(ssize_t result, int socket_fd)
{
    if (result != 1)
    {
        close_worker(socket_fd, "Failed to notify client.", DISCONNECTED);
        return -1;
    }
}


void Klient::connect_to_serwer()
{
    struct sockaddr_in server_address = get_server_address(host_name.c_str(), port_number);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) 
    {
        common::print_error("Failed to create socket.");
        return;
    }

    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) &server_address,
                (socklen_t) sizeof(server_address)) < 0)
    {
        common::print_error("Failed to connect to server.");
        close(socket_fd);
        return;
    }

    cout << "Connected to server\n";

    if (senders::send_iam(socket_fd, seat) < 0)
    {
        common::print_error("Failed to send seat name.");
        close(socket_fd);
        return;
    }

    struct pollfd poll_fds[2];
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = client_read_pipe[0];
    poll_fds[1].events = POLLIN;
    while (true)
    {
        poll_fds[0].revents = 0;
        poll_fds[1].revents = 0;
        int poll_result = poll(poll_fds, 1, -1);
        if (poll_fds[0].revents & POLLIN)
        {
            string message;
            getline(cin, message);
            if (message.substr(0, 1) == "!")
            {
                string card = message.substr(1, message.size() - 1);
                messages_mutex.lock();
                messages_to_send.push(card);
                messages_mutex.unlock();
                ssize_t pipe_result = common::write_to_pipe(client_write_pipe[1], CARD_PLAY);
                if (pipe_result != 1)
                {
                    common::print_error("Failed to notify client.");
                    // Close my ends of pipes.
                    close(client_read_pipe[1]);
                    close(client_write_pipe[0]);
                    interaction_thread.join();
                    return;
                }
            }
            else
            {
                cout << "Congrats.\n";
            }
        }
    }
}

void Klient::handle_client(int socket_fd)
{
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
            close_worker(socket_fd, "Failed to poll.", DISCONNECTED);
            return;
        }
        else
        {
            if (poll_fds[0].revents & POLLIN)
            { // Message form the client.
                string message;
                ssize_t pipe_result = common::read_from_pipe(client_write_pipe[0], message);
                if (assert_client_read_pipe(pipe_result, socket_fd) < 0) { return; }
                
                messages_mutex.lock();
                string card = messages_to_send.front();
                messages_to_send.pop();
                messages_mutex.unlock();

                ssize_t send_result = senders::send_trick(socket_fd, trick_number, {card});
                if (assert_client_write_socket(send_result, socket_fd) < 0) { return; }
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

                if (regex::BUSY_check)
                {
                    
                }
                else if(regex::DEAL_check(message))
                {
                    ++trick_number;
                    if (trick < 10) { message = message.substr(6, message.size() - 8); }
                    else { message = message.substr(7, message.size() - 9); }
                    vector<string> cards = regex::get_cards(message);
                    // display cards.
                    for (const string& card : cards) { cout << card << " "; }
                    cout << "\n";
                }
                else if (regex::WRONG_check(message))
                {
                    cout << "Server rejected your messsage.\n";
                }
                else if (regex::TAKEN_check(message))
                {
                    
                }
                else if (regex::SCORE_check(message))
                {

                }
                else if (regex::TOTAL_check(message))
                {

                }
                // else: ignore messages.
            }
            else if(poll_fds[1].revents & POLLHUP)
            {
                close_worker(socket_fd, "SERVER POLLHUP", SERVER_DISCONNECT);
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