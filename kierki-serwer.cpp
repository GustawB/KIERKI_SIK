#include <iostream>
#include <iterator>
#include <exception>
#include <string>

#include "serwer.h"
#include "cmd_args_parsers.h"
#include "file_reader.h"

using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::exception;
using std::invalid_argument;

int main(int argc, char* argv[])
{
    file_reader::FileReader fr("deals.txt");
    while(fr.read_next_deal() > 0)
    {
        cout << "Trick type: " << fr.get_trick_type() << "\n";
        cout << "Seat: " << fr.get_seat() << "\n";
        for (const auto& card : fr.get_cards())
        {
            cout << card << "\n";
        }
    }  
    cout << "End of file.\n";
    /*int port = 0;
    string game_file_name;
    int timeout = 5;
    
    parser::parse_server_args(argc, argv, port, game_file_name, timeout);

    serwer::Serwer s(port, timeout, game_file_name);
    s.start_game();
    return 0;*/
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
