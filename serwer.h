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

#include "common.h"
#include "regex.h"

namespace serwer 
{
    #define QUEUE_SIZE 5

    using std::thread;
    using std::mutex;
    using std::array;
    using std::string;
    using std::vector;
    using std::cout;
    using std::map;

    using poll_size = vector<struct pollfd>::size_type;

    class Serwer
    {
    private:
        thread connection_thread;
    public:
        Serwer() = delete;
        Serwer(int port, int timeout, const std::string& game_file_name);
        ~Serwer();

        void handle_connections();
        void handle_client(int client_fd, int pipe_write_fd, int pipe_read_fd);
        void start_game();
    private:
        int port;
        int timeout;
        string game_file_name;

        thread connection_manager_thread;

        int nr_of_main_threads;
        mutex nr_of_main_threads_mutex;

        int occupied = 0;
        map<string, int> seats_status;
        mutex seats_mutex;

        // 0 - read; 1 - write
        array<int[2], 5> server_read_pipes;
        array<int[2], 5> server_write_pipes;

        map<string, int> array_mapping;
    };

    inline Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
        : port(port), timeout(timeout), game_file_name(game_file_name), 
        seats_status({{"N", -1}, {"E", -1}, {"S", -1}, {"W", -1}}), seats_mutex(),
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

    inline void Serwer::handle_client(int client_fd, int pipe_write_fd, int pipe_read_fd)
    {
        // Read the message from the client.
        cout << "Handling client " << client_fd << "\n";
        string message;
        common::read_from_socket(client_fd, message);
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
                    common::write_to_socket(server_read_pipes[array_mapping[seat]][1], server_message.data(), 1);
                }
            }
            else
            {
                cout << "Seat " << message << " is already taken\n";
                string busy_message = "BUSY";
                for (auto& [seat, fd] : seats_status)
                {
                    if (fd != -1) { busy_message += seat; }
                }
                seats_mutex.unlock();
                busy_message += "\r\n";
                common::write_to_socket(client_fd, busy_message.data(), busy_message.size());
                string server_message = "c";
                common::write_to_socket(pipe_write_fd, server_message.data(), 1);
                return;
            }

            // Read messages from the client.
            /*for(;;)
            {
                // Setup the poll for the client and the pipe.
                struct pollfd poll_descriptors[2];
                poll_descriptors[0].fd = client_fd;
                poll_descriptors[0].events = POLLIN;
                poll_descriptors[1].fd = pipe_read_fd;
                poll_descriptors[1].events = POLLIN;

                int poll_result = poll(&poll_descriptors[0], 2, -1);
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
                    cout << "TO BE CONTINUED\n";
                }
            }*/
        }
    }
} // namespace serwer