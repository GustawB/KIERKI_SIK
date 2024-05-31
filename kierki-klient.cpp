#include <iostream>
#include <iterator>
#include <exception>
#include <string>

#include "cmd_args_parsers.h"
#include "klient.h"

using std::cout;
using std::cerr;
using std::string;
using std::exception;
using std::invalid_argument;
using std::vector;
using std::make_pair;
using std::pair;

int main(int argc, char* argv[])
{
    string host_name;
    int32_t port = 0;
    int16_t ip_version = -1;
    string seat;
    bool AI = false;

    int16_t result = parser::parse_client_args(argc, argv, host_name,
        port, ip_version, seat, AI);
    if (result != 0) {return result;}

    Klient klient(host_name, port, ip_version, seat, AI);
    return klient.run_client();
}
