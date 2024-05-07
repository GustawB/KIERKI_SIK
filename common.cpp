#include "common.h"

void common::read_from_socket(int socket_fd, string& buffer)
{
    ssize_t bytes_read = 1;
    // Read from socket char-by-char.
    while (bytes_read > 0)
    {
        ssize_t bytes_read = read(socket_fd, buffer.data(), 1);
        if (bytes_read < 0) 
        {
            cout << "Failed to read from client\n";
            exit(1);
        }
        else if (bytes_read == 0)
        {
            cout << "Client disconnected\n";
            exit(1);
        }
        else 
        { // Managed to read without problems.
            if (buffer.ends_with(DELIMETER))
            {
                break;
            }
        }
    }
}

void common::write_to_socket(int socket_fd, char* buffer, size_t buffer_length)
{
    char* iter_ptr = buffer;
    while (buffer_length > 0) 
    {
        ssize_t bytes_written = write(socket_fd, iter_ptr, buffer_length);
        if (bytes_written <= 0)
        {
            cout << "Failed to write to client\n";
            exit(1);
        }
        buffer_length -= bytes_written;
    }
}
