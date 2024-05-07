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
    class Serwer
    {
    private:
        thread connection_thread;
    public:
        Serwer() = delete;
        Serwer(int port, int timeout, const std::string& game_file_name);
        ~Serwer();

        void handle_connections();
        void handle_client(int client_fd);
    private:
        int port;
        int timeout;
        string game_file_name;

        thread connection_manager_thread;
        
        int total_threads;
        bool b_is_waiting;
        mutex total_threads_mutex;
        mutex remaining_threads_mutex;

        int nr_of_main_threads;
        mutex nr_of_main_threads_mutex;

        map<string, bool> seats_status;
        map<string, mutex> seats_mutex;
    };

    inline Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
        : port(port), timeout(timeout), game_file_name(game_file_name), total_threads(0), total_threads_mutex(),
            seats_status({{"N", false}, {"E", false}, {"S", false}, {"W", false}})
    {
        // std::mutex has literally no initialization functionality, hence this crazy ass magic.
        seats_mutex.emplace(std::piecewise_construct, std::make_tuple("N"), std::make_tuple());
        seats_mutex.emplace(std::piecewise_construct, std::make_tuple("E"), std::make_tuple());
        seats_mutex.emplace(std::piecewise_construct, std::make_tuple("S"), std::make_tuple());
        seats_mutex.emplace(std::piecewise_construct, std::make_tuple("W"), std::make_tuple());
        
        connection_manager_thread = thread(&Serwer::handle_connections, this);
    }

    inline Serwer::~Serwer()
    {
        connection_manager_thread.join();
        // Wait for every remaining thread to finish.
        total_threads_mutex.lock();
        if (total_threads > 0)
        {
            b_is_waiting = true;
            total_threads_mutex.unlock();
            remaining_threads_mutex.lock();
        }
        else 
        {
            total_threads_mutex.unlock();
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

        for (;;) 
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

            cout << "Accepted connection\n";

            // Create a new thread to handle the client and detach it.
            thread client_thread(&Serwer::handle_client, this, client_fd);
            client_thread.detach();
        }
    }

    inline void Serwer::handle_client(int client_fd)
    {
        total_threads_mutex.lock();
        total_threads++;
        total_threads_mutex.unlock();

        // Read the message from the client.
        array<char, 1024> buffer;
        int bytes_read = read(client_fd, buffer.data(), buffer.size());
        if (bytes_read < 0) 
        {
            cout << "Failed to read from client\n";
            exit(1);
        }

        string message(buffer.data(), bytes_read);
        cout << "Received message: " << message << "\n";

        seats_mutex[message].lock();
        if (seats_status[message] == true)
        {
            cout << "Seat " << message << " is already taken\n";
        }
        else
        {
            cout << "Seat " << message << " is free\n";
            seats_status[message] = true;
        }
        seats_mutex[message].unlock();

        // Close the connection.
        close(client_fd);

        total_threads_mutex.lock();
        total_threads--;
        if (total_threads == 0 && b_is_waiting)
        {
            // Notify the destructor that all threads have finished.
            remaining_threads_mutex.unlock();
        }
        total_threads_mutex.unlock();
    }
} // namespace serwer