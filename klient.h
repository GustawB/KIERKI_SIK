#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <array>
#include <cstring>
#include <string>
#include <netdb.h>
#include <cinttypes>

namespace klient
{
    using std::string;
    using std::cerr;
    using std::cout;

    class Klient
    {
    public:
        Klient() = delete;
        Klient(const string& host, int port, int ip, const string& seat_name, bool AI);
        ~Klient();

        void connect_to_serwer();

        struct sockaddr_in get_server_address(char const *host, uint16_t port);

        string host_name;
        int port_number;
        int ip_version;
        string seat;
        bool is_ai;
    };

    inline Klient::Klient(const string& host, int port, int ip, const string& seat_name, bool AI)
        : host_name(host), port_number(port), ip_version(ip), seat(seat_name), is_ai(AI) {}

    inline Klient::~Klient() {}

    inline struct sockaddr_in Klient::get_server_address(char const *host, uint16_t port)
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
    
    inline void Klient::connect_to_serwer()
    {
        struct sockaddr_in server_address = get_server_address(host_name.c_str(), port_number);

        // Create a socket.
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0)
        {
            cout << "Failed to create socket\n";
            exit(1);
        }

        // Connect to the server.
        if (connect(socket_fd, (struct sockaddr *) &server_address,
                    (socklen_t) sizeof(server_address)) < 0)
        {
            cout << "Failed to connect to server\n";
            exit(1);
        }

        cout << "Connected to server\n";

        // Send the seat name to the server.
        if (send(socket_fd, seat.c_str(), seat.size(), 0) < 0)
        {
            cout << "Failed to send seat name\n";
            exit(1);
        }

        close(socket_fd);
        cout << "Connection closed\n";
    }


} // namespace klient