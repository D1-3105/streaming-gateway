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
    public:
        virtual void PutOnSHMQueue(void* iter_holder) = 0;
        virtual void ListenSHMQueue
                (
                        std::function<void*(const ipc_queue::Message *, size_t)> callback,
                        long long prefetch_count
                ) = 0;
    };
}
#endif //STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H
