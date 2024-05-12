#ifndef REGEX_H
#define REGEX_H

#include <boost/regex.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace regex
{
    using std::string;
    using std::vector;

    bool IAM_check(const string& s);
    bool BUSY_check(const string& s);
    bool DEAL_check(const string& s);
    bool TRICK_check(const string& s);
    bool TRICK_client_check(const string& s);
    bool WRONG_check(const string& s);
    bool TAKEN_check(const string& s);
    bool SCORE_check(const string& s);
    bool TOTAL_check(const string& s);

    vector<string> extract_cards(const string& s);
    string extract_trick_nr(const string& s);
    vector<string> extract_seat_score(const string& s);
} // namespace regex

#endif // REGEX_H