#include "../src/logging.h"
#include "test_shared_memory_manager.hpp"
#include "test_webcam_streamer_queue.hpp"

//
// Created by oleg on 22.04.24.
//
int main() {
    logging::init_logging();
    BOOST_LOG_TRIVIAL(info) << "===========TestSharedMemoryManager==============";
    TestSharedMemoryManager test;
    test.runTests();

    BOOST_LOG_TRIVIAL(info) << "===========TestWebcamStreamerQueue=============";

    TestWebcamStreamerQueue test1;
    test1.runTests();
    std::cout << "All tests passed successfully" << std::endl;

    return 0;
}
