#include "senders.h"

ssize_t senders::send_iam(int32_t socket_fd, const string& seat,
    string& message)
{
    message = "IAM" + seat + DELIMETER;
    return common::write_to_socket(socket_fd,
        message.data(), message.length());
}

ssize_t senders::send_busy(int32_t socket_fd, const string& seats,
    string& message)
{
    message = "BUSY" + seats + DELIMETER;
    return common::write_to_socket(socket_fd,
        message.data(), message.length());
}

ssize_t senders::send_deal(int32_t socket_fd, int16_t deal_type,
    const string& start_seat, const vector<string>& cards, string& message)
{
    message = "DEAL" + std::to_string(deal_type) + start_seat;
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(),
        message.length());
}

ssize_t senders::send_trick(int32_t socket_fd, int16_t trick_number,
    const vector<string>& cards, string& message)
{
    message = "TRICK" + std::to_string(trick_number);
    for (const string& card : cards) { message += card; }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(),
        message.length());
}

ssize_t senders::send_wrong(int32_t socket_fd,
    int16_t trick_number, string& message)
{
    message = "WRONG" + std::to_string(trick_number) + DELIMETER;
    return common::write_to_socket(socket_fd,
        message.data(), message.length());
}

ssize_t senders::send_taken(int32_t socket_fd, int16_t trick_number,
    const vector<string>& cards, const string& taking_seat, string& message)
{
    message = "TAKEN" + std::to_string(trick_number);
    for (const string& card : cards) { message += card; }
    message += taking_seat + DELIMETER;
    return common::write_to_socket(socket_fd,
        message.data(), message.length());
}

ssize_t senders::send_score(int32_t socket_fd,
    const map<string, int32_t>& scores, string& message)
{
    message = "SCORE";
    for (const auto& score : scores) 
    {
        message += score.first + std::to_string(score.second);
    } 
    message += DELIMETER;
    return common::write_to_socket(socket_fd,
        message.data(), message.length());
}

ssize_t senders::send_total(int32_t socket_fd,
    const map<string, int32_t>& scores, string& message)
{
    message = "TOTAL";
    for (const auto& score : scores) 
    {
        message += score.first + std::to_string(score.second); 
    }
    message += DELIMETER;
    return common::write_to_socket(socket_fd, message.data(),
        message.length());
}