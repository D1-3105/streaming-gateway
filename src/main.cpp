#include "cli.h"
#include "logging.h"
#include "http.h"
#include "ipc_message_queue.h"
#include <stdexcept> // For std::runtime_error
#include <cstring> // For std::strerror
#include <csignal> // signal
// libboost
#include <boost/process.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


SharedMemoryManager* shm_manager;

boost::process::child* streamer_process;
boost::process::child* handler_process;
std::string shm_key;
std::string video_repository;


void handle_interruption_host(int sig)
{
    BOOST_LOG_TRIVIAL(info) << "Interrupted main thread;";
    try
    {
        if (streamer_process)
            streamer_process->terminate();
    }
    catch (boost::process::process_error& e)
    {
        BOOST_LOG_TRIVIAL(error) << "Streamer error during terminate: " << e.what();
    }

    try
    {
        if(handler_process)
            handler_process->terminate();
    }
    catch (boost::process::process_error& e)
    {
        BOOST_LOG_TRIVIAL(error) << "Handler error during terminate: " << e.what();
    }

    shm_manager->UnregisterAll();
    shm_unlink(shm_key.c_str());
    exit(sig);
}


void main_server(const char* executed_with, const std::map<std::string, std::string>& commandLineArguments)
{
    const std::string streamer = commandLineArguments.find("streamer")->second;
    const std::string handler = commandLineArguments.find("handler")->second;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    shm_key = boost::uuids::to_string(uuid);
    video_repository = commandLineArguments.find("video_repository")->second;
    assert(std::filesystem::is_directory(video_repository));
    assert(std::filesystem::exists(executed_with));
    assert(std::filesystem::exists(streamer));
    assert(std::filesystem::exists(handler));

    assert((std::filesystem::status(executed_with).permissions() & std::filesystem::perms::owner_exec) == std::filesystem::perms::owner_exec);
    assert((std::filesystem::status(streamer).permissions() & std::filesystem::perms::owner_exec) == std::filesystem::perms::owner_exec);
    assert((std::filesystem::status(handler).permissions() & std::filesystem::perms::owner_exec) == std::filesystem::perms::owner_exec);

    size_t byte_size = 50 * 1024 * 1024; // buffer - 50mb

    shm_manager = new SharedMemoryManager();
    shm_manager->RegisterSystemSharedMemory("shm_queue", shm_key, 0, byte_size);

    std::signal(SIGINT, handle_interruption_host);
    std::signal(SIGABRT, handle_interruption_host);
    std::signal(SIGTERM, handle_interruption_host);

    try
    {
        std::vector<std::string> handler_args {
            "--exec_mode=handler",
            std::string("--exe_path=") + handler,
            std::string("--shm_key=") + shm_key,
            std::string("--region_name=shm_queue"),
            std::string("--video_repository=") + video_repository,
            std::string("--shm_size=") + std::to_string(byte_size)
        };
        std::ostringstream handler_command;
        handler_command << "Starting handler subprocess with command: " << executed_with;
        handler_command << " ";
        for (const auto& arg : handler_args) {
            handler_command << "=" << arg;
        }
        BOOST_LOG_TRIVIAL(info) << handler_command.str();
        handler_process = new boost::process::child(
                executed_with, handler_args
        );

        std::vector<std::string> streamer_args {
                "--exec_mode=streamer",
                std::string("--exe_path=") + streamer,
                std::string("--shm_key=") + shm_key,
                std::string("--region_name=shm_queue"),
                std::string("--shm_size=") + std::to_string(byte_size)
        };

        std::ostringstream streamer_command;
        streamer_command << "Starting streamer subprocess with command: " << executed_with;
        streamer_command << " ";
        for (const auto& arg : streamer_args) {
            streamer_command << " " << arg;
        }
        BOOST_LOG_TRIVIAL(info) << streamer_command.str();

        streamer_process = new boost::process::child(
                executed_with, streamer_args
        );

        int port = 18081;

        pthread_t httpServerThread;
        httpServer::ServerArguments args{video_repository, port};
        pthread_create(
                &httpServerThread,
                nullptr,
                httpServer::serveApp,
                (void*)(&args)
        );

        handler_process->join();
        streamer_process->join();

        pthread_join(httpServerThread, nullptr);
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << "Exception in main thread: " << e.what();
        handle_interruption_host(-1);
    }

    shm_manager->UnregisterAll();
    handle_interruption_host(0);
}

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
            main_server(argv[0], commandLineArguments);
        }
        else if (execMode == "streamer")
        {
            const std::string executable = commandLineArguments.find("exe_path")->second;
            const std::string region = commandLineArguments.find("region_name")->second;
            shm_key = commandLineArguments.find("shm_key")->second;
            int byte_size = std::stol(commandLineArguments.find("shm_size")->second);
            // Prepare arguments for execv

            std::vector<std::string> args {
                    std::string("--shm_key=") + shm_key,
                    std::string("--shm_size=") + std::to_string(byte_size)
            };
            std::ostringstream cmd;
            cmd << executable;
            for (const auto& arg : args) {
                cmd << " " << arg;
            }
            BOOST_LOG_TRIVIAL(info) << cmd.str();

            char** arg_ptr = new char*[args.size() + 2];
            for (size_t i = 0; i < args.size(); ++i) {
                arg_ptr[i + 1] = const_cast<char*>(args[i].c_str());
            }
            arg_ptr[args.size() + 1] = nullptr;

            // Execute the specified executable with the arguments
            if (execv(executable.c_str(), arg_ptr) == -1) {
                // If execv returns, it means an error occurred
                std::string errorMessage = std::strerror(errno);
                throw std::runtime_error("Error during process <"+executable+"> creation. Execv failed: " + errorMessage);
            }
        }
        else if (execMode == "handler")
        {
            const std::string executable = commandLineArguments.find("exe_path")->second;
            const std::string region = commandLineArguments.find("region_name")->second;
            shm_key = commandLineArguments.find("shm_key")->second;
            video_repository = commandLineArguments.find("video_repository")->second;
            int byte_size = std::stol(commandLineArguments.find("shm_size")->second);

            std::vector<std::string> args {
                    std::string("--shm_key=") + shm_key,
                    std::string("--shm_size=") + std::to_string(byte_size),
                    std::string("--video_repository=") + video_repository
            };
            std::ostringstream cmd;
            cmd << executable;
            for (const auto& arg : args) {
                cmd << " " << arg;
            }
            BOOST_LOG_TRIVIAL(info) << cmd.str();

            char** arg_ptr = new char*[args.size() + 2];
            for (size_t i = 0; i < args.size(); ++i) {
                arg_ptr[i + 1] = const_cast<char*>(args[i].c_str());
            }
            arg_ptr[args.size() + 1] = nullptr;

            // Execute the specified executable with the arguments
            if (execv(executable.c_str(), arg_ptr) == -1) {
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
