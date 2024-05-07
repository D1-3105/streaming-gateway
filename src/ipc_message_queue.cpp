//
// Created by oleg on 22.04.24.
//

#include "ipc_message_queue.h"


void*
FromRelationalPointer(void* base_addr, ulong offset)
{
    return (void*) ((ulong) base_addr + offset);
}


ulong
ToRelationalPointer(void* base_addr, void* ptr)
{
    return ((ulong) ptr - (ulong) base_addr);
}


shm_queue::Message*
GetFromSHM(void* message_start)
{
    auto* msg = new shm_queue::Message;
    msg->processed = true;
    void* temp;

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(message_start);
    memcpy(&msg->dataLength, temp, sizeof(size_t));

    temp = shm_queue::Message::GetPtrOnNextOnSHM(message_start);

    ulong position;
    memcpy(&position,  temp, sizeof(typeof msg->nextMessageStart));
    msg->nextMessageStart = FromRelationalPointer(message_start, position);

    msg->data = new u_char[msg->dataLength];
    temp = shm_queue::Message::GetPtrOnDataOnSHM(message_start);
    memcpy(msg->data, temp, msg->dataLength * sizeof(u_char));
    return msg;
}


void
PutOnSHM(shm_queue::Message* msg, void*& message_start)
{
    size_t message_size = msg->MessageSizeOnSHM();
    if (!message_start)
    {
        throw std::runtime_error("Invalid message start on PutOnSHM");
    }

    void* true_message_start;

    true_message_start = message_start;
    void* temp;

    temp = shm_queue::Message::GetPtrOnProcessedOnSHM(true_message_start);

    memcpy(temp, &msg->processed, sizeof(bool));

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(true_message_start);

    memcpy(temp, &msg->dataLength, sizeof(msg->dataLength) );

    temp = shm_queue::Message::GetPtrOnNextOnSHM(true_message_start);
    auto* pos = reinterpret_cast<ulong*>(temp);
    *pos = (ulong) true_message_start + message_size;
    ulong relational_pos = ToRelationalPointer(true_message_start, (void*)(*pos));
    memcpy(
        temp,
        &relational_pos,
        sizeof(void*)
    );

    temp = shm_queue::Message::GetPtrOnDataOnSHM(true_message_start);
    memcpy(
            temp,
            msg->data,
            msg->dataLength * sizeof(u_char)
    );

    message_start = true_message_start;
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
    auto* metric_demarshaled = new shm_queue::QueueMetric;
    memcpy(&metric_demarshaled->message_cnt, metric, sizeof(size_t));

    ulong head_position_offset;

    memcpy(&head_position_offset, (void*)((ulong)metric + sizeof(size_t)), sizeof(void*));
    if (head_position_offset >= QueueMetric::struct_size())
        metric_demarshaled->head_position = FromRelationalPointer(metric, head_position_offset);
    else
        metric_demarshaled->head_position = FromRelationalPointer(metric, QueueMetric::struct_size());

    ulong rear_position_offset = 0;

    memcpy(&rear_position_offset, (void*)((ulong)metric + sizeof(size_t) + sizeof(void*)), sizeof(void*));
    if (rear_position_offset >= QueueMetric::struct_size())
        metric_demarshaled->rear_position = FromRelationalPointer(metric, rear_position_offset);
    else
        metric_demarshaled->rear_position = nullptr;

    return metric_demarshaled;
}


void
shm_queue::InitializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    // queue flag
    auto* region_info = manager.GetMemoryMap(region_name);
    auto* metric_demarshaled = GetQueueMetric(manager, region_name);
    metric_demarshaled->message_cnt = 0;
    // queue data
    void* queueStartAddress = nullptr;
    manager.GetMemoryInfo(
            region_name,
            shm_queue::QueueMetric::struct_size(),
            region_info->byte_size_ - shm_queue::QueueMetric::struct_size(),
            &queueStartAddress
    );
    metric_demarshaled->head_position = queueStartAddress;
    metric_demarshaled->rear_position = nullptr;
    metric_demarshaled->DumpQueueMetric(region_info->mapped_addr_);
    delete metric_demarshaled;
}

std::vector<void*>
TraversalQueue(void* start, size_t max_offset)
{
    void* current = start;
    std::vector<void*> addresses;
    ulong traversal_distance = 0;
    do
    {
        addresses.push_back(current);
        void* temp = reinterpret_cast<void**>(shm_queue::Message::GetPtrOnNextOnSHM(const_cast<void *>(current)));
        size_t message_size_no_data = shm_queue::Message::MessageSizeWithoutDataOnSHM();
        size_t message_data_length = *reinterpret_cast<size_t*>(shm_queue::Message::GetPtrOnDataLengthOnSHM(current));
        traversal_distance += message_size_no_data + message_data_length;
        current = FromRelationalPointer(current, *(ulong*)temp);
    } while (traversal_distance < max_offset);
    return addresses;
}

bool
AnyMessageInProgress(const std::vector<void*>& addresses)
{
    for(auto address: addresses)
    {
        bool processed = *static_cast<bool*>(shm_queue::Message::GetPtrOnProcessedOnSHM(address));
        if (!processed)
        {
            return true;
        }
    }
    return false;
}

void
JoinListener(void*& head, shm_queue::Message* msg)
{
    std::vector<void *> neighbour_addresses;
    neighbour_addresses = TraversalQueue(const_cast<void*>(head), msg->MessageSizeOnSHM());
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    while (
            AnyMessageInProgress(neighbour_addresses)
    );
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
    }
    else
    {
        void** ptrToNextMessage = reinterpret_cast<void**>(Message::GetPtrOnNextOnSHM(startAddress));
        startAddress = FromRelationalPointer(startAddress, (ulong)*ptrToNextMessage);
    }
    auto region_info = *manager.GetMemoryMap(region_name);
    if (!region_info.mapped_addr_)
    {
        throw std::runtime_error("Improperly configured region_info detected!");
    }
    if ((ulong)queueStartAddress + region_info.byte_size_ <= (ulong) startAddress + msg->MessageSizeOnSHM()) // queue overflow
    {
        BOOST_LOG_TRIVIAL(info) << "Message queue overflow detected; Joining listener";
        JoinListener(queueStartAddress, msg);
        BOOST_LOG_TRIVIAL(info) << "Listener fetched entire queue";
        startAddress = queueStartAddress;
    }

    if (startAddress == nullptr)
    {
        throw std::runtime_error("Exception during startAddress allocation!");
    }

    PutOnSHM(msg, (void*&)startAddress);
    metric->rear_position = startAddress;

    if (queueStartAddress != startAddress)
    {
        setup_Head(metric, startAddress);
        incrementQueueStat(metric, 1);
    }
    else
    {
        metric->head_position = startAddress;
        metric->message_cnt = 1;
    }
    metric->DumpQueueMetric(region_info.mapped_addr_);
    delete metric;
}


shm_queue::Message*
shm_queue::Deque(SharedMemoryManager& manager, const char* region_name)
{
    shm_queue::QueueMetric* metric = GetQueueMetric(manager, region_name);
    void* temp;

    auto* head = metric->head_position;
    auto* region_info = manager.GetMemoryMap(region_name);
    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            QueueMetric::struct_size(),
            region_info->byte_size_ - QueueMetric::struct_size(),
            &queueStartAddress
    );

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
    metric->head_position = FromRelationalPointer(
            head,
            *(ulong*)(Message::GetPtrOnNextOnSHM(head))  // omg
    );
    if (!msg->nextMessageStart)
    {
        throw QueueException("Message queue exceeded!");
    }
    temp = Message::GetPtrOnProcessedOnSHM(head);
    if (temp)
    {
        *((bool*) temp) = true;
    }
    else
    {
        throw QueueException("Message publish terminated!");
    }
    metric->DumpQueueMetric(region_info->mapped_addr_);
    delete metric;
    return msg;
}

void *shm_queue::QueueMetric::DumpQueueMetric(void *base_addr) {
    memcpy(base_addr, &message_cnt, sizeof(size_t));

    if (head_position)
    {
        ulong head_position_offset = ToRelationalPointer( base_addr, head_position);
        memcpy((void*)((ulong) base_addr + sizeof(size_t)), &head_position_offset, sizeof(void*));
    }
    else
    {
        ulong head_position_offset = 0;
        memcpy((void*)((ulong) base_addr + sizeof(size_t)), &head_position_offset, sizeof(void*));
    }


    if (rear_position){
        ulong rear_position_offset = ToRelationalPointer( base_addr, rear_position);
        memcpy((void*)((ulong) base_addr + sizeof(size_t) + sizeof(void*)), &rear_position_offset, sizeof(void*));
    }
    else
    {
        ulong rear_position_offset = 0;
        memcpy((void*)((ulong) base_addr + sizeof(size_t) + sizeof(void*)), &rear_position_offset, sizeof(void*));
    }

    return base_addr;
}
