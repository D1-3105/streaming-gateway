//
// Created by oleg on 22.04.24.
//

#include "WebCam.h"
#include "cli.h"

#include "shared_memory_manager.h"
#include "logging.h"
#include "wc_daemon.h"

#include <iostream>
#include <csignal>


auto shared_manager = new SharedMemoryManager;
std::string region;

void dieHandler( int signum ) {
    if(shared_manager)
    {
        shared_manager->UnregisterAll();
    }
    exit(signum);
}

struct PuttingThreadArgs {
    wc_daemon::WebCamStreamDaemon* wc_d;
    webcam::WebcamIterator* stream_iterator;
};


[[noreturn]] void* putter(void* args_ptr) {
    PuttingThreadArgs* args = static_cast<PuttingThreadArgs*>(args_ptr);
    wc_daemon::WebCamStreamDaemon& wc_d = *(args->wc_d);
    webcam::WebcamIterator& stream_iterator = *(args->stream_iterator);

    while (true) {
        wc_d.PutOnSHMQueue(&stream_iterator);
    }
}

int main(int argc, char *argv[]) {
    logging::init_logging();

    std::map<std::string, std::string> commandLineArguments = cli::parseArgs(argc, argv);
    region = "shm_queue";
    const std::string shm_key = commandLineArguments.find("shm_key")->second;
    const std::string q_byte_size = commandLineArguments.find("shm_size")->second;

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
    webcam::WebcamIterator stream_iterator;

    PuttingThreadArgs thread_args = {&wc_d, &stream_iterator};
    pthread_t putting_thread;
    pthread_create(&putting_thread, nullptr, putter, &thread_args);
    pthread_join(putting_thread, nullptr);

    delete shared_manager;
    return 0;
}
