#include "regex.h"

bool has_unique_chars(const string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        for (size_t j = i + 1; j < s.size(); ++j)
        {
            if (s[i] == s[j])
            {
                return false;
            }
        }
    }

    return true;
}

bool regex::IAM_check(const std::string& s)
{
    boost::regex IAM_regex("IAM[NESW]\\r\\n");
    return boost::regex_match(s, IAM_regex);
}

bool regex::BUSY_check(const std::string& s)
{
    boost::regex BUSY_regex("BUSY([NESW]{0,4})\\r\\n");

    if (!boost::regex_match(s, BUSY_regex))
    {
        return false;
    }
    else
    {
        string seats = s.substr(4, s.find('\r') - 1);
        return has_unique_chars(seats);
    }
}

bool regex::DEAL_check(const std::string& s)
{
    boost::regex DEAL_regex("DEAL[1-7][NESW]([][NESW]{0,4})\\r\\n");
}