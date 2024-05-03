#include "cli.h"
#include "logging.h"
#include "http.h"
#include "ipc_message_queue.h"
#include <stdexcept> // For std::runtime_error
#include <cstring> // For std::strerror

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
        else if (execMode == "streamer")
        {
            const std::string executable = commandLineArguments.find("exe_path")->second;
            const std::string region = commandLineArguments.find("region_name")->second;
            const std::string shm_key = commandLineArguments.find("shm_key")->second;
            int byte_size = sizeof(ipc_queue::Message);
            // Prepare arguments for execv
            char* args[] = {
                    const_cast<char*>("--region"),
                    const_cast<char*>(region.c_str()),
                    const_cast<char*>("--shm_key"),
                    const_cast<char*>(shm_key.c_str()),
                    const_cast<char*>("--region_byte_size"),
                    const_cast<char*>(std::to_string(byte_size).c_str()),
                    nullptr
            };

            // Execute the specified executable with the arguments
            if (execv(executable.c_str(), args) == -1) {
                // If execv returns, it means an error occurred
                std::string errorMessage = std::strerror(errno);
                throw std::runtime_error("Error during process <"+executable+"> creation. Execv failed: " + errorMessage);
            }
        }
        else if (execMode == "video_handler")
        {
            const std::string executable = commandLineArguments.find("exe_path")->second;
            const std::string region = commandLineArguments.find("region_name")->second;
            const std::string shm_key = commandLineArguments.find("shm_key")->second;
            const std::string video_repository = commandLineArguments.find("video_repository")->second;

            int byte_size = sizeof(ipc_queue::Message);

            char* args[] = {
                    const_cast<char*>("--region"),
                    const_cast<char*>(region.c_str()),
                    const_cast<char*>("--shm_key"),
                    const_cast<char*>(shm_key.c_str()),
                    const_cast<char*>("--region_byte_size"),
                    const_cast<char*>(std::to_string(byte_size).c_str()),
                    const_cast<char*>("--video_repository"),
                    const_cast<char*>((video_repository).c_str()),
                    nullptr
            };
            // Execute the specified executable with the arguments
            if (execv(executable.c_str(), args) == -1) {
                // If execv returns, it means an error occurred
                std::string errorMessage = std::strerror(errno);
                throw std::runtime_error("Error during process <"+executable+"> creation. Execv failed: " + errorMessage);
            }
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
