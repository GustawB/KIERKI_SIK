#include <exception>

#include "common.h"

ssize_t common::read_from_socket(int socket_fd, string& buffer)
{
    ssize_t bytes_read = 1;
    // Read from socket char-by-char.
    while (bytes_read > 0)
    {
        char c;
        ssize_t bytes_read = read(socket_fd, &c, 1);

        if (bytes_read = 0) {return bytes_read;}
        else 
        { // Managed to read without problems.
            buffer += c;
            if (buffer.length() >= 2 && buffer.ends_with(DELIMETER)) {return 1;}
        }
    }
}

ssize_t common::write_to_socket(int socket_fd, char* buffer, size_t buffer_length)
{
    char* iter_ptr = buffer;
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
}
