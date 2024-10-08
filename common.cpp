#include "common.h"

std::string get_time()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_c, &now_tm);
    auto ms = std::chrono::duration_cast<std::chrono
        ::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void log(const struct sockaddr_in& source_addr,
    const struct sockaddr_in& dest_addr, const string& message)
{
    cout << "[" << inet_ntoa(source_addr.sin_addr) << ":" 
        << ntohs(source_addr.sin_port);
    cout << "," << inet_ntoa(dest_addr.sin_addr) << ":" 
        << ntohs(dest_addr.sin_port) << ",";
    cout << get_time() << "] " << message;
    cout.flush();
}

void log(const struct sockaddr_in6& source_addr,
    const struct sockaddr_in6& dest_addr, const string& message)
{
    char* str = (char*)malloc(INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &source_addr.sin6_addr, str, INET6_ADDRSTRLEN);
    string src{str};
    inet_ntop(AF_INET6, &dest_addr.sin6_addr, str, INET6_ADDRSTRLEN);
    string dest{str};
    free(str);

    cout << "[" << src << ":" << ntohs(source_addr.sin6_port);
    cout << "," << dest << ":" << ntohs(dest_addr.sin6_port) << ",";
    cout << get_time() << "] " << message;
    cout.flush();
}

void common::print_log(const struct sockaddr_in6& src_addr,
    const struct sockaddr_in6& dest_addr, const string& message,
    mutex& log_mutex, bool is_ai)
{
    if (is_ai && message != "")
    {
        log_mutex.lock();
        log(src_addr, dest_addr, message);
        if (message.size() < 2 || message.substr(message.size() - 2, 2) != 
            "\r\n") { cout << "\n"; }
        log_mutex.unlock();
    }
}

void common::print_log(const struct sockaddr_in& src_addr,
    const struct sockaddr_in& dest_addr, const string& message,
    mutex& log_mutex, bool is_ai)
{
    if (is_ai && message != "")
    {
        log_mutex.lock();
        log(src_addr, dest_addr, message);
        if (message.size() < 2 || 
            message.substr(message.size() - 2, 2) != "\r\n") { cout << "\n"; }
        log_mutex.unlock();
    }
}

void common::print_error(const string& error_message)
{
    cerr << "\n\tERROR: " << error_message;
    if (errno != 0) { cerr << " (" << errno << ")\n"; }
    else {cerr << "\n";}
}

void common::print_error(const string& message, mutex& error_mutex)
{
    error_mutex.lock();
    print_error(message);
    error_mutex.unlock();
}

ssize_t common::read_from_socket(int32_t socket_fd, string& buffer)
{
    ssize_t bytes_read = 1;
    ssize_t total_read = 0;
    // Read from socket char-by-char.
    while (bytes_read > 0)
    {
        char c;
        ssize_t bytes_read = read(socket_fd, &c, 1);
        total_read += bytes_read;

        if (bytes_read <= 0) { return bytes_read; }
        else 
        { // Managed to read without problems.
            buffer += c;
            if (buffer.length() >= 2 && buffer.ends_with(DELIMETER))
            {
                return buffer.length();
            }
            else if (total_read > MAX_BUFFER_SIZE) 
            { 
                return -1;
            }
        }
    }

    return total_read;
}

ssize_t common::read_from_pipe(int32_t pipe_fd, string& buffer)
{
    char c;
    ssize_t bytes_read = read(pipe_fd, &c, 1);
    if (bytes_read == 1) {buffer += c;}
    return bytes_read;
}

ssize_t common::write_to_socket(int32_t socket_fd, char* buffer,
    size_t buffer_length)
{
    char* iter_ptr = buffer;
    ssize_t store_buff_len = buffer_length;
    while (buffer_length > 0) 
    {
        ssize_t bytes_written = write(socket_fd, iter_ptr, buffer_length);
        if (bytes_written <= 0) {return bytes_written;}
        else 
        {
            iter_ptr += bytes_written;
            buffer_length -= bytes_written;
        }
    }

    return store_buff_len - buffer_length;
}

ssize_t common::write_to_pipe(int32_t pipe_fd, const string& buffer)
{
    ssize_t bytes_written = write(pipe_fd, buffer.data(), 1);
    return bytes_written;
}

void common::assert_close(int32_t fd)

{
    if (close(fd) == -1) { cerr << "Failed to close file descriptor.\n"; }
}

int32_t common::create_socket()
{
    int32_t socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) { print_error("Failed to create socket."); }
    return socket_fd;
}

int32_t common::create_socket6()
{
    int32_t socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (socket_fd == -1) { print_error("Failed to create socket."); }
    return socket_fd;
}

ssize_t common::get_server_ipv4_addr(char const *host, int32_t port,
    struct sockaddr_in& server_address)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result = nullptr;
    int32_t errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0)
    {
        print_error("getaddrinfo failed: " + string(gai_strerror(errcode)));
        if (address_result != nullptr) {
            freeaddrinfo(address_result);
        }
        return -1;
    }

    struct addrinfo *result = address_result;
    while (result != nullptr)
    {
        if (result->ai_family == AF_INET)
        {
            server_address.sin_family = AF_INET;   // IPv4
            server_address.sin_addr.s_addr = ((struct sockaddr_in *)
                (address_result->ai_addr))->sin_addr.s_addr;
            server_address.sin_port = htons(port); // port from the cmd
            freeaddrinfo(address_result);
            return 0;
        }
        result = result->ai_next;
    }

    freeaddrinfo(address_result);
    print_error("Failed to find IPv4 address.");
    return -1;
}

ssize_t common::get_server_ipv6_addr(char const *host, int32_t port,
    struct sockaddr_in6& server_address)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6; // IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result = nullptr;
    int32_t errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0)
    {
        print_error("getaddrinfo failed: " + string(gai_strerror(errcode)));
        if (address_result != nullptr) { freeaddrinfo(address_result); }
        return -1;
    }

    struct addrinfo *result = address_result;
    while (result != nullptr)
    {
        if (result->ai_family == AF_INET6)
        {
            server_address.sin6_family = AF_INET6;   // IPv6
            server_address.sin6_addr = ((struct sockaddr_in6 *)
                (address_result->ai_addr))->sin6_addr;
            server_address.sin6_port = htons(port); // port from the cmd
            freeaddrinfo(address_result);
            return 0;
        }
        result = result->ai_next;
    }

    freeaddrinfo(address_result);
    print_error("Failed to find IPv6 address.");
    return -1;
}


ssize_t common::get_server_unknown_addr(char const *host, int32_t port,
    struct sockaddr_in& v4_addr, struct sockaddr_in6& v6_addr)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result = nullptr;
    int32_t errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0)
    {
        print_error("getaddrinfo failed: " + string(gai_strerror(errcode)));
        if (address_result != nullptr) { freeaddrinfo(address_result); }
        return -1;
    }

    struct addrinfo *result = address_result;
    while (result != nullptr)
    {
        if (result->ai_family == AF_INET)
        {
            v4_addr.sin_family = AF_INET;
            v4_addr.sin_addr.s_addr = ((struct sockaddr_in *)
                (address_result->ai_addr))->sin_addr.s_addr;
            v4_addr.sin_port = htons(port);
            freeaddrinfo(address_result);
            return 0;
        }
        else if (result->ai_family == AF_INET6)
        {
            v6_addr.sin6_family = AF_INET6;
            v6_addr.sin6_addr = ((struct sockaddr_in6 *)
                (address_result->ai_addr))->sin6_addr;
            v6_addr.sin6_port = htons(port);
            freeaddrinfo(address_result);
            return 1;
        }
        result = result->ai_next;
    }

    print_error("Failed to find IPv4 or IPv6 address.");
    freeaddrinfo(address_result);
    return -1;
}

int32_t common::setup_server_socket(int32_t port, int32_t queue_size,
    struct sockaddr_in6& server_addr)
{
    int32_t server_fd = create_socket6();
    if (server_fd == -1) {return -1;}

    int32_t optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
        &optval, sizeof(optval)) < 0) 
    {
        print_error("Failed to set socket options.");
        assert_close(server_fd);
        return 1;
    }

    optval = 0;
    // Set the IPV6_V6ONLY option to false so that the server can accept
    // both IPv4 and IPv6 connections.
    if (setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY,
        &optval, sizeof(optval)) == -1)
    {
        print_error("Failed to set socket options.");
        assert_close(server_fd);
        return 1;
    }

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_flowinfo = 0;
    server_addr.sin6_scope_id = 0;
    if (port > 0) {server_addr.sin6_port = htons(port);}

    if (bind(server_fd, (struct sockaddr*)&server_addr,
        sizeof(server_addr)) == -1)
    {
        print_error("Failed to bind server socket.");
        assert_close(server_fd);
        return -1;
    }

    if (listen(server_fd, queue_size) == -1)
    {
        print_error("Failed to listen on server socket.");
        assert_close(server_fd);
        return -1;
    }

    return server_fd;
}

int32_t common::accept_client(int32_t socket_fd,
    struct sockaddr_in6& client_addr)
{
    socklen_t client_addr_len = sizeof(client_addr);
    int32_t client_fd = accept(socket_fd, 
        (struct sockaddr*)&client_addr, &client_addr_len);
    return client_fd;
}