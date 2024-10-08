#include "points_calculator.h"

PointsCalculator::PointsCalculator(const vector<string>& played_cards,
    const string& starter, int16_t trick_type, int16_t trick)
    : trick_type{trick_type}, trick{trick}, starter{starter},
    seats{"N", "E", "S", "W" }, seats_mapping{{ "N", 0 }, { "E", 1 },
    { "S", 2 }, { "W", 3 }}
{
    for (int16_t i = 0; i < 4; i++)
    {
        int16_t beginning = seats_mapping[starter];
        colors[(beginning + i) % 4] = played_cards[i]
            .substr(played_cards[i].size() - 1, 1);
        string figure = played_cards[i]
            .substr(0, played_cards[i].size() - 1);
        if (figure == "J") { figures[(beginning + i) % 4] = 11; }
        else if (figure == "Q") { figures[(beginning + i) % 4] = 12; }
        else if (figure == "K") { figures[(beginning + i) % 4] = 13; }
        else if (figure == "A") { figures[(beginning + i) % 4] = 14; }
        else { figures[(beginning + i) % 4] = std::stoi(figure); }
    }
}

pair<string, int32_t> PointsCalculator::calculate_points()
{
    string taker = find_taker();
    if (trick_type == 1) { return no_tricks(taker); }
    if (trick_type == 2) { return no_hearts(taker); }
    if (trick_type == 3) { return no_queens(taker); }
    if (trick_type == 4) { return no_misters(taker); }
    if (trick_type == 5) { return no_hearts_king(taker); }
    if (trick_type == 6) { return no_seventh_last_trick(taker); }
    if (trick_type == 7) { return bandit(taker); }
    return pair<string, int32_t> {taker, -1};
}

string PointsCalculator::find_taker()
{
    string starting_color = colors[seats_mapping[starter]];
    int16_t starting_figure = figures[seats_mapping[starter]];
    string taker = starter;
    int max_figure = starting_figure;
    for (int16_t i = 1; i < 4; i++)
    {
        int32_t current_figure = figures[(seats_mapping[starter] + i) % 4];
        if (colors[(seats_mapping[starter] + i) % 4] == 
            starting_color && current_figure > max_figure)
        {
            max_figure = current_figure;
            taker = seats[(seats_mapping[starter] + i) % 4];
        }
    }

    return taker;
}

pair<string, int32_t> PointsCalculator::no_tricks(const string& taker)
{
    return pair<string, int32_t>{taker, 1};
}

pair<string, int32_t> PointsCalculator::no_hearts(const string& taker)
{
    int32_t points = 0;
    for (int16_t i = 0; i < 4; i++)
    {
        if (colors[i] == "H") { points += 1; }
    }

    return pair<string, int32_t>{taker, points};
}

pair<string, int32_t> PointsCalculator::no_queens(const string& taker)
{
    int16_t points = 0;
    for (int16_t i = 0; i < 4; i++)
    {
        if (figures[i] == 12) { points += 5; }
    }

    return pair<string, int32_t>{taker, points};
}

pair<string, int32_t> PointsCalculator::no_misters(const string& taker)
{
    int32_t points = 0;
    for (int16_t i = 0; i < 4; i++)
    {
        if (figures[i] == 11 || figures[i] == 13) { points += 2; }
    }

    return pair<string, int32_t>{taker, points};
}

pair<string, int32_t> PointsCalculator::no_hearts_king(const string& taker)
{
    int32_t points = 0;
    for (int16_t i = 0; i < 4; i++)
    {
        if (colors[i] == "H" && figures[i] == 13) { points += 18; }
    }

    return pair<string, int32_t>{taker, points};
}

pair<string, int32_t> PointsCalculator::no_seventh_last_trick
    (const string& taker)
{
    int16_t points = 0;
    for (int16_t i = 0; i < 4; i++)
    {
        if (trick == 7 || trick ==  13) { points += 10; }
    }

    return pair<string, int32_t>{taker, points};
}

pair<string, int32_t> PointsCalculator::bandit(const string& taker)
{
    int32_t points = 0;
    points += no_tricks(taker).second;
    points += no_hearts(taker).second;
    points += no_queens(taker).second;
    points += no_misters(taker).second;
    points += no_hearts_king(taker).second;
    points += no_seventh_last_trick(taker).second;
    return pair<string, int32_t> { taker, points };
}
