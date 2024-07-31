//
// Created by oleg on 31.07.24.
//
#include "gtest/gtest.h"
#include "mutex"

#include "shared_memory_manager.h"
#include "ipc_message_queue.h"
#include <iostream>
#include "condition_variable"

class TestSharedMemoryManager: public ::testing::Test {
public:
    std::mutex lock;
    SharedMemoryManager* manager_ = nullptr;
    std::vector<std::string> on_teardown_ = {"test_key32", "test_ipc_queue_key"};
protected:
    void SetUp() override
    {
        lock.lock();
        manager_ = SharedMemoryManager_new();
    }

    void TearDown() override
    {
        SharedMemoryManager_delete(manager_);
        lock.unlock();
        for(const std::string& v: on_teardown_)
        {
            shm_unlink(v.c_str());
        }
    }

    static void simple_enqueue(const char* region_name, SharedMemoryManager& manager, int num) {
        auto* msg = new shm_queue::Message;
        msg->dataLength = sizeof(int) / sizeof(u_char);
        msg->data = new u_char[msg->dataLength];
        *reinterpret_cast<int*>(msg->data) = num;
        shm_queue::Enqueue(manager, msg, region_name);
    }

    static int simple_dequeue(const char* region_name, SharedMemoryManager& manager) {
        auto* message = shm_queue::Deque(manager, region_name);
        int num;
        num = *message->data;
        delete[] message->data;
        delete message;
        return num;
    }
};

TEST_F(TestSharedMemoryManager, test_register_and_unregister)
{
    const char* name = "test_shm11";
    const char* shm_key = "test_key32";
    int offset = 0;
    int byte_size = 1024;

    std::cout << "RegisterSystemSharedMemory in test_register_and_unregister" << std::endl;

    RegisterSystemSharedMemory(manager_, name, shm_key, offset, byte_size);
    ASSERT_TRUE(!MutexState(manager_));

    std::cout << "Unregister in test_register_and_unregister" << std::endl;
    Unregister(manager_, name);
    ASSERT_TRUE(!MutexState(manager_));
}


TEST_F(TestSharedMemoryManager, test_get_memory_info)
{
    const char* name = "test_shm111";
    const char* shm_key = "test_key23";
    int offset = 0;
    int byte_size = 1024;

    std::cout << "RegisterSystemSharedMemory in test_get_memory_info" << std::endl;
    RegisterSystemSharedMemory(manager_, name, shm_key, offset, byte_size);
    ASSERT_TRUE(!MutexState(manager_));

    void* shm_mapped_addr = nullptr;

    std::cout << "GetMemoryInfo in test_get_memory_info" << std::endl;
    GetMemoryInfo(manager_, name, offset, byte_size, &shm_mapped_addr);
    ASSERT_TRUE(!MutexState(manager_));
    ASSERT_TRUE(shm_mapped_addr != nullptr);
}


TEST_F(TestSharedMemoryManager, test_ipc_message_queue)
{
    const char* shm_key = "test_ipc_queue_key";
    const char* region_name = "test_ipc_queue";
    int offset = 0;
    int byte_size = 5*1024*1024;

    std::cout << "RegisterSystemSharedMemory in test_ipc_message_queue" << std::endl;
    RegisterSystemSharedMemory(manager_, region_name, shm_key, offset, byte_size);
    assert(!MutexState(manager_));

    shm_queue::InitializeQueue(*manager_, region_name);

    for (int i = 0; i < 5; ++i) {
        auto* msg = new shm_queue::Message;
        msg->dataLength = sizeof(int) / sizeof(u_char);
        msg->data = new u_char[msg->dataLength];
        *reinterpret_cast<int*>(msg->data) = i;
        shm_queue::Enqueue(*manager_, msg, region_name);
    }
    SharedMemoryManager another_manager(*manager_);
    for (int i = 0; i < 5; ++i) {
        auto* message = shm_queue::Deque(another_manager, region_name);
        if (message != nullptr) {
            assert(*message->data == i);
            delete[] message->data;
            delete message;
        }
    }
    Unregister(manager_, region_name);
}

TEST_F(TestSharedMemoryManager, test_ipc_message_queue_traversal_case)
{
    const char* shm_key = "test_ipc_queue_key";
    const char* region_name = "test_ipc_queue";
    int offset = 0;
    int byte_size = 100;

    std::cout << "RegisterSystemSharedMemory in test_ipc_message_queue" << std::endl;
    RegisterSystemSharedMemory(manager_, region_name, shm_key, offset, byte_size);
    assert(!MutexState(manager_));

    shm_queue::InitializeQueue(*manager_, region_name);
    int i = 0;
    for (; i < 4; ++i) {
        simple_enqueue(region_name, *manager_, i);
    }
    std::thread enqueue_thread(
            simple_enqueue, region_name, std::ref(*manager_), i);
    sleep(2);
    SharedMemoryManager another_manager(*manager_);
    int val_overflow = simple_dequeue(region_name, another_manager);
    assert(val_overflow == 0);
    std::cout << "enqueue_thread.join()" << std::endl;
    enqueue_thread.join(); // should be unlocked

    for (int j = 1; j < i; ++j) {
        int val = simple_dequeue(region_name, another_manager);
        assert(val == j);
    }
    Unregister(manager_, region_name);
}
