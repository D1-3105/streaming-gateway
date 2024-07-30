//
// Created by oleg on 18.04.24.
//

#include "HandleStreamDaemon.h"
#include <sys/ipc.h>
#include <sys/sem.h>

#include "utils.h"

int stream_daemon::HandleStreamDaemon::GetSema() {
    const std::string shm_key = memoryManager_->GetMemoryMap(region_name_)->shm_key_;
    int pr_id = crc::crc16((uint8_t*)shm_key.data(), shm_key.length());
    int semid_ = semget(pr_id, 1, IPC_CREAT | 0666);
    BOOST_LOG_TRIVIAL(info) << "Connected to semaphore: " << pr_id;
    return semid_;
}

void stream_daemon::HandleStreamDaemon::InitStreaming() {
    if (!initialized_publisher_)
    {
        shm_queue::InitializeQueue(*memoryManager_, region_name_);
        int sema_desc = GetSema();

        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        } semarg;
        pid_t p = getppid();
        semarg.val = crc::crc8((uint8_t*)&p, sizeof(pid_t) / sizeof(uint8_t));  // Set semaphore to parent process ID
        int sem_res = semctl(sema_desc, 0, SETVAL, semarg);
        if (sem_res == -1)
        {
            perror("semctl");
            throw std::runtime_error("Error during semaphore acquiring!");
        }
        BOOST_LOG_TRIVIAL(info) << "Semaphore #" << sema_desc << " unlocked with value: " << semarg.val;
    }
}

void stream_daemon::HandleStreamDaemon::InitHandler() {
    if (!initialized_publisher_)
    {
        BOOST_LOG_TRIVIAL(info) << "Waiting for publisher...";
        int semaphore_desc = GetSema();
        pid_t p = getppid();
        int sem_value = crc::crc8((uint8_t*)&p, sizeof(pid_t) / sizeof(uint8_t));
        BOOST_LOG_TRIVIAL(info) << "Expected handshake: " << sem_value;
        utils::waitForSemaphoreValue(semaphore_desc, sem_value);
        BOOST_LOG_TRIVIAL(info) << "Publisher online!";
    }
    initialized_publisher_ = true;
}


void* wait_for_messages(SharedMemoryManager& manager, const char* region_name)
{
    const auto metric = shm_queue::GetQueueMetric(manager, region_name);
    while(true)
    {
        if (metric->message_cnt.load(std::memory_order_relaxed)) {
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return nullptr;
}

void stream_daemon::HandleStreamDaemon::ListenSHMQueue
        (
                std::function<void*(const shm_queue::Message *, size_t)> callback,
                long long prefetch_count
        )
{
    std::lock_guard<std::mutex> guard(mu_);
    InitHandler();

    auto* message_buffer = new shm_queue::Message[prefetch_count];
    size_t prefetched = 0;

    for (size_t i = 0; i < prefetch_count; i++)
    {
        wait_for_messages(*memoryManager_, region_name_);
        try {
            message_buffer[i] = *shm_queue::Deque(*memoryManager_, region_name_);
            prefetched ++;
        } catch(shm_queue::QueueException& e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
        }
    }
    callback(message_buffer, prefetched);
}

