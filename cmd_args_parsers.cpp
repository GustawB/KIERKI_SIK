#include "cmd_args_parsers.h"

using std::string;
using std::vector;
using std::exception;
using std::invalid_argument;
using std::pair;

namespace po = boost::program_options;

int16_t parser::parse_server_args(int argc, char* argv[], int32_t& port, 
    string& game_file_name, int32_t& timeout)
{
    try 
    {
        // Manually parse command line arguments
        vector<string> args(argv + 1, argv + argc);
        vector<string> manual_args;
        vector<string> seat_order;
        vector<string> ip_order;
        bool b_is_arg = false;
        for (const string& arg : args)
        {
            if (arg[0] == '-') 
            {
                b_is_arg = true;
                manual_args.push_back(arg);
            }
            else if (b_is_arg)
            {
                b_is_arg = false;
                manual_args.push_back(arg);
            }
            else { throw invalid_argument("Invalid argument: " + arg); }
        }

        po::options_description desc("Allowed options");
        desc.add_options()
            (",p", po::value<vector<int32_t>>()->multitoken(), "port number")
            (",f", po::value<vector<string>>()->multitoken(), "game file name")
            (",t", po::value<vector<int32_t>>()->multitoken(), "timeout");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
        po::notify(vm);

        if (vm.count("-p")) 
        {
            port = vm["-p"].as<vector<int32_t>>()[0];
            if (port < 0) 
            {
                throw invalid_argument("Port number must be non-negative");
            }
        }

        if (vm.count("-f")) { game_file_name = vm["-f"]
            .as<vector<string>>()[0]; }
        else { throw invalid_argument("Game file name must be provided"); }

        if (vm.count("-t")) 
        {
            timeout = vm["-t"].as<vector<int32_t>>()[0];
            if (timeout < 0) 
            {
                throw invalid_argument("Timeout must be non-negative");
            }
        }
    }
    catch(exception& e) 
    {
        common::print_error(e.what());
        return 1;
    }
    catch(...) 
    {
        common::print_error("Exception of unknown type!");
        return 1;
    }

    // Check if file exists.
    if (FILE* file = fopen(game_file_name.c_str(), "r")) { fclose(file); }
    else 
    {
        common::print_error("Game file does not exist.");
        return 1;
    }
    return 0;
}

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace po = boost::program_options;
using namespace std;

int16_t parser::parse_client_args(int argc, char* argv[], string& host, 
    int32_t& port_number, int16_t& IP_v, string& seat, bool& is_AI)
{
    try
    {
        // Manually parse command line arguments
        vector<string> args(argv + 1, argv + argc);
        vector<string> manual_args;
        vector<string> seat_order;
        vector<string> ip_order;
        bool b_is_arg = false;
        for (const string& arg : args)
        {
            if (arg == "-4" || arg == "-6")
            {
                ip_order.push_back(arg);
                b_is_arg = false;
            }
            else if (arg == "-N" || arg == "-E" || arg == "-S" || arg == "-W")
            {
                // Store the seat flag without the '-'
                seat_order.push_back(arg.substr(1));
                b_is_arg = false;
            }
            else if (arg[0] == '-') 
            {
                b_is_arg = true;
                // Keep non-seat flags for Boost to handle
                manual_args.push_back(arg);
            }
            else if (b_is_arg)
            {
                b_is_arg = false;
                // Keep non-seat flags for Boost to handle
                manual_args.push_back(arg);
            }
            else { throw invalid_argument("Invalid argument: " + arg); }
        }

        if (seat_order.size() > 0) {seat = seat_order[0];}
        else {throw invalid_argument("Seat must be provided");}
        if (ip_order.size() > 0) { IP_v = ip_order[0] == "-4" ? 4 : 6; }

        po::options_description desc("Allowed options");
        desc.add_options()
            (",h", po::value<vector<string>>()->multitoken(), "host name")
            (",p", po::value<vector<int32_t>>()->multitoken(), "port number")
            (",a", po::value<vector<bool>>()->zero_tokens()
                ->composing(), "AI");

        po::variables_map vm;
        // Parse remaining arguments with Boost
        auto parsed = po::command_line_parser(manual_args).options(desc).run();

        po::store(parsed, vm);
        po::notify(vm);

        if (vm.count("-h") && !vm["-h"].as<vector<string>>().empty()) 
        {
            host = vm["-h"].as<vector<string>>()[0];
        }
        else 
        {
            throw invalid_argument("Host name must be provided");
        }

        if (vm.count("-p") && !vm["-p"].as<vector<int32_t>>().empty()) 
        {
            if (vm["-p"].as<vector<int32_t>>()[0] < 0) 
            {
                throw invalid_argument("Port number must be non-negative");
            }
            port_number = vm["-p"].as<vector<int32_t>>()[0];
        }
        else { throw invalid_argument("Port number must be provided"); }

        // Count occurrences of -a
        if (vm.count("-a")) { is_AI = true; }
    }
    catch(const exception& e) 
    {
        common::print_error(e.what());
        return 1;
    }
    catch(...) 
    {
        common::print_error("Exception of unknown type!");
        return 1;
    }

    return 0;
}
