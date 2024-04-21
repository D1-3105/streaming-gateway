#include <iostream>
#include <mutex>

#include "../src/shared_memory_manager.h"
#include "../src/logging.h"


class TestSharedMemoryManager {
public:
    void runTests() {
        test_register_and_unregister();
        test_get_memory_info();
    }

private:
    std::mutex lock;
    SharedMemoryManager* manager;

    void setUp() {
        lock.lock();
        manager = SharedMemoryManager_new();
    }

    void tearDown() {
        SharedMemoryManager_delete(manager);
        lock.unlock();
    }

    void test_register_and_unregister() {
        setUp();

        const char* name = "test_shm11";
        const char* shm_key = "test_key32";
        int offset = 0;
        int byte_size = 1024;

        std::cout << "RegisterSystemSharedMemory in test_register_and_unregister" << std::endl;

        RegisterSystemSharedMemory(manager, name, shm_key, offset, byte_size);
        assert(!MutexState(manager));

        std::cout << "Unregister in test_register_and_unregister" << std::endl;
        Unregister(manager, name);
        assert(!MutexState(manager));

        tearDown();
    }

    void test_get_memory_info() {
        setUp();

        const char* name = "test_shm111";
        const char* shm_key = "test_key23";
        int offset = 0;
        int byte_size = 1024;

        std::cout << "RegisterSystemSharedMemory in test_get_memory_info" << std::endl;
        RegisterSystemSharedMemory(manager, name, shm_key, offset, byte_size);
        assert(!MutexState(manager));

        void* shm_mapped_addr = nullptr;

        std::cout << "GetMemoryInfo in test_get_memory_info" << std::endl;
        GetMemoryInfo(manager, name, offset, byte_size, &shm_mapped_addr);
        assert(!MutexState(manager));
        assert(shm_mapped_addr != nullptr);

        tearDown();
    }
};

int main() {
    logging::init_logging();
    TestSharedMemoryManager test;
    test.runTests();

    std::cout << "All tests passed successfully" << std::endl;

    return 0;
}
