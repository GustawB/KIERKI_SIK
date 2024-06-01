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

    ssize_t send_iam(int32_t socket_fd, const string& seat, string& message);
    ssize_t send_busy(int32_t socket_fd, const string& seats, string& message);
    ssize_t send_deal(int32_t socket_fd, int16_t deal_type, const string& start_seat, const vector<string>& cards, string& message);
    ssize_t send_trick(int32_t socket_fd, int16_t trick_number, const vector<string>& cards, string& message);
    ssize_t send_wrong(int32_t socket_fd, int16_t trick_number, string& message);
    ssize_t send_taken(int32_t socket_fd, int16_t trick_number, const vector<string>& cards, const string& taking_seat, string& message);
    ssize_t send_score(int32_t socket_fd, const map<string, int32_t>& scores, string& message);
    ssize_t send_total(int32_t socket_fd, const map<string, int32_t>& scores, string& message);   
} // namespace senders

#endif // SENDERS_H