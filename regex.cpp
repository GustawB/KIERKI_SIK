#include "regex.h"

bool regex::IAM_check(const std::string& s)
{
    boost::regex IAM_regex("^IAM[NESW]\\r\\n$");
    return boost::regex_match(s, IAM_regex);
}

bool regex::BUSY_check(const std::string& s)
{
    boost::regex BUSY_regex("^BUSY([NESW]{0,4})\\r\\n$");
    return boost::regex_match(s, BUSY_regex);
}

bool regex::DEAL_check(const std::string& s)
{
    boost::regex DEAL_regex
        ("^DEAL[1-7][NESW](([2-9]|1[0]|J|Q|K|A)[CDHS]){13}\\r\\n$");
    return boost::regex_match(s, DEAL_regex);
}
  
bool regex::TRICK_check(const std::string& s, int16_t trick_nr)
{
    boost::regex TRICK_regex("^TRICK" + to_string(trick_nr) + 
        "(([2-9]|1[0]|J|Q|K|A)[CDHS]){0,3}\\r\\n$");
    return regex_match(s, TRICK_regex);
}

bool regex::TRICK_client_check(const std::string& s)
{
    boost::regex TRICK_regex
        ("^TRICK([1-9]|1[0-3])([2-9]|1[0]|J|Q|K|A)[CDHS]\\r\\n$");
    return boost::regex_match(s, TRICK_regex);
}

bool regex::WRONG_check(const std::string& s)
{
    boost::regex WRONG_regex("^WRONG([1-9]|1[0-3])\\r\\n$");
    return boost::regex_match(s, WRONG_regex);
}

bool regex::TAKEN_check(const std::string& s, int16_t trick_nr)
{
    boost::regex TAKEN_regex
        ("^TAKEN" + to_string(trick_nr) + 
        "(([2-9]|1[0]|J|Q|K|A)[CDHS]){4}[NESW]\\r\\n$");
    return boost::regex_match(s, TAKEN_regex);
}

bool regex::SCORE_check(const std::string& s)
{
    boost::regex SCORE_regex("^SCORE([NESW]\\d+){4}\\r\\n$");
    return boost::regex_match(s, SCORE_regex);
}

bool regex::TOTAL_check(const std::string& s)
{
    boost::regex TOTAL_regex("^TOTAL([NESW]\\d+){4}\\r\\n$");
    return boost::regex_match(s, TOTAL_regex);
}

std::vector<std::string> regex::extract_cards(const std::string& s)
{
    boost::regex card_regex("([2-9]|1[0]|J|Q|K|A)[CDHS]");
    boost::sregex_iterator card_iterator(s.begin(), s.end(), card_regex);
    boost::sregex_iterator end;
    std::vector<std::string> cards;
    for (; card_iterator != end; ++card_iterator)
    {
        cards.push_back(card_iterator->str());
    }
    return cards;
}

std::string regex::extract_trick_nr(const std::string& s)
{
    boost::regex trick_nr_regex("TRICK(?:[1-9]|1[0-3])");
    boost::sregex_iterator trick_nr_iterator(s.begin(),
        s.end(), trick_nr_regex);
    return trick_nr_iterator->str();
}

std::vector<std::string> regex::extract_seat_score(const std::string& s)
{
    boost::regex seat_score_regex("([NESW]\\d+)");
    boost::sregex_iterator seat_score_iterator(s.begin(), s.end(),
        seat_score_regex);
    boost::sregex_iterator end;
    std::vector<std::string> seat_scores;
    for (; seat_score_iterator != end; ++seat_score_iterator)
    {
        seat_scores.push_back(seat_score_iterator->str());
    }
    return seat_scores;
}