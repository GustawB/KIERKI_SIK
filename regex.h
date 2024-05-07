#include <boost/regex.hpp>
#include <iostream>

namespace regex
{
    using std::string;

    bool IAM_check(const std::string& s);
    bool BUSY_check(const std::string& s);
    bool DEAL_check(const std::string& s);
} // namespace regex