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
GetFromSHM(void* message_start, void* low_bound, bool ack=true)
{
    auto* msg = new shm_queue::Message;
    void* temp;

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(message_start);
    memcpy(&msg->dataLength, temp, sizeof(size_t));

    temp = shm_queue::Message::GetPtrOnNextOnSHM(message_start);

    ulong position;
    memcpy(&position,  temp, sizeof(typeof msg->nextMessageStart));
    msg->nextMessageStart = FromRelationalPointer(low_bound, position);

    msg->data = new u_char[msg->dataLength];
    temp = shm_queue::Message::GetPtrOnDataOnSHM(message_start);
    memcpy(msg->data, temp, msg->dataLength * sizeof(u_char));
    auto* is_processed = (std::atomic_bool*) shm_queue::Message::GetPtrOnProcessedOnSHM(message_start);
    if (ack)
        is_processed->exchange(true);
    msg->processed = is_processed;
    return msg;
}


void
PutOnSHM(shm_queue::Message* msg, void*& message_start, void* low_bound)
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
    msg->processed = new std::atomic_bool(false);
    memcpy(temp, msg->processed, sizeof(std::atomic_bool));

    temp = shm_queue::Message::GetPtrOnDataLengthOnSHM(true_message_start);

    memcpy(temp, &msg->dataLength, sizeof(msg->dataLength) );

    temp = shm_queue::Message::GetPtrOnNextOnSHM(true_message_start);
    auto* pos = reinterpret_cast<ulong*>(temp);
    *pos = (ulong) true_message_start + message_size;
    ulong relational_pos = ToRelationalPointer(low_bound, (void*)(*pos));
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

shm_queue::QueueMetric*
shm_queue::GetQueueMetric(SharedMemoryManager& manager, const char* region_name)
{
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(shm_queue::QueueMetric), &metric);
    auto metric_demarshalled = (shm_queue::QueueMetric*) metric;
    return metric_demarshalled;
}


void
shm_queue::InitializeQueue(SharedMemoryManager& manager, const char* region_name)
{
    // queue flag
    void* metric;
    manager.GetMemoryInfo(region_name, 0, sizeof(shm_queue::QueueMetric), &metric);
    new (metric) shm_queue::QueueMetric{
        std::atomic_flag(false),
        std::atomic_size_t(0),
        sizeof(shm_queue::QueueMetric),
        0
    };
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
        auto processed = static_cast<std::atomic_bool*>(shm_queue::Message::GetPtrOnProcessedOnSHM(address));
        if (!processed->load(std::memory_order_relaxed))
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
    QueueMetric* metric = GetQueueMetric(manager, region_name);
    metric->Acquire();
    BOOST_LOG_TRIVIAL(info) << "Published " << metric->message_cnt.load(std::memory_order_relaxed) << " message";

    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            sizeof(QueueMetric),
            manager.GetMemoryMap(region_name)->byte_size_ - sizeof(QueueMetric),
            &queueStartAddress
    );
    auto* startAddress = metric->rear_pos_local();
    if (startAddress == nullptr) {
        startAddress = queueStartAddress;
    }
    else
    {
        void** ptrToNextMessage = reinterpret_cast<void**>(Message::GetPtrOnNextOnSHM(startAddress));
        startAddress = FromRelationalPointer(queueStartAddress, (ulong)*ptrToNextMessage);
    }
    auto region_info = *manager.GetMemoryMap(region_name);
    if (!region_info.mapped_addr_)
    {
        throw std::runtime_error("Improperly configured region_info detected!");
    }
    if ((ulong)queueStartAddress + region_info.byte_size_ <= (ulong) startAddress + msg->MessageSizeOnSHM()) // queue overflow
    {
        metric->Unlock();
        BOOST_LOG_TRIVIAL(info) << "Message queue overflow detected; Joining listener";
        if (metric->message_cnt.load(std::memory_order_relaxed) == 0 and metric->head_pos_local() == queueStartAddress) {
            throw QueueException("Attempted to JoinListener, while message_cnt == 0!");
        }
        JoinListener(queueStartAddress, msg);
        BOOST_LOG_TRIVIAL(info) << "Listener fetched entire queue";
        metric->Acquire();
        BOOST_LOG_TRIVIAL(info) << "Reacquired metric";
        startAddress = queueStartAddress;
    }

    if (startAddress == nullptr)
    {
        throw std::runtime_error("Exception during startAddress allocation!");
    }

    PutOnSHM(
            msg,
            (void*&)startAddress,
            queueStartAddress
    );

    if(not metric->head_position) {
        metric->SetHead(startAddress);
    }
    metric->SetRear(startAddress);
    metric->message_cnt.fetch_add(1, std::memory_order_seq_cst);
    metric->Unlock();
    // metric is part of shm, so we do not delete it
}


shm_queue::Message*
shm_queue::Deque(SharedMemoryManager& manager, const char* region_name)
{
    QueueMetric* metric = GetQueueMetric(manager, region_name);
    metric->Acquire();

    if (not metric->message_cnt.load()) {
        metric->Unlock();
        throw QueueException("queue is empty - message_cnt = 0");
    }

    void* temp;

    auto* head = metric->head_pos_local();
    auto* region_info = manager.GetMemoryMap(region_name);
    void* queueStartAddress;
    manager.GetMemoryInfo(
            region_name,
            sizeof(QueueMetric),
            region_info->byte_size_ - sizeof(QueueMetric),
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

    Message* msg = GetFromSHM(head, queueStartAddress);
    if (metric->rear_pos_local() != metric->head_pos_local()) {
        metric->SetHead(FromRelationalPointer(
                queueStartAddress,
                *(ulong*)(Message::GetPtrOnNextOnSHM(head))  // omg
        ));
    } else {
        metric->head_position = 0;
    }

    if (!msg->nextMessageStart)
    {
        throw QueueException("Message queue exceeded!");
    }
    temp = Message::GetPtrOnProcessedOnSHM(head);
    if (temp)
    {
        ((std::atomic_bool*) temp)->exchange(true);
    }
    else
    {
        throw QueueException("Message publish terminated!");
    }
    metric->message_cnt.fetch_sub(1, std::memory_order_seq_cst);
    metric->Unlock();
    return msg;
}