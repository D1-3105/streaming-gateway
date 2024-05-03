//
// Created by oleg on 22.04.24.
//

#include "ipc_message_queue.h"


ulong
PutOnSHM(ipc_queue::Message& msg, ulong message_start, const SharedMemoryInfo& region_info_)
{
    uint message_size = sizeof(msg.dataLength) + sizeof(int) * msg.dataLength + sizeof(uint);
    if (!message_start)
    {
        throw std::runtime_error("Invalid message start on PutOnSHM");
    }
    ulong true_message_start = message_start;
    uint cummOffset = 0;
    if (message_size + message_start <= region_info_.byte_size_ + region_info_.offset_ + (ulong) region_info_.mapped_addr_)
    {
        true_message_start = message_start;
    }
    else
    {
        true_message_start = region_info_.byte_size_ + region_info_.offset_;
    }
    msg.nextMessageStart = message_size + true_message_start;
    memcpy(reinterpret_cast<void *>(true_message_start + cummOffset), &msg.dataLength, sizeof(msg.dataLength) );
    cummOffset += sizeof(msg.dataLength);
    memcpy(
            reinterpret_cast<void *>(true_message_start + cummOffset),
            &msg.nextMessageStart,
            sizeof(msg.nextMessageStart)
    );
    cummOffset += sizeof(msg.nextMessageStart);
    memcpy(
            reinterpret_cast<void *>(true_message_start + cummOffset),
            msg.data,
            msg.dataLength * sizeof(int)
    );
    return true_message_start;
}

ipc_queue::Message
GetFromSHM(ulong message_start) {
    ipc_queue::Message msg{};
    uint cummOffset = 0;

    memcpy(&msg.dataLength, reinterpret_cast<const void *>(message_start + cummOffset), sizeof(size_t));
    cummOffset += sizeof(msg.dataLength);

    memcpy(&msg.nextMessageStart, reinterpret_cast<const void *>(message_start + cummOffset), sizeof(msg.nextMessageStart));
    cummOffset += sizeof(msg.nextMessageStart);
    msg.data = new int[msg.dataLength];
    memcpy(msg.data, reinterpret_cast<const void *>(message_start + cummOffset), msg.dataLength * sizeof(int));

    return msg;
}

void
incrementQueueStat(void* metric, long long message_cnt)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    metric_demarshaled->message_cnt += message_cnt;
}

void
setup_Head(void* metric, ulong probable_head)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    if (!metric_demarshaled->head_position)
        metric_demarshaled->head_position = probable_head;
}

void
set_Rear(void* metric, ulong rear)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);

    metric_demarshaled->rear_position = rear;
}

ulong
get_Rear(void* metric)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);

    return metric_demarshaled->rear_position;
}

void
change_Head(void* metric, ulong new_head)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    metric_demarshaled->head_position = new_head;
}

ulong
get_Head(void* metric)
{
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    return metric_demarshaled->head_position;
}

ipc_queue::QueueMetric*
ipc_queue::GetQueueMetric(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(ipc_queue::QueueMetric), &metric);
    ipc_queue::QueueMetric* metric_demarshaled;
    metric_demarshaled = static_cast<ipc_queue::QueueMetric*>(metric);
    return metric_demarshaled;
}

void
ipc_queue::InitializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    // queue flag
    auto* metric_demarshaled = GetQueueMetric(manager, region_name);
    metric_demarshaled->message_cnt = 0;
    // queue data
    void* queueStartAddress = nullptr;
    manager.GetMemoryInfo(region_name, metric_demarshaled->struct_size(), 5 * 1024 * 1024 - metric_demarshaled->struct_size(), &queueStartAddress);
    metric_demarshaled->head_position = (ulong) queueStartAddress;
}

void
ipc_queue::Enqueue(SharedMemoryManager& manager, Message *msg, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, ipc_queue::QueueMetric::struct_size(), &metric);

    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            ipc_queue::QueueMetric::struct_size(),
            1024*1024,  // todo map only one message
            &queueStartAddress
    );


    ulong startAddress = get_Rear(metric);
    if (startAddress == 0) {
        startAddress = (ulong) queueStartAddress;
    }

    ulong new_start = PutOnSHM(*msg, (ulong) startAddress, *manager.GetMemoryMap(region_name));
    incrementQueueStat(metric, 1);
    setup_Head(metric, new_start);
    set_Rear(metric, msg->nextMessageStart);
}


ipc_queue::Message*
ipc_queue::Deque(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, ipc_queue::QueueMetric::struct_size(), &metric);

    ulong head = get_Head(metric);

    void* queueStartAddress;
    manager.GetMemoryInfo(region_name, ipc_queue::QueueMetric::struct_size(), 1024*1024, &queueStartAddress);
    if (head < (ulong) queueStartAddress)
    {
        throw std::runtime_error("Queue is improperly allocated!");
    }
    if(head == (ulong) queueStartAddress)
    {
        return nullptr;
    }

    auto* msg = new ipc_queue::Message();
    ipc_queue::Message copy_from = GetFromSHM(head);
    msg->nextMessageStart = copy_from.nextMessageStart;
    msg->dataLength = copy_from.dataLength;
    msg->data = copy_from.data;

    incrementQueueStat(metric, -1);

    change_Head(metric, msg->nextMessageStart);

    return msg;
}