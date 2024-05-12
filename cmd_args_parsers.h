#ifndef CMD_ARGS_PARSERS_H
#define CMD_ARGS_PARSERS_H

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <exception>

namespace parser
{
    using std::string;
    using std::vector;
    using std::exception;
    using std::invalid_argument;
    using std::cout;
    using std::cerr;
    

    namespace po = boost::program_options;

    void parse_server_args(int argc, char* argv[], int& port, string& game_file_name, int& timeout);
    void parse_client_args(int argc, char* argv[], string& host, int& port_number, int& IP_v, string& seat, bool& is_AI);
} // namespace parser

#endif // CMD_ARGS_PARSERS_H