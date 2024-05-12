#ifndef SENDERS_H
#define SENDERS_H

#include <stdio.h>
#include <string>
#include <vector>
#include <map>

#include "common.h"

namespace senders
{
    using std::string;
    using std::vector;
    using std::map;

    ssize_t send_iam(int socket_fd, const string& seat);
    ssize_t send_busy(int socket_fd, const string& seats);
    ssize_t send_deal(int socket_fd, int8_t deal_type, const string& start_seat, const vector<string>& cards);
    ssize_t send_trick(int socket_fd, int8_t trick_type, const vector<string>& cards);
    ssize_t send_wrong(int socket_fd, int8_t trick_type);
    ssize_t send_taken(int socket_fd, int8_t trick_type, const vector<string>& cards, const string& taking_seat);
    ssize_t send_score(int socket_fd, const map<string, int32_t>& scores);
    ssize_t send_total(int socket_fd, const map<string, int32_t>& scores);   
} // namespace senders

#endif // SENDERS_H