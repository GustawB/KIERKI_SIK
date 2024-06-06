#ifndef REGEX_H
#define REGEX_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <boost/regex.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace regex
{
    using std::string;
    using std::vector;
    using std::to_string;

    bool IAM_check(const string& s);
    bool BUSY_check(const string& s);
    bool DEAL_check(const string& s);
    bool TRICK_check(const string& s, int16_t trick_nr);
    bool TRICK_client_check(const string& s);
    bool WRONG_check(const string& s);
    bool TAKEN_check(const string& s, int16_t trick_nr);
    bool SCORE_check(const string& s);
    bool TOTAL_check(const string& s);

    /*
    * Extracts the cards from a string containing
    * a sequence of cards (ONLY CARDS).
    */
    vector<string> extract_cards(const string& s);

    /*
    * Extracts the trick number from a string containing
    * a trick message.
    */
    string extract_trick_nr(const string& s);

    /*
    * Extracts the seat score from a string containing
    * a score message (ONLY SEAT-SCORE part).
    */
    vector<string> extract_seat_score(const string& s);
} // namespace regex

#pragma GCC diagnostic pop

#endif // REGEX_H