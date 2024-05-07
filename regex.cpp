#include "regex.h"

bool regex::IAM_check(const std::string& s)
{
    boost::regex IAM_regex("IAM[NESW]\\r\\n");
    return boost::regex_match(s, IAM_regex);
}

bool regex::BUSY_check(const std::string& s)
{
    boost::regex BUSY_regex("BUSY([NESW]{0,4})\\r\\n");
    return boost::regex_match(s, BUSY_regex);
}

bool regex::DEAL_check(const std::string& s)
{
    boost::regex DEAL_regex("DEAL[1-7][NESW]([2-9]|10|[JQKA][CDHS]){13}\\r\\n");
    return boost::regex_match(s, DEAL_regex);
}

bool regex::TRICK_check(const std::string& s)
{
    boost::regex TRICK_regex("TRICK[1-13]([NESW][CDHS]){0-3}\\r\\n");
    return boost::regex_match(s, TRICK_regex);
}

bool regex::TRICK_client_check(const std::string& s)
{
    boost::regex TRICK_regex("TRICK[1-13]([NESW][CDHS]){0-3}\\r\\n");
    return boost::regex_match(s, TRICK_regex);
}

bool regex::WRONG_check(const std::string& s)
{
    boost::regex WRONG_regex("WRONG[1-13]\\r\\n");
    return boost::regex_match(s, WRONG_regex);
}

bool regex::TAKEN_check(const std::string& s)
{
    boost::regex TAKEN_regex("TAKEN[1-13]([2-9]|10|[JQKA][CDHS]){4}[NESW]\\r\\n");
    return boost::regex_match(s, TAKEN_regex);
}

bool regex::SCORE_check(const std::string& s)
{
    boost::regex SCORE_regex("SCORE([NESW]\\b\\d+\\b){4}\\r\\n");
    return boost::regex_match(s, SCORE_regex);
}

bool regex::TOTAL_check(const std::string& s)
{
    boost::regex TOTAL_regex("TOTAL([NESW]\\b\\d+\\b){4}\\r\\n");
    return boost::regex_match(s, TOTAL_regex);
}