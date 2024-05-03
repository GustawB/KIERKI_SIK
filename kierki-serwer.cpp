#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <string>

#include "serwer.h"

using std::cout;
using std::cerr;
using std::string;
using std::vector;

int main(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        (",p", po::value<vector<int>>()->multitoken(), "port number")
        (",f", po::value<vector<string>>()->multitoken(), "file name")
        (",t", po::value<vector<int>>()->multitoken(), "timeout");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    serwer::Serwer s;

    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
