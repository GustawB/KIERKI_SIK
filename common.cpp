#include <exception>
#include <chrono>
#include <format>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h>

#include "common.h"

ssize_t common::read_from_socket(int socket_fd, string& buffer)
{
    ssize_t bytes_read = 1;
    ssize_t total_read = 0;
    // Read from socket char-by-char.
    while (bytes_read > 0)
    {
        char c;
        ssize_t bytes_read = read(socket_fd, &c, 1);
        total_read += bytes_read;

        if (bytes_read <= 0) {return bytes_read;}
        else 
        { // Managed to read without problems.
            buffer += c;
            if (buffer.length() >= 2 && buffer.ends_with(DELIMETER)) {return buffer.length();}
            else if (total_read > MAX_BUFFER_SIZE) {return -1;}
        }
    }

    return total_read;
}

ssize_t common::read_from_pipe(int pipe_fd, string& buffer)
{
    char c;
    ssize_t bytes_read = read(pipe_fd, &c, 1);
    if (bytes_read == 1) {buffer += c;}
    return bytes_read;
}

ssize_t common::write_to_socket(int socket_fd, char* buffer, size_t buffer_length)
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

ssize_t common::write_to_pipe(int pipe_fd, const string& buffer)
{
    ssize_t bytes_written = write(pipe_fd, buffer.data(), 1);
    return bytes_written;
}

void common::print_error(const string& error_message)
{
    cerr << "\n\tERROR: " << error_message;
    if (errno != 0)
    {
        cerr << " (" << errno << ")\n";
    }
    else {cerr << "\n";}
}

ssize_t common::create_socket()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {print_error("Failed to create socket.");}
    return socket_fd;
}

ssize_t common::setup_server_socket(int port, int queue_size, struct sockaddr_in& server_addr)
{
    int server_fd = create_socket();
    if (server_fd == -1) {return -1;}

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        print_error("Failed to set socket options.");
        close(server_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        print_error("Failed to bind server socket.");
        return -1;
    }

    if (listen(server_fd, queue_size) == -1)
    {
        print_error("Failed to listen on server socket.");
        return -1;
    }

    return server_fd;
}

ssize_t common::accept_client(int socket_fd, struct sockaddr_in& client_addr)
{
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    return client_fd;
}

std::string get_time()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_c, &now_tm);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void common::print_log(const struct sockaddr_in& source_addr, const struct sockaddr_in& dest_addr, const string& message)
{
    cout << "[" << inet_ntoa(source_addr.sin_addr) << ":" << ntohs(source_addr.sin_port);
    cout << "," << inet_ntoa(dest_addr.sin_addr) << ":" << ntohs(dest_addr.sin_port) << ",";
    cout << get_time() << "] " << message;
    cout.flush();
}