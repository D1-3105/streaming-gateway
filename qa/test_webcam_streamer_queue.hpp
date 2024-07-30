//
// Created by oleg on 23.04.24.
//

#ifndef STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP
#define STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP

#include "BaseTest.h"
#include "mutex"
#include "pthread.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "filesystem"

#include "../src/shared_memory_manager.h"
#include "../src/ipc_message_queue.h"

#include "../src/WebCam.h"
#include "../src/exporters.h"

#include "../src/ipc_message_queue.h"
#include "../src/wc_daemon.h"

class TestWebcamStreamerQueue: public BaseTest {
public:
    void runTests() override
    {
        test_enqueue_dequeue_queue();
        test_hls_exporter();
    }
protected:
    std::mutex lock;
    SharedMemoryManager* manager;

    void setUp() override
    {
        lock.lock();
        manager = SharedMemoryManager_new();
        video_repository = "/home/oleg/CLionProjects/streaming-gateway-crow/video_repository";
        region_name = "test_ipc_video";
    }

    void tearDown() override
    {
        SharedMemoryManager_delete(manager);
        shm_unlink("test_ipc_queue_key");
        std::filesystem::path video_repo(video_repository + "/*");
        std::filesystem::remove_all(video_repo);
        lock.unlock();
    }

private:
    std::string video_repository;
    std::string region_name;

    void test_enqueue_dequeue_queue()
    {
        setUp();
        const char* shm_key = "test_ipc_queue_key";

        int offset = 0;
        int byte_size = 10 * 50 * 1024 * 1024; // 500mb - test purposes
        std::cout << "RegisterSystemSharedMemory in test_enqueue_dequeue_queue" << std::endl;
        RegisterSystemSharedMemory(manager, region_name.c_str(), shm_key, offset, byte_size);

        wc_daemon::WebCamStreamDaemon wc_d(manager, region_name.c_str());
        webcam::WebcamIterator stream_iterator;

        long long batch_size = 5;
        bool forced_exit = false;
        auto listen_assertion = [&forced_exit] (const shm_queue::Message* messages, size_t size) {
            for(size_t i = 0; i < size and !forced_exit; i++)
            {
                if (messages[i].data == nullptr) {
                    BOOST_LOG_TRIVIAL(error) << "messages[i].data != nullptr - false, [" << i << "]";
                }
                assert(messages[i].data != nullptr);
            }
            return nullptr;
        };

        std::thread fetcher([listen_assertion, batch_size, this, &forced_exit](){
            wc_daemon::WebCamStreamDaemon daemon(
                    manager,
                    region_name.c_str()
            );
            try{
                for (int i = 0; i < 100 / batch_size and !forced_exit; i++)
                    daemon.ListenSHMQueue(listen_assertion, batch_size);
            }
            catch (std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Error during async fetch: " << e.what();
            }
        });
        try {
            for (int i = 0; i < 100; i++) {
                wc_d.PutOnSHMQueue(&stream_iterator);
            }
        }
        catch (shm_queue::QueueException& eq) {
            forced_exit = true;
            BOOST_LOG_TRIVIAL(error) << eq.what();
        }
        catch (std::runtime_error& er) {
            forced_exit = true;
            BOOST_LOG_TRIVIAL(error) << er.what();
        }
        catch (std::exception& e) {
            forced_exit = true;
            throw e;
        }
        if (!forced_exit)
            fetcher.join();
        tearDown();
    }

    static void* publisher(const std::string& region_name, SharedMemoryManager* manager, std::string& video_repository)
    {
        wc_daemon::WebCamStreamDaemon wc_d(manager, region_name.c_str());
        webcam::WebcamIterator stream_iterator;

        for (int i = 0; i < stream_iterator.fps() * 5; i++)  // 5 seconds X fps
        {
            wc_d.PutOnSHMQueue(&stream_iterator);
        }
        return nullptr;
    }

    static void* listener(const std::string& region_name, SharedMemoryManager* manager, std::string& video_repository)
    {
        wc_daemon::WebCamStreamDaemon wc_d(manager, region_name.c_str());
        exporters::FileBaseHLSMessageHandler fb_hls_handler(
                &wc_d,
                10,
                video_repository.c_str()
        );
        fb_hls_handler.HandleMessages();
    }

    static void* listenerStatic(void* arg) {
        auto* instance = static_cast<TestWebcamStreamerQueue*>(arg);
        TestWebcamStreamerQueue::listener(instance->region_name, instance->manager, instance->video_repository);
        return nullptr;
    }

    static void* publisherStatic(void* arg) {
        auto* instance = static_cast<TestWebcamStreamerQueue*>(arg);
        TestWebcamStreamerQueue::publisher(instance->region_name, instance->manager, instance->video_repository);
        return nullptr;
    }

    void test_hls_exporter()
    {
        setUp();
        pthread_t listener_t, publisher_t;
        const char* shm_key = "test_ipc_queue_key";

        int offset = 0;
        int byte_size = 7 * 1024 * 1024; // 10mb - test purposes

        RegisterSystemSharedMemory(manager, region_name.c_str(), shm_key, offset, byte_size);

        pthread_create(&listener_t, nullptr, &TestWebcamStreamerQueue::listenerStatic, this);
        pthread_create(&publisher_t, nullptr, &TestWebcamStreamerQueue::publisherStatic, this);
        pthread_join(publisher_t, nullptr);
        sleep(2);
        pthread_cancel(listener_t);
        pthread_join(listener_t, nullptr);
        tearDown();
    }
};

#endif //STREAMINGGATEWAYCROW_TEST_WEBCAM_STREAMER_QUEUE_HPP
