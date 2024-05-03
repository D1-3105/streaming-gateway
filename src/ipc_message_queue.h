//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#define STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#include <iostream>
#include "shared_memory_manager.h"

namespace ipc_queue
{
    struct QueueMetric {
        unsigned long message_cnt;
        unsigned long head_position;
        unsigned long rear_position;

        static ulong struct_size(){
            return sizeof(ulong ) * 3;
        }
    };

    struct Message {
        size_t dataLength;
        int* data;
        unsigned long nextMessageStart;
    };
    ipc_queue::QueueMetric* GetQueueMetric(SharedMemoryManager& manager, const char* region_name);
    void InitializeQueue(SharedMemoryManager & manager, const char* region_name);
    void Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name);
    ipc_queue::Message*  Deque(SharedMemoryManager& manager, const char* region_name);
}


#endif //STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
