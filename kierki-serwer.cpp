#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <string>

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

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (vm.count("-f")) {
        vector<string> values = vm["-f"].as<vector<string>>();
        cout << "files:";
        for (const auto& value : values) {
            cout << " " << value;
        }
        cout << ".\n";
    } else {
        cerr << "File name was not set.\n";
    }

    if (vm.count("-p")) {
        vector<int> values = vm["-p"].as<vector<int>>();
        cout << "ports:";
        for (const auto& value : values) {
            cout << " " << value;
        }
        cout << ".\n";
    }

    if (vm.count("-t")) {
        vector<int> values = vm["-t"].as<vector<int>>();
        cout << "timeouts:";
        for (const auto& value : values) {
            cout << " " << value;
        }
        cout << ".\n";
    }

    return 0;
}

// g++ -I/home/gustaw/boost_library/boost_1_74_0 -std=c++20  kierki-serwer.cpp -o kierki -L/home/gustaw/boost_library/boost_1_74_0/stage/lib -lm -l:libboost_program_options.a
