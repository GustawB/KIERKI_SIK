#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <exception>
#include <string>

#include "serwer.h"

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
    try 
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            (",p", po::value<vector<int>>()->multitoken(), "port number")
            (",f", po::value<vector<string>>()->multitoken(), "game file name")
            (",t", po::value<vector<int>>()->multitoken(), "timeout");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("-p")) 
        {
            port = vm["-p"].as<vector<int>>()[0];
            if (port < 0) 
            {
                throw invalid_argument("Port number must be non-negative");
            }
        }

        if (vm.count("-f")) 
        {
            game_file_name = vm["-f"].as<vector<string>>()[0];
        }
        else 
        {
            throw invalid_argument("Game file name must be provided");
        }

        if (vm.count("-t")) 
        {
            timeout = vm["-t"].as<vector<int>>()[0];
            if (timeout < 0) 
            {
                throw invalid_argument("Timeout must be non-negative");
            }
        }
    }
    catch(exception& e) 
    {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) 
    {
        cerr << "Exception of unknown type!\n";
    }

    serwer::Serwer s(port, timeout, game_file_name);
    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
