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
    int port = 0;
    string game_file_name;
    int timeout = 5;
    
    int result = parser::parse_server_args(argc, argv, port, game_file_name, timeout);
    if (result != 0) {return result;}

    Serwer s(port, timeout, game_file_name);
    if (s.start_game() == 0) {return s.run_game();}
    else {return 1;}
    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
