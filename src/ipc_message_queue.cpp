//
// Created by oleg on 22.04.24.
//

#include "ipc_message_queue.h"

void incrementQueueStat(void* shm_mapped_addr, long long message_cnt)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(shm_mapped_addr);
    metric_demarshaled->message_cnt += message_cnt;
}

ipc_queue::QueueMetric* ipc_queue::GetQueueMetric(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::QueueMetric), &metric);
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    return metric_demarshaled;
}

void ipc_queue::InitializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    // queue flag
    auto* metric_demarshaled = GetQueueMetric(manager, region_name);
    metric_demarshaled->message_cnt = 0;
    // queue data
    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, sizeof(long long), sizeof(ipc_queue::Message*) * 2, &queueStartAddress);
    char* startAddress = static_cast<char*>(queueStartAddress);
    *reinterpret_cast<ipc_queue::Message**>(startAddress) = nullptr;
    *reinterpret_cast<ipc_queue::Message**>(startAddress + sizeof(ipc_queue::Message*)) = nullptr;
}

void ipc_queue::Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::QueueMetric), &metric);

    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            sizeof(ipc_queue::QueueMetric),
            sizeof(ipc_queue::Message*) * 2,
            &queueStartAddress
    );

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
    incrementQueueStat(metric, 1);
}

ipc_queue::Message* ipc_queue::Deque(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::QueueMetric), &metric);

    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, sizeof(ipc_queue::QueueMetric), sizeof(ipc_queue::Message*) * 2, &queueStartAddress);

    char* startAddress = static_cast<char*>(queueStartAddress);
    auto** headPtr = reinterpret_cast<ipc_queue::Message**>(startAddress);
    auto** tailPtr = reinterpret_cast<ipc_queue::Message**>(startAddress + sizeof(ipc_queue::Message*));

    if (*headPtr == nullptr) {
        return nullptr;
    }

    incrementQueueStat(metric, -1);

    auto* head = *headPtr;
    *headPtr = head->next;

    if (*headPtr == nullptr) {
        *tailPtr = nullptr;
    }

    return head;
}

