#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <string>

using std::cout;
using std::cerr;
using std::string;
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

   

    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
