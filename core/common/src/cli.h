#ifndef STREAMING_GATEWAY_CROW_CLI_H
#define STREAMING_GATEWAY_CROW_CLI_H
#include <iostream>
#include <vector>
#include <string>
#include "map"

namespace cli
{
    std::map<std::string, std::string> parseArgs(int argc, char *argv[]);
}
#endif //STREAMING_GATEWAY_CROW_CLI_H
