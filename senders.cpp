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

ssize_t senders::send_deal(int socket_fd, int16_t deal_type, const string& start_seat, const vector<string>& cards)
{
    string message = "DEAL " + std::to_string(deal_type) + start_seat;
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_trick(int socket_fd, int16_t trick_type, const vector<string>& cards)
{
    string message = "TRICK " + std::to_string(trick_type);
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_wrong(int socket_fd, int16_t trick_type)
{
    string message = "WRONG " + std::to_string(trick_type) + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_taken(int socket_fd, int16_t trick_type, const vector<string>& cards, const string& taking_seat)
{
    string message = "TAKEN " + std::to_string(trick_type);
    for (const string& card : cards) { message += card; }
    message += taking_seat + DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_score(int socket_fd, const map<string, int32_t>& scores)
{
    string message = "SCORE";
    for (const auto& score : scores) { message += score.first + std::to_string(score.second); } 
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}

ssize_t senders::send_total(int socket_fd, const map<string, int32_t>& scores)
{
    string message = "TOTAL";
    for (const auto& score : scores) { message += score.first + std::to_string(score.second); }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(), message.length());
}