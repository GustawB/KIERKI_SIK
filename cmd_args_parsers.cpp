#include "cmd_args_parsers.h"

using std::string;
using std::pair;

void parser::parse_server_args(int argc, char* argv[], int& port, string& game_file_name, int& timeout)
{
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
        exit(1);
    }
    catch(...) 
    {
        cerr << "Exception of unknown type!\n";
    }
}

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

void parser::parse_client_args(int argc, char* argv[], string& host, int& port_number, int& IP_v, string& seat, bool& is_AI)
{
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
            host = vm["-h"].as<vector<string>>()[0];
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
            port_number = vm["-p"].as<vector<int>>()[0];
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
            is_AI = true;
        }

        if (vm.count("IP")) 
        {
            IP_v = vm["IP"].as<vector<int>>()[0];
        }
    }
    catch(exception& e) 
    {
        cerr << "error: " << e.what() << "\n";
        exit(1);
    }
    catch(...) 
    {
        cerr << "Exception of unknown type!\n";
    }
}