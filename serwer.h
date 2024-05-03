#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <array>

namespace serwer 
{
    using std::thread;
    using std::mutex;
    using std::array;
    using std::string;
    using std::vector;
    using std::cout;
    class Serwer
    {
    private:
        thread connection_thread;
    public:
        Serwer() = delete;
        Serwer(int port, int timeout, const std::string& game_file_name);
        ~Serwer();

    private:
        void handle_connections();

        int port;
        int timeout;
        string game_file_name;

        thread connection_manager_thread;
        array<int, 4> socket_fds;
    };

    inline Serwer::Serwer(int port, int timeout, const std::string& game_file_name)
        : port(port), timeout(timeout), game_file_name(game_file_name), socket_fds({-1, -1, -1, -1})
    {
        connection_manager_thread = thread(&Serwer::handle_connections, this);

        
    }

    inline Serwer::~Serwer()
    {
        connection_manager_thread.join();
    }

    inline void Serwer::handle_connections()
    {
        int socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
        if (socket_fd == -1)
        {
            cout << "Error creating socket\n";
            exit(1);
        }

        // Bind the socket to a concrete address.
        struct sockaddr_in6 server_address;
        server_address.sin6_family = AF_INET6; // IPv6
        server_address.sin6_addr = in6addr_any; // Listening on all interfaces.
        server_address.sin6_port = htons(port);

        if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
            cout << "Error binding socket\n";
            exit(1);
        }

        // Switch the socket to listening.
        if (listen(socket_fd, 10) < 0) {
            cout << "Error listening on socket\n";
            exit(1);
        }

        for (;;) 
        {
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);
            int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                                &client_address_len);
            if (client_fd < 0) 
            {
                cout << "Error accepting connection\n";
                exit(1);
            }

            cout << "Connection accepted\n";
        }
    }
} // namespace serwer