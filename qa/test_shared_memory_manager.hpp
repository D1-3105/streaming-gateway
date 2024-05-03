//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_TEST_SHARED_MEMORY_MANAGER_HPP
#define STREAMINGGATEWAYCROW_TEST_SHARED_MEMORY_MANAGER_HPP
#include <iostream>
#include <mutex>

#include "../src/shared_memory_manager.h"
#include "../src/logging.h"
#include "../src/ipc_message_queue.h"
#include "BaseTest.h"


class TestSharedMemoryManager: public BaseTest {
public:
    void runTests() override {
        test_register_and_unregister();
        test_get_memory_info();
        test_ipc_message_queue();
    }
protected:
    void setUp() override
    {
        lock.lock();
        manager = SharedMemoryManager_new();
    }

    void tearDown() override
    {
        SharedMemoryManager_delete(manager);
        lock.unlock();
        for(const std::string& v: on_teardown)
        {
            shm_unlink(v.c_str());
        }
    }
private:
    std::mutex lock;
    SharedMemoryManager* manager;
    std::vector<std::string> on_teardown = {"test_key32", "test_ipc_queue_key"};

    void test_register_and_unregister()
    {
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

    void test_get_memory_info()
    {
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

    void test_ipc_message_queue()
    {
        setUp();
        const char* shm_key = "test_ipc_queue_key";

        try{
            const char* region_name = "test_ipc_queue";
            int offset = 0;
            int byte_size = 5*1024*1024;

            std::cout << "RegisterSystemSharedMemory in test_ipc_message_queue" << std::endl;
            RegisterSystemSharedMemory(manager, region_name, shm_key, offset, byte_size);
            assert(!MutexState(manager));

            ipc_queue::InitializeQueue(*manager, region_name);

            for (int i = 0; i < 5; ++i) {
                auto* msg = new ipc_queue::Message;
                msg->dataLength = 1;
                msg->data = new u_char[msg->dataLength];
                *reinterpret_cast<int*>(msg->data) = i;
                msg->nextMessageStart = 0;

                ipc_queue::Enqueue(*manager, msg, region_name);
            }
            SharedMemoryManager another_manager(*manager);
            for (int i = 0; i < 5; ++i) {
                auto* message = ipc_queue::Deque(another_manager, region_name);
                if (message != nullptr) {
                    assert(*message->data == i);
                    delete[] message->data;
                    delete message;
                }
            }
            Unregister(manager, region_name);
        } catch (std::exception& e) {
            throw e;
        }
        tearDown();
    }
};





#endif //STREAMINGGATEWAYCROW_TEST_SHARED_MEMORY_MANAGER_HPP