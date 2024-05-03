#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <exception>
#include <string>

#include "klient.h"

using std::cout;
using std::cerr;
using std::string;
using std::exception;
using std::invalid_argument;
using std::vector;
using std::make_pair;
using std::pair;

pair<string, string> reg_additional_options(const string& s)
{
    if (s.find("-4") == 0) { return make_pair("IP", string("4")); } 
    else if (s.find("-6") == 0) { return make_pair("IP", string("6")); }
    else if (s.find("-N") == 0) { return make_pair("seat", string("N")); } 
    else if (s.find("-E") == 0) { return make_pair("seat", string("E")); } 
    else if (s.find("-S") == 0) { return make_pair("seat", string("S")); } 
    else if (s.find("-W") == 0) { return make_pair("seat", string("W")); } 
    else if (s.find("-a") == 0) { return make_pair("AI", string("a")); } 
    else { return make_pair(string(), string()); }
}

int main(int argc, char* argv[])
{
    string host_name;
    int port = 0;
    int ip_version = -1;
    string seat;
    bool AI = false;
    try
    {
        po::options_description desc("Allowed options");
        vector<string> custom{};
        desc.add_options()
            ("help", "produce help message")
            (",h", po::value<vector<string>>()->multitoken(), "host name")
            (",p", po::value<vector<int>>()->multitoken(), "port number")
            ("IP", po::value<vector<int>>()->multitoken(), "IP")
            ("seat", po::value<vector<string>>()->multitoken(), "seat")
            ("AI", po::value<vector<string>>()->multitoken(), "AI");
            

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).extra_parser(reg_additional_options).run(), vm);
        po::notify(vm);

        if (!vm.count("-h")) 
        {
            throw invalid_argument("Host name must be provided");
        }
        else 
        {
            host_name = vm["-h"].as<vector<string>>()[0];
        }

        if (!vm.count("-p")) 
        {
            throw invalid_argument("Port number must be provided");
        }
        else if (vm["-p"].as<vector<int>>()[0] < 0) 
        {
            throw invalid_argument("Port number must be non-negative");
        }
        else 
        {
            port = vm["-p"].as<vector<int>>()[0];
        }

        if (!vm.count("seat")) 
        {
            throw invalid_argument("Seat must be provided");
        }
        else 
        {
            seat = vm["seat"].as<vector<string>>()[0];
        }

        if (vm.count("AI")) 
        {
            AI = true;
        }

        if (vm.count("IP")) 
        {
            ip_version = vm["IP"].as<vector<int>>()[0];
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

    klient::Klient klient(host_name, port, ip_version, seat, AI);

    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
