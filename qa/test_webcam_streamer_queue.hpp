//
// Created by oleg on 23.04.24.
//

#ifndef STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP
#define STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP

#include "BaseTest.h"
#include "mutex"
#include "../src/shared_memory_manager.h"
#include "../src/ipc_message_queue.h"

#include "../src/WebCam.h"

class TestWebcamStreamerQueue: public BaseTest {
public:
    void runTests() override
    {
        test_enqueue_dequeue_queue();
    }
protected:
    std::mutex lock;
    SharedMemoryManager* manager;

    void setUp() override
    {
        lock.lock();
        manager = SharedMemoryManager_new();
    }

    void tearDown() override
    {
        SharedMemoryManager_delete(manager);
        shm_unlink("test_ipc_queue_key");
        lock.unlock();
    }
private:
    void test_enqueue_dequeue_queue()
    {
        setUp();
        const char* shm_key = "test_ipc_queue_key";

        try {
            const char* region_name = "test_ipc_queue";
            int offset = 0;
            int byte_size = 10 * 50*1024*1024; // 500mb - test purposes

            std::cout << "RegisterSystemSharedMemory in test_enqueue_dequeue_queue" << std::endl;
            RegisterSystemSharedMemory(manager, region_name, shm_key, offset, byte_size);

            wc_daemon::WebCamStreamDaemon wc_d(manager, region_name);
            webcam::WebcamIterator stream_iterator;

            for (int i = 0; i < 100; i++)
                wc_d.PutOnSHMQueue(&stream_iterator);

            long long batch_size = 5;

            auto listen_assertion = [batch_size] (const ipc_queue::Message* messages, size_t size) {
                for(size_t i = 0; i < size; i++)
                {
                    assert(messages[i].data != nullptr);
                }
                return nullptr;
            };
            wc_d.ListenSHMQueue(listen_assertion, batch_size);
        } catch (std::exception& e) {
            throw e;
        }


        tearDown();
    }
};

#endif //STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP
