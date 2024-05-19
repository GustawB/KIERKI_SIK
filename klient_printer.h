#include <iostream>
#include <string>
#include <vector>

#include "regex.h"

namespace client_printer
{
    using std::cout;
    using std::string;
    using std::vector;

    void print_busy(const string& s);
    void print_deal(const string& s);
    void print_wrong(int16_t trick_nr);
    void print_taken(const string& s);
    void print_score(const string& s);
    void print_total(const string& s);
    void print_trick(const string& s, int16_t trick_nr, vector<string> my_cards);
    void print_my_cards(const vector<string>& my_cards);
    void print_my_tricks(const vector<vector<string>>& taken_tricks);

} // namespace client_printer
