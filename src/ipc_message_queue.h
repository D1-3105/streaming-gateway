//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#define STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
#include <iostream>
#include "shared_memory_manager.h"

namespace ipc_queue
{
    struct Message {
        size_t dataLength;
        char* data;
        Message* next;
    };

    void initializeQueue(SharedMemoryManager& manager, const char* region_name);

    void enqueue(SharedMemoryManager& manager, Message* msg, const char* region_name);
    ipc_queue::Message*  deque(SharedMemoryManager& manager, const char* region_name);
}


#endif //STREAMINGGATEWAYCROW_IPC_MESSAGE_QUEUE_H
