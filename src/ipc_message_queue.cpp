//
// Created by oleg on 22.04.24.
//

#include "ipc_message_queue.h"

shm_queue::Message*
GetFromSHM(void* message_start)
{
    auto* msg = new shm_queue::Message;
    msg->processed = true;
    void* temp;

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(message_start);
    memcpy(&msg->dataLength, temp, sizeof(size_t));

    temp = shm_queue::Message::GetPtrOnNextOnSHM(message_start);
    memcpy(&msg->nextMessageStart,  temp, sizeof(typeof msg->nextMessageStart));

    msg->data = new u_char[msg->dataLength];
    temp = shm_queue::Message::GetPtrOnDataOnSHM(message_start);
    memcpy(msg->data, temp, msg->dataLength * sizeof(u_char));
    return msg;
}


void
PutOnSHM(shm_queue::Message* msg, void*& message_start, const SharedMemoryInfo& region_info_)
{
    uint message_size = sizeof(size_t) + msg->dataLength * sizeof(u_char) + sizeof(shm_queue::Message*) + sizeof(bool);
    if (!message_start)
    {
        throw std::runtime_error("Invalid message start on PutOnSHM");
    }

    void* true_message_start;
    auto current_offset = reinterpret_cast<uintptr_t>(message_start);
    if (current_offset + message_size <= region_info_.byte_size_ + region_info_.offset_ + reinterpret_cast<uintptr_t>(region_info_.mapped_addr_))
    {
        true_message_start = message_start;
    }
    else
    {
        true_message_start = reinterpret_cast<shm_queue::Message*>(reinterpret_cast<uintptr_t>(region_info_.mapped_addr_) + region_info_.offset_);
    }
    void* temp;

    temp = shm_queue::Message::GetPtrOnProcessedOnSHM(true_message_start);

    memcpy(temp, &msg->processed, sizeof(bool));

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(true_message_start);

    memcpy(temp, &msg->dataLength, sizeof(msg->dataLength) );

    temp = shm_queue::Message::GetPtrOnNextOnSHM(true_message_start);
    auto* pos = reinterpret_cast<ulong*>(temp);
    *pos = (ulong) true_message_start + message_size;

    temp = shm_queue::Message::GetPtrOnDataOnSHM(true_message_start);
    memcpy(
            temp,
            msg->data,
            msg->dataLength * sizeof(u_char)
    );

    message_start = true_message_start;
    // delete msg;
}

void
incrementQueueStat(void* metric, long long message_cnt)
{
    shm_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<shm_queue::QueueMetric*>(metric);
    metric_demarshaled->message_cnt += message_cnt;
}


void
setup_Head(void* metric, void* probable_head)
{
    shm_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<shm_queue::QueueMetric*>(metric);
    if (!metric_demarshaled->head_position)
        metric_demarshaled->head_position = probable_head;
}

shm_queue::QueueMetric*
shm_queue::GetQueueMetric(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(shm_queue::QueueMetric), &metric);
    shm_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<shm_queue::QueueMetric*>(metric);
    return metric_demarshaled;
}

void
shm_queue::InitializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    // queue flag
    auto* metric_demarshaled = GetQueueMetric(manager, region_name);
    metric_demarshaled->message_cnt = 0;
    // queue data
    void* queueStartAddress = nullptr;
    manager.GetMemoryInfo(region_name, shm_queue::QueueMetric::struct_size(), manager.GetMemoryMap(region_name)->byte_size_ - shm_queue::QueueMetric::struct_size(), &queueStartAddress);
    metric_demarshaled->head_position = queueStartAddress;
    metric_demarshaled->rear_position = nullptr;
}

void
shm_queue::Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name)
{
    shm_queue::QueueMetric* metric = GetQueueMetric(manager, region_name);
    BOOST_LOG_TRIVIAL(info) << "Published " << metric->message_cnt << " message";

    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            shm_queue::QueueMetric::struct_size(),
            manager.GetMemoryMap(region_name)->byte_size_ - shm_queue::QueueMetric::struct_size(),
            &queueStartAddress
    );
    msg->processed = false;
    auto* startAddress = metric->rear_position;
    if (startAddress == nullptr) {
        startAddress = queueStartAddress;
    } else {
        void** ptrToNextMessage = reinterpret_cast<void**>(Message::GetPtrOnNextOnSHM(startAddress));
        startAddress = *ptrToNextMessage;
    }

    if (startAddress == nullptr)
    {
        throw std::runtime_error("Exception during startAddress allocation!");
    }
    auto region_info = *manager.GetMemoryMap(region_name);
    if (!region_info.mapped_addr_)
    {
        throw std::runtime_error("Improperly configured region_info detected!");
    }

    PutOnSHM(msg, (void*&)startAddress, region_info);
    incrementQueueStat(metric, 1);
    setup_Head(metric, startAddress);
    metric->rear_position = startAddress;
}


shm_queue::Message*
shm_queue::Deque(SharedMemoryManager& manager, const char* region_name)
{
    shm_queue::QueueMetric* metric = GetQueueMetric(manager, region_name);
    void* temp;

    auto* head = metric->head_position;

    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            QueueMetric::struct_size(),
            manager.GetMemoryMap(region_name)->byte_size_ - QueueMetric::struct_size(),
            &queueStartAddress
    );
    temp = Message::GetPtrOnProcessedOnSHM(head);
    if (temp)
    {
        *((bool*) temp) = true;
    } else {
        throw QueueException("Message publish terminated!");
    }

    if(queueStartAddress == nullptr)
    {
        throw std::runtime_error("queueStartAddress received invalid value!");
    }

    if (head < queueStartAddress)
    {
        throw std::runtime_error("Queue is improperly allocated!");
    }
    Message* msg = GetFromSHM(head);

    incrementQueueStat(metric, -1);
    metric->head_position = *reinterpret_cast<void**>(Message::GetPtrOnNextOnSHM(head));
    if (!msg->nextMessageStart)
    {
        throw QueueException("Message queue exceeded!");
    }
    return msg;
}