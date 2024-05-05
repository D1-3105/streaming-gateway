//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#define STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#include <iostream>
#include "shared_memory_manager.h"

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
        bool processed;
        u_char* data;

        static void* GetPtrOnNextOnSHM(void* base_addr)
        {
            return (void*) ((ulong) base_addr + sizeof(bool) + sizeof(typeof Message::dataLength));
        }

        static void* GetPtrOnDataLengthOnSHM(void* base_addr)
        {
            return (void*) ((ulong) base_addr + sizeof(bool));
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
            size_t message_size = sizeof(size_t) + dataLength * sizeof(u_char) + sizeof(void*) + sizeof(bool);
            return message_size;
        }

        static size_t MessageSizeWithoutDataOnSHM()
        {
            size_t message_size = sizeof(size_t) + sizeof(void*) + sizeof(bool);
            return message_size;
        }
    };

    struct QueueMetric {
        size_t message_cnt;
        void* head_position;
        void* rear_position;

        static ulong struct_size(){
            return sizeof(size_t) + sizeof(void*) + sizeof(void*);
        }
    };


    shm_queue::QueueMetric* GetQueueMetric(SharedMemoryManager& manager, const char* region_name);
    void InitializeQueue(SharedMemoryManager & manager, const char* region_name);
    void Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name);
    shm_queue::Message*  Deque(SharedMemoryManager& manager, const char* region_name);
}


#endif //STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
