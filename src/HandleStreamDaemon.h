//
// Created by oleg on 18.04.24.
//

#ifndef STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H
#define STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H

#include "opencv2/opencv.hpp"
#include <iostream>
#include "ipc_message_queue.h"

namespace stream_daemon
{
    class HandleStreamDaemon {
    protected:
        const char* region_name_;
        SharedMemoryManager* memoryManager_;
        bool initialized_publisher_;
    public:
        explicit HandleStreamDaemon(SharedMemoryManager* memManager,
                           const char* region_name
        ): memoryManager_(memManager), region_name_(region_name), initialized_publisher_(false) {}
        virtual void PutOnSHMQueue(void* iter_holder) = 0;
        virtual void ListenSHMQueue
                (
                        std::function<void*(const shm_queue::Message *, size_t)> callback,
                        long long prefetch_count
                ) = 0;

        int GetSema();
    };
}
#endif //STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H
