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

void Klient::connect_to_serwer()
{
    if (!is_ai)
    {
        if (pipe(client_read_pipe) < 0 || pipe(client_write_pipe) < 0)
        {
            common::print_error("Failed to create pipes.");
            return;
        }
        interaction_thread = thread(&Klient::handle_client, this);
    }

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

    string message = "IAM" + seat + "\r\n";

    // Send the seat name to the server.
    common::write_to_socket(socket_fd, message.data(), message.size());
    if (common::read_from_socket(socket_fd, message) < 0)
    {
        common::print_error("Failed to read from socket.");
        close(socket_fd);
        return;
    }
}

void Klient::handle_client(int socket_fd)
{
    cout << "Hello, welcome to the YYY\n";
    messages_to_send_mutex.lock();
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
            common::print_error("Poll failed.");
            break;
        }
        else
        {
            if (poll_fds[0].revents & POLLIN)
            { // Message form the client.
                string card_to_play;
                if (common::read_from_pipe(client_write_pipe[0], message) < 0)
                {
                    messages_to_send_mutex.unlock();
                    common::print_error("Failed to read from pipe.");
                    return;
                }
                messages_to_send_mutex.unlock();
                messages_to_send_mutex.lock();
                if (b_got_server_resp)
                {
                    b_got_server_resp = false;
                    card_to_play = messages_to_send.pop();
                    
                }
            }
            if (poll_fds[1].revents & POLLIN)
            {
                string message;
                if (common::read_from_socket(socket_fd, message) < 0)
                {
                    common::print_error("Failed to read from socket.");
                    break;
                }
                if(regex::TRICK_check(message))
                {
                    ++trick_number;
                    if (trick < 10)
                    {
                        message = message.substr(6, message.size() - 8);
                    }
                    else
                    {
                        message = message.substr(7, message.size() - 9);
                    }
                    vector<string> cards = regex::get_cards(message);

                }
                else if (regex::WRONG_check(message))
                {

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
                    
                if(senders::send_trick(socket_fd, trick_number, {message}) < 0)
                {
                    common::print_error("Failed to send trick.");
                    break;
                }
            }
            else if(poll_fds[1].revents & POLLHUP)
            {
                common::print_error("Server disconnected.");
                return;
            }
        }
    }
}
