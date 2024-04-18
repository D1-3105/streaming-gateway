#include "../include/crow/crow_all.h"
#include "cli.h"
#include "logging.h"
#include "http.h"

int main(int argc, char *argv[])
{

    logging::init_logging();

    std::map<std::string, std::string> commandLineArguments = cli::parseArgs(argc, argv);

    auto execModeIter = commandLineArguments.find("exec_mode");
    if (execModeIter != commandLineArguments.end())
    {
        std::string execMode = execModeIter->second;


        if (execMode == "http")
        {
            httpServer::serveApp(18081);
        }
        else
        {
            BOOST_LOG_TRIVIAL(error) << "Error: Unsupported execution mode '" << execMode << "'" << std::endl;
        }
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Error: Missing argument 'exec_mode'" << std::endl;
    }

    return 0;
}
