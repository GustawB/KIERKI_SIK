#include "senders.h"

ssize_t senders::send_iam(int socket_fd, const string& seat)
{
    string message = "IAM " + seat + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_busy(int socket_fd, const string& seats)
{
    string message = "BUSY " + seats + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_deal(int socket_fd, int8_t deal_type, const string& start_seat, const vector<string>& cards)
{
    string message = "DEAL " + std::to_string(deal_type) + start_seat;
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_trick(int socket_fd, int8_t trick_type, const vector<string>& cards)
{
    string message = "TRICK " + std::to_string(trick_type);
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_wrong(int socket_fd, int8_t trick_type)
{
    string message = "WRONG " + std::to_string(trick_type) + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_taken(int socket_fd, int8_t trick_type, const vector<string>& cards, const string& taking_seat)
{
    string message = "TAKEN " + std::to_string(trick_type);
    for (const string& card : cards) { message += card; }
    message += taking_seat + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_score(int socket_fd, const array<int, 4>& scores)
{
    array<string, 4> seats{"N", "E", "S", "W"};
    string message = "SCORE";
    for (int i = 0; i < 4; i++) { message += seats[i] + std::to_string(scores[i]); }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_total(int socket_fd, const array<int, 4>& scores)
{
    array<string, 4> seats{"N", "E", "S", "W"};
    string message = "TOTAL";
    for (int i = 0; i < 4; i++) { message += seats[i] + std::to_string(scores[i]); }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}