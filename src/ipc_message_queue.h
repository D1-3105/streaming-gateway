//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#define STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#include <iostream>
#include "shared_memory_manager.h"
#include "logging.h"
#include <boost/stacktrace.hpp>
#include "atomic"

namespace shm_queue
{
    class QueueException: public std::exception
    {
    public:
        explicit QueueException(std::string  message) : msg_(std::move(message)) {}
        [[nodiscard]] const char* what() const noexcept override {
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };

    struct Message {
        size_t dataLength;
        void* nextMessageStart;
        std::atomic_bool* processed;
        u_char* data;

        static void* GetPtrOnNextOnSHM(void* base_addr)
        {
            return (void*) ((ulong) base_addr + sizeof(std::atomic_bool) + sizeof(typeof Message::dataLength));
        }

        static void* GetPtrOnDataLengthOnSHM(void* base_addr)
        {
            return (void*) ((ulong) base_addr + sizeof(std::atomic_bool));
        }

        static void* GetPtrOnProcessedOnSHM(void* base_addr)
        {
            return base_addr;
        }

        static void* GetPtrOnDataOnSHM(void* base_addr)
        {
            void* tmp = Message::GetPtrOnNextOnSHM(base_addr);
            return (void*) ((ulong) tmp + sizeof(ulong));
        }

        [[nodiscard]] size_t MessageSizeOnSHM() const
        {
            size_t message_size = sizeof(size_t) + dataLength * sizeof(u_char) + sizeof(void*) + sizeof(std::atomic_bool);
            return message_size;
        }

        static size_t MessageSizeWithoutDataOnSHM()
        {
            size_t message_size = sizeof(size_t) + sizeof(void*) + sizeof(std::atomic_bool);
            return message_size;
        }
    };

    struct QueueMetric {
        std::atomic_flag locked;
        std::atomic_size_t message_cnt;
        std::atomic_ulong head_position;
        std::atomic_ulong rear_position;

        void Acquire() {
            while(std::atomic_flag_test_and_set_explicit(&locked, std::memory_order_acquire));
        };

        void Unlock() {
            std::atomic_flag_clear_explicit(&locked, std::memory_order_release);
        }

        void* head_pos_local() {
            return (void*) (ulong(this) + this->head_position.load());
        }

        void* rear_pos_local() {
            if (not this->rear_position)
                return nullptr;
            return (void*) (ulong(this) + this->rear_position.load());
        }

        void SetRear(void* base_addr) {
            this->rear_position.exchange(ulong(base_addr) - ulong(this));
        }

        void SetHead(void* base_addr) {
            this->head_position.exchange(ulong(base_addr) - ulong(this));
        }
    };
    shm_queue::QueueMetric* GetQueueMetric(SharedMemoryManager& manager, const char* region_name);
    void InitializeQueue(SharedMemoryManager & manager, const char* region_name);
    void Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name);
    shm_queue::Message*  Deque(SharedMemoryManager& manager, const char* region_name);
}


#endif //STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
