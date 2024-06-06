#include "klient_printer.h"

void client_printer::print_busy(const std::string& s)
{
    cout << "Place busy, list of busy places received:";
    string seats = s.substr(4, s.size() - 6);
    bool b_is_first = true;
    for (char c : seats) 
    {
        if (b_is_first) 
        {
            cout << " " << c;
            b_is_first = false;
        }
        else
        {
            cout << ", " << c;
        }
    }
    cout << ".\n";
}

void client_printer::print_deal(const std::string& s)
{
    char trick_type = s[4];
    char starter_seat = s[5];
    string cards = s.substr(6, s.size() - 8);
    cout << "New deal " << trick_type << ": " << "staring place: "
        << starter_seat << ", your cards:";
    bool b_is_first = true;
    for (const string& card : regex::extract_cards(cards)) 
    { 
        if (b_is_first) 
        {
            cout << " " << card;
            b_is_first = false;
        }
        else
        {
            cout << ", " << card;
        }
    }
    cout << ".\n";
}

void client_printer::print_wrong(int16_t trick_nr)
{
    cout << "Wrong message received in trick " << trick_nr << ".\n";
}

void client_printer::print_taken(const std::string& s, int16_t trick_nr)
{
    string cards;
    if (trick_nr < 10) { cards = s.substr(6, s.size() - 9); }
    else { cards = s.substr(7, s.size() - 10); }
    char taker = s[s.size() - 3];
    cout << "A trick " << trick_nr << " is taken by " << taker << ", cards";
    bool b_is_first = true;
    for (const string& card : regex::extract_cards(cards)) 
    {
        if (b_is_first) 
        {
            cout << " " << card;
            b_is_first = false;
        }
        else
        {
            cout << ", " << card;
        }
    }
    cout << ".\n";
}

void client_printer::print_score(const std::string& s)
{
    string scores = s.substr(5, s.size() - 7);
    cout << "The scores are:\n";
    for (const string& score : regex::extract_seat_score(scores)) 
    {
         cout << score[0] << " | " 
            << score.substr(1, score.size() - 1) << "\n";
    }
}

void client_printer::print_total(const std::string& s)
{
    string scores = s.substr(5, s.size() - 7);
    cout << "The total scores are:\n";
    for (const string& score : regex::extract_seat_score(scores))
    {
        cout << score[0] << " | " << score.substr(1, score.size() - 1) << "\n";
    }
}

void client_printer::print_trick(const std::string& s,
    int16_t trick_nr, vector<string> my_cards)
{
    string cards;
    if (trick_nr < 10) { cards = s.substr(6, s.size() - 8); }
    else { cards = s.substr(7, s.size() - 9); }
    cout << "Trick: (" << trick_nr << ")";
    bool b_is_first = true;
    for (const string& card : regex::extract_cards(cards)) 
    { 
        if (b_is_first) 
        {
            cout << " " << card;
            b_is_first = false;
        }
        else
        {
            cout << ", " << card;
        }
    }
    b_is_first = true;
    cout << "\nAvailable cards:";
    for (const string& card : my_cards) 
    {
        if (b_is_first) 
        {
            cout << " " << card;
            b_is_first = false;
        }
        else
        {
            cout << ", " << card;
        }   
    }
    cout << "\n";
}

void client_printer::print_my_cards(const vector<string>& my_cards)
{
    bool b_is_first = true;
    for (const string& card : my_cards)
    {
        if (b_is_first)
        {
            cout << card;
            b_is_first = false;
        }
        else
        {
            cout << ", " << card;
        }
    }
    cout << ".\n";
}

void client_printer::print_my_tricks
    (const vector<vector<string>>& taken_tricks)
{
    for (const vector<string>& trick : taken_tricks)
    {
        bool b_is_first = true;
        for (const string& card : trick)
        {
            if (b_is_first)
            {
                cout << card;
                b_is_first = false;
            }
            else
            {
                cout << ", " << card;
            }
        }
        cout << "\n";
    }
}