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

namespace serwer 
{
    #define QUEUE_SIZE 69

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
        void handle_connections();
        void handle_client(int client_fd, int pipe_write_fd, int pipe_read_fd);
        void close_client_thread(const string& error_message, initializer_list<int> fds);
        int assert_client_read_socket(ssize_t result, initializer_list<int> fds);
        int assert_client_write_socket(ssize_t result, initializer_list<int> fds);

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

        map<string, int16_t> round_scores;
        map<string, int16_t> total_scores;
    };

    inline Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
        : port(port), timeout(timeout), game_file_name(game_file_name), 
        client_threads(), client_threads_mutex(), occupied(0), current_message(""), current_message_mutex(), cards_on_table(""),
        seats_status({{"N", -1}, {"E", -1}, {"S", -1}, {"W", -1}}), seats_mutex(), round_scores(), total_scores(),
        array_mapping({{"N", 0}, {"E", 1}, {"S", 2}, {"W", 3}}) {}

    inline Serwer::~Serwer()
    {
        connection_manager_thread.join();
    }

    inline void Serwer::start_game()
    {
        cout << "Starting the game\n";
        for (int i = 0; i < 5; ++i)
        {
            if (pipe(server_read_pipes[i]) < 0 || pipe(server_write_pipes[i]) < 0)
            {
                cout << "Failed to create pipes\n";
                exit(1);
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

    inline void Serwer::run_game()
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
            
            int16_t trick = fr.get_trick_type();
            strign seat = fr.get_seat();
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

    inline void Serwer::handle_connections()
    {
        // Create a socket.
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) 
        {
            cout << "Failed to create socket\n";
            exit(1);    
        }

        // Bind the socket to a concrete address.
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET; // IPv4
        server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
        server_address.sin_port = htons(port);

        if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
        {
            cout << "Failed to bind socket\n";
            exit(1);
        }

        // Switch the socket to listening.
        if (listen(socket_fd, QUEUE_SIZE) < 0)
        {
            cout << "Failed to listen\n";
            exit(1);
        }

        cout << "Listening on port " << port << "\n";

        // Four for seats, one for server, one for new connection.
        vector<struct pollfd> poll_descriptors(6);
        vector<int> pipe_fds(6, -1);
        poll_descriptors[0].fd = socket_fd;
        poll_descriptors[0].events = POLLIN;
        // Initialize the rest of the fds.

        for (poll_size i = 1; i < 6; i++)
        {
            poll_descriptors[i].fd = -1;
            poll_descriptors[i].events = POLLIN;
        }

        for (;;) 
        {
            // Reset the revents.
            for (poll_size i = 0; i < poll_descriptors.size(); ++i)
            {
                poll_descriptors[i].revents = 0;
            }

            int poll_result = poll(&poll_descriptors[0], poll_descriptors.size(), -1);
            if (poll_result < 0)
            {
                cout << "Failed to poll\n";
                exit(1);
            }
            else if (poll_result == 0)
            {
                cout << "De fuq?\n";
                continue;
            }
            else
            {
                // Handle the new connection.
                if (poll_descriptors[0].revents & POLLIN)
                {
                    struct sockaddr_in client_address;
                    socklen_t client_address_len = sizeof(client_address);
                    int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                                        &client_address_len);
                    if (client_fd < 0) 
                    {
                        cout << "Failed to accept connection\n";
                        exit(1);
                    }

                    int pipe_write_fd[2], pipe_read_fd[2];
                    if (pipe(pipe_write_fd) < 0 || pipe(pipe_read_fd) < 0)
                    {
                        cout << "Failed to create pipes\n";
                        exit(1);
                    }

                    poll_descriptors.push_back({pipe_read_fd[0], POLLIN, 0});
                    pipe_fds.push_back(pipe_write_fd[1]);
                    cout << "Accepted connection\n";
                    thread client_thread(&Serwer::handle_client, this, client_fd, pipe_read_fd[1], pipe_write_fd[0]);
                    client_thread.detach();
                }

                // Handle new clients that are eligible for the place at the table.
                for (poll_size i = 6; i < poll_descriptors.size(); ++i)
                {
                    if (poll_descriptors[i].revents & POLLIN)
                    {
                        // Client managed to reserve a spot at the table. Move him to the top four fds.
                        string msg{"0"};
                        ssize_t bytes_read = read(poll_descriptors[i].fd, msg.data(), 1);
                        if (bytes_read < 0)
                        {
                            cout << "Failed to read from client\n";
                            exit(1);
                        }
                        for (int j = 1; j < 5; ++j)
                        {
                            if (poll_descriptors[j].fd == -1)
                            {
                                poll_descriptors[j] = poll_descriptors[i];
                                auto iter = poll_descriptors.begin() + i;
                                poll_descriptors.erase(iter);
                                --i;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    inline void Serwer::close_client_thread(const string& error_message, initializer_list<int> fds)
    {
        common::print_error(error_message);
        string server_message = "c";
        pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
        if (pipe_write != 1) {common::print_error("Failed to notify server.");}
        for (int fd : fds) { close(fd); }
    }

    inline int Serwer::assert_client_read_socket(ssize_t result, initializer_list<int> fds)
    {
        string server_message = "c";
        if (result == 0)
        {
            // Client disconnected.
            close_client_thread("Client disconnected.", fds);
            return -1;
        }
        else if (result < 0)
        {
            // Error.
            close_client_thread("Failed to read from client.", fds);
            return -1;
        }
        return 0;
    }

    inline int Serwer::assert_client_write_socket(ssize_t result, initializer_list<int> fds)
    {
        if (socket_write <= 0)
        {
            close_client_thread("Failed to send message to the client.", {client_fd, pipe_write_fd, pipe_read_fd});
            return -1;
        }
        return 0;
    }

    inline void Serwer::handle_client(int client_fd, int pipe_write_fd, int pipe_read_fd)
    {
        // Read the message from the client.
        cout << "Handling client " << client_fd << "\n";
        string message;
        ssize_t pipe_read = -1;
        ssize_t socket_read = common::read_from_socket(client_fd, message);
        ssize_t pipe_write = -1;
        ssize_t socket_write = -1;
        int16_t current_trick = 1;
        if (socket_read == 0) 
        {
            // Client disconnected. Here we don't have to worry about anything, end execution.
            close(client_fd);
            return; 
        }
        else if (socket_read < 0)
        {
            close(client_fd);
            common::print_error("Failed to read IAM message.");
        }
        if (regex::IAM_check(message))
        {
            string seat = message.substr(3, 1);
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
                    socket_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], server_message.data());
                    if (socket_read != 1)
                    {
                        close(client_fd);
                        common::print_error("Failed to notify server.");
                    }
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
                socket_write = senders::send_busy(client_fd, occupied_seats);
                if (socket_write <= 0)
                {
                    close(client_fd);
                    common::print_error("Failed to send BUSY message.");
                    return;
                }
                return;
            }

            // Managed to get a place at the table.
            // Setup the poll for the client and the pipe.
            struct pollfd poll_descriptors[2];
            poll_descriptors[0].fd = client_fd;
            poll_descriptors[0].events = POLLIN;
            poll_descriptors[1].fd = server_write_pipes[array_mapping[seat]][0];
            poll_descriptors[1].events = POLLIN;

            bool b_was_destined_to_play = false;

            // Read messages from the client.
            for(;;)
            {
                int poll_result = poll(&poll_descriptors[0], 2, 5000);
                if (poll_result == 0)
                {
                    socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
                    if (assert_client_write_socket(socket_write, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}
                }
                else if (poll_result < 0)
                {
                    close(client_fd);
                    common::print_error("Failed to poll.");
                    return;  
                }
                else
                {
                    if (poll_descriptors[0].revents & POLLIN)
                    { // Client sent a message.
                        string client_message;
                        socket_read = common::read_from_socket(client_fd, client_message);
                        if (assert_client_read_socket(socket_read, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}

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
                                if (pipe_write != 1) 
                                {
                                    close(client_fd);
                                    common::print_error("Failed to notify server.");
                                    return;
                                }
                                b_was_destined_to_play = false;
                            }
                            else
                            {
                                // Client send a message out of order.
                                socket_write = senders::send_wrong(client_fd, current_trick);
                                if (assert_client_write_socket(socket_write, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}
                            }
                            
                        }
                        else
                        {
                            // Invalid message, close the connection.
                            close(client_fd);
                            common::print_error("Client send invalid message.");
                            pipe_write = common::write_to_pipe(server_read_pipes[array_mapping[seat]][1], "c");
                            if (pipe_write != 1) {common::print_error("Failed to notify server.");}
                            return;
                        }
                    }
                    
                    if (poll_descriptors[1].revents & POLLIN)
                    { // Server sent a message.
                        string server_message;
                        pipe_read = common::read_from_pipe(pipe_read_fd, server_message);
                        if (pipe_read == 0)
                        {
                            // Server disconnected. SHouldn't happen but whatever, close connections.
                            close(client_fd);
                            common::print_error("Server disconnected.");
                            return;
                        }
                        else if (pipe_read < 0)
                        {
                            // Error communicating with the server.
                            close(client_fd);
                            common::print_error("Failed to read from server.");
                            return;
                        }

                        if (server_message == "p")
                        {
                            // Server wants the client to play a card.
                            socket_write = senders::send_trick(client_fd, current_trick, cards_on_table);
                            if (assert_client_write_socket(socket_write, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}
                            b_was_destined_to_play = true;
                        }
                        else if (server_message == "c")
                        {
                            // Server wants the client to disconnect.
                            close(client_fd);
                            return;
                        }
                        else if(server_message == "s")
                        {
                            // Score.
                            socket_write = senders::send_score(client_fd, round_scores);
                            if (assert_client_write_socket(socket_write, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}
                        }
                        else if(server_message == "t")
                        {
                            // Total score.
                            socket_write = senders::send_total(client_fd, total_scores);
                            if (assert_client_write_socket(socket_write, {client_fd, pipe_write_fd, pipe_read_fd}) < 0) {return;}
                        }
                        else
                        {
                            // Server send invalid message.
                            close(client_fd);
                            common::print_error("Server send invalid message.");
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            common::print_error("Client send invalid message.");
        }
        close(client_fd);
    }
} // namespace serwer

#endif // SERWER_H