//
// Created by oleg on 22.04.24.
//

#include "ipc_message_queue.h"

void ipc_queue::initializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::Message*) * 2, &queueStartAddress);
    char* startAddress = static_cast<char*>(queueStartAddress);
    *reinterpret_cast<ipc_queue::Message**>(startAddress) = nullptr;
    *reinterpret_cast<ipc_queue::Message**>(startAddress + sizeof(ipc_queue::Message*)) = nullptr;
}

void ipc_queue::enqueue(SharedMemoryManager& manager, ipc_queue::Message* msg, const char* region_name)
{
    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::Message*) * 2, &queueStartAddress);

    char* startAddress = static_cast<char*>(queueStartAddress);
    auto** headPtr = reinterpret_cast<ipc_queue::Message**>(startAddress);
    auto** tailPtr = reinterpret_cast<ipc_queue::Message**>(startAddress + sizeof(ipc_queue::Message*));

    if (*headPtr == nullptr) {
        *headPtr = msg;
        *tailPtr = msg;
    } else {
        (*tailPtr)->next = msg;
        *tailPtr = msg;
    }
}

ipc_queue::Message* ipc_queue::deque(SharedMemoryManager& manager, const char* region_name)
{
    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::Message*) * 2, &queueStartAddress);

    char* startAddress = static_cast<char*>(queueStartAddress);
    auto** headPtr = reinterpret_cast<ipc_queue::Message**>(startAddress);
    auto** tailPtr = reinterpret_cast<ipc_queue::Message**>(startAddress + sizeof(ipc_queue::Message*));

    if (*headPtr == nullptr) {
        return nullptr;
    }

    auto* head = *headPtr;
    *headPtr = head->next;

    if (*headPtr == nullptr) {
        *tailPtr = nullptr;
    }

    return head;
}



