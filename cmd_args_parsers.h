#ifndef CMD_ARGS_PARSERS_H
#define CMD_ARGS_PARSERS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "common.h"

namespace parser
{
    using std::string;
    using std::vector;
    using std::exception;
    using std::invalid_argument;
    using std::cout;
    using std::cerr;
    

    namespace po = boost::program_options;

    int parse_server_args(int argc, char* argv[], int& port, string& game_file_name, int& timeout);
    int parse_client_args(int argc, char* argv[], string& host, int& port_number, int& IP_v, string& seat, bool& is_AI);
} // namespace parser

#pragma GCC diagnostic pop

#endif // CMD_ARGS_PARSERS_H