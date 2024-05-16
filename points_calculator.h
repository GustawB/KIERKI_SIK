#ifndef POINTS_CALCULATOR_H
#define POINTS_CALCULATOR_H

#include <iostream>
#include <array>
#include <string>
#include <utility>
#include <map>

using std::array;
using std::string;
using std::pair;

class PointsCalculator 
{
public:
    PointsCalculator() = delete;
    PointsCalculator(const array<string, 4>& played_cards, const string& starter, int16_t hand, int16_t trick);
    ~PointsCalculator() = default;

    pair<string, int16_t> calculate_points();

private:
    string find_taker();
    pair<string, int16_t> no_tricks(const string& taker);
    pair<string, int16_t> no_hearts(const string& taker);
    pair<string, int16_t> no_queens(const string& taker);
    pair<string, int16_t> no_misters(const string& taker);
    pair<string, int16_t> no_hearts_king(const string& taker);
    pair<string, int16_t> no_seventh_last_trick(const string& taker);
    pair<string, int16_t> bandit(const string& taker);

    int16_t trick_type;
    int16_t trick;
    string starter;
    array<string, 4> colors;
    array<int16_t, 4> figures;
    array<string, 4> seats;
    std::map<string, int> seats_mapping;
};
#endif // POINTS_CALCULATOR_H