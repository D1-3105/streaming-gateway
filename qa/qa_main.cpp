#include "../src/logging.h"
#include "test_shared_memory_manager.h"
//
// Created by oleg on 22.04.24.
//
int main() {
    logging::init_logging();
    TestSharedMemoryManager test;
    test.runTests();

    std::cout << "All tests passed successfully" << std::endl;

    return 0;
}