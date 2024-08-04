#include "cli.h"

std::map<std::string, std::string> cli::parseArgs(int argc, char *argv[]) {
    std::map<std::string, std::string> parsedArgs;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0, 2) == "--") {
            // If argument starts with "--", it's considered a key-value pair
            size_t pos = arg.find('=');
            if (pos != std::string::npos) {
                // Split key and value
                std::string key = arg.substr(2, pos - 2);
                std::string value = arg.substr(pos + 1);
                parsedArgs[key] = value;
            } else {
                // If no '=' found, treat it as a flag
                parsedArgs[arg.substr(2)] = "";
            }
        } else if (arg.substr(0, 1) == "-") {
            // If argument starts with "-", it's considered a short option
            // For simplicity, we're not handling short options in this example
            std::cerr << "Short options are not supported in this example." << std::endl;
        } else {
            // Otherwise, treat it as a positional argument
            parsedArgs["arg" + std::to_string(i)] = arg;
        }
    }
    return parsedArgs;
}
