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
    // Poprawione złożone wyrażenie regularne
    //boost::regex TRICK_regex("TRICK(1[0-3]|[1-9])((2-9|10|J|Q|K|A)(C|D|H|S)){0-3}\\r\\n");
    boost::regex TRICK_regexa("TRICK(1[0-3]|[1-9])((2-9|10|J|Q|K|A)[CDHS]){0}\\r\\n");
    boost::regex TRICK_regexb("TRICK(1[0-3]|[1-9])((2-9|10|J|Q|K|A)[CDHS]){1}\\r\\n");
    boost::regex TRICK_regexc("TRICK(1[0-3]|[1-9])((2-9|10|J|Q|K|A)[CDHS]){2}\\r\\n");
    boost::regex TRICK_regexd("TRICK(1[0-3]|[1-9])((2-9|10|J|Q|K|A)[CDHS]){3}\\r\\n");

    return boost::regex_match(s, TRICK_regexa) || boost::regex_match(s, TRICK_regexb) || boost::regex_match(s, TRICK_regexc) || boost::regex_match(s, TRICK_regexd);
}

bool regex::TRICK_client_check(const std::string& s)
{
    boost::regex TRICK_regex("TRICK((1[0-3])|[1-9])([2-9]|10|[JQKA][CDHS]){0-3}\\r\\n");
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

std::vector<std::string> regex::extract_cards(const std::string& s)
{
    boost::regex card_regex("([2-9]|10|[JQKA][CDHS])");
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
    boost::regex trick_nr_regex("TRICK([1-9]|1[0-3])");
    boost::sregex_iterator trick_nr_iterator(s.begin(), s.end(), trick_nr_regex);
    return trick_nr_iterator->str();
}

std::vector<std::string> regex::extract_seat_score(const std::string& s)
{
    boost::regex seat_score_regex("([NESW]\\b\\d+\\b)");
    boost::sregex_iterator seat_score_iterator(s.begin(), s.end(), seat_score_regex);
    boost::sregex_iterator end;
    std::vector<std::string> seat_scores;
    for (; seat_score_iterator != end; ++seat_score_iterator)
    {
        seat_scores.push_back(seat_score_iterator->str());
    }
    return seat_scores;
}