#include <boost/regex.hpp>
#include <iostream>

namespace regex
{
    using std::string;

    bool IAM_check(const string& s);
    bool BUSY_check(const string& s);
    bool DEAL_check(const string& s);
    bool TRICK_check(const string& s);
    bool TRICK_client_check(const string& s);
    bool WRONG_check(const string& s);
    bool TAKEN_check(const string& s);
    bool SCORE_check(const string& s);
    bool TOTAL_check(const string& s);
} // namespace regex