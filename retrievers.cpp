#include "retrievers.h"

using std::string;

string retrievers::retrieve_iam(const string& message)
{
    return message.substr(3, 1);
}

string retrievers::retrieve_busy(const string& message)
{
    return message.substr(4, message.length() - 2 - 4);
}

retrievers::deal_result retrievers::retrieve_deal(const string& message)
{
    deal_result result;
    result.deal_type = message[4] - '0';
    result.start_seat = message.substr(5, 1);
    result.cards = regex::extract_cards(message.substr(6, message.length() - 2 - 6));
    return result;
}

retrievers::trick_result retrievers::retrieve_trick(const string& message)
{
    trick_result result;
    string trick = regex::extract_trick_nr(message);
    result.trick_type = stoi(trick);
    result.cards = regex::extract_cards(message.substr(5 + trick.length(), message.length() - 2 - 5 - trick.length()));
    return result;
}

int16_t retrievers::retrieve_wrong(const string& message)
{
    return message[5] - '0';
}

retrievers::taken_result retrievers::retrieve_taken(const string& message)
{
    taken_result result;
    string trick = regex::extract_trick_nr(message);
    result.trick_type = stoi(trick);
    result.cards = regex::extract_cards(message.substr(5 + trick.length(), message.length() - 3 - 5 - trick.length()));
    result.taking_seat = message.substr(message.length() - 3, 1);
    return result;
}

std::map<string, int32_t> retrievers::retrieve_score(const string& message)
{
    std::map<string, int32_t> result;
    vector<string> seat_scores = regex::extract_seat_score(message.substr(5, message.length() - 2 - 5));
    for (const string& seat_score : seat_scores)
    {
        result[seat_score.substr(0, 1)] = stoi(seat_score.substr(1));
    }
    return result;
}

std::map<string, int32_t> retrievers::retrieve_total(const string& message)
{
    std::map<string, int32_t> result;
    vector<string> seat_scores = regex::extract_seat_score(message.substr(5, message.length() - 2 - 5));
    for (const string& seat_score : seat_scores)
    {
        result[seat_score.substr(0, 1)] = stoi(seat_score.substr(1));
    }
    return result;
}