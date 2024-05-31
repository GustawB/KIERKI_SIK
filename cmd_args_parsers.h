#ifndef CMD_ARGS_PARSERS_H
#define CMD_ARGS_PARSERS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include <exception>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "common.h"

namespace parser
{
    using std::string;

    /* Parses command line arguments for the server. */
    int16_t parse_server_args(int argc, char* argv[], int32_t& port, 
        string& game_file_name, int32_t& timeout);

    /* Parses command line arguments for the client. */
    int16_t parse_client_args(int argc, char* argv[], string& host, 
        int32_t& port_number, int16_t& IP_v, string& seat, bool& is_AI);
} // namespace parser

#pragma GCC diagnostic pop

#endif // CMD_ARGS_PARSERS_H