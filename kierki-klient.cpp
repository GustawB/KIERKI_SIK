#include <iostream>
#include <iterator>
#include <exception>
#include <string>

#include "klient.h"
#include "cmd_args_parsers.h"

using std::cout;
using std::cerr;
using std::string;
using std::exception;
using std::invalid_argument;
using std::vector;
using std::make_pair;
using std::pair;

int main(int argc, char* argv[])
{
    string host_name;
    int port = 0;
    int ip_version = -1;
    string seat;
    bool AI = false;

    parser::parse_client_args(argc, argv, host_name, port, ip_version, seat, AI);

    Klient klient(host_name, port, ip_version, seat, AI);
    return klient.run_client();
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
