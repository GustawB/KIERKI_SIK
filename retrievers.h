#ifndef RETRIEVERS_H
#define RETRIEVERS_H

#include <string>
#include <vector>
#include <map>

#include "regex.h"

namespace retrievers
{
    using std::string;
    using std::vector;
    using std::map;
    using std::move;

    struct deal_result
    {
        deal_result() = default;
        deal_result(deal_result&& other)
            : deal_type(other.deal_type), start_seat(move(other.start_seat)), cards(move(other.cards)) {}
        ~deal_result() = default;

        int8_t deal_type;
        std::string start_seat;
        std::vector<std::string> cards;
    };

    struct trick_result
    {
        trick_result() = default;
        trick_result(trick_result&& other)
            : trick_type(other.trick_type), cards(move(other.cards)) {}
        ~trick_result() = default;

        int8_t trick_type;
        std::vector<std::string> cards;
    };

    struct taken_result
    {
        taken_result() = default;
        taken_result(taken_result&& other)
            : trick_type(other.trick_type), cards(move(other.cards)), taking_seat(move(other.taking_seat)) {}
        ~taken_result() = default;

        int8_t trick_type;
        std::vector<std::string> cards;
        std::string taking_seat;
    };

    string retrieve_iam(const string& message);
    string retrieve_busy(const string& message);
    deal_result retrieve_deal(const string& message);
    trick_result retrieve_trick(const string& message);
    int8_t retrieve_wrong(const string& message);
    taken_result retrieve_taken(const string& message);
    map<string, int32_t> retrieve_score(const string& message);
    map<string, int32_t> retrieve_total(const string& message);
} // namespace retrievers

#endif // RETRIEVERS_H