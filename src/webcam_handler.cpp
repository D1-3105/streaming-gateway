//
// Created by oleg on 24.04.24.
//

#include <csignal>
#include "logging.h"
#include "cli.h"
#include "shared_memory_manager.h"
#include "WebCam.h"
#include "exporters.h"
#include "wc_daemon.h"

auto shared_manager = new SharedMemoryManager;
std::string region;


void dieHandler( int signum ) {
    if(shared_manager)
    {
        shared_manager->UnregisterAll();
    }
    exit(signum);
}


int main(int argc, char *argv[])
{
    logging::init_logging();
    std::map<std::string, std::string> commandLineArguments = cli::parseArgs(argc, argv);
    region = std::string("shm_queue");
    std::string shm_key = commandLineArguments.find("shm_key")->second;
    std::string q_byte_size = commandLineArguments.find("shm_size")->second;
    std::string video_repository = commandLineArguments.find("video_repository")->second;

    char *endptr;
    int byte_size = std::strtol(q_byte_size.c_str(), &endptr, 10);

    if (*endptr != '\0')
    {
        BOOST_LOG_TRIVIAL(error) << "error during byte size parsing: " << q_byte_size;
        throw std::exception();
    }

    shared_manager->RegisterSystemSharedMemory(region, shm_key, 0, byte_size);
    signal(SIGINT, dieHandler);
    signal(SIGTERM, dieHandler);

    wc_daemon::WebCamStreamDaemon wc_d(shared_manager, region.c_str());

    exporters::FileBaseHLSMessageHandler fb_hls_handler(
            &wc_d,
            10,
            video_repository.c_str()
    );
    while (true)
        fb_hls_handler.HandleMessages();  // threads here
}
