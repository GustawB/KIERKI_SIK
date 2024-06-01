#ifndef POINTS_CALCULATOR_H
#define POINTS_CALCULATOR_H

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <utility>
#include <map>

using std::array;
using std::vector;
using std::string;
using std::pair;

/*
* This class should be initialized with cards on the table,
* the starter, the type of the trick and the number of the trick.
* Then, calling the calculate_points method will return the taker,
* and the points he got.
*/
class PointsCalculator 
{
public:
    PointsCalculator() = delete;
    PointsCalculator(const vector<string>& played_cards,
        const string& starter, int16_t hand, int16_t trick);
    ~PointsCalculator() = default;

    /*
    * This method calculates the points for the trick.
    * It returns the taker and the points he got.
    */
    pair<string, int32_t> calculate_points();

private:
    /*
    * This method finds the taker of the trick.
    * Used by every other method that calculates points
    * based on the taker.
    */
    string find_taker();
    pair<string, int32_t> no_tricks(const string& taker);
    pair<string, int32_t> no_hearts(const string& taker);
    pair<string, int32_t> no_queens(const string& taker);
    pair<string, int32_t> no_misters(const string& taker);
    pair<string, int32_t> no_hearts_king(const string& taker);
    pair<string, int32_t> no_seventh_last_trick(const string& taker);
    pair<string, int32_t> bandit(const string& taker);

    int16_t trick_type;
    int16_t trick;
    string starter;
    array<string, 4> colors;
    array<int16_t, 4> figures;
    array<string, 4> seats;
    std::map<string, int16_t> seats_mapping;
};
#endif // POINTS_CALCULATOR_H