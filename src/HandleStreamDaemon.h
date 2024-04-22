//
// Created by oleg on 18.04.24.
//

#ifndef STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H
#define STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H

#include "opencv2/opencv.hpp"
#include <iostream>

namespace stream_daemon
{
    class HandleStreamDaemon {
    private:

    public:
        virtual void PutOnSHMQueue(void* iter_holder) = 0;
    };
}
#endif //STREAMING_GATEWAY_CROW_HANDLESTREAMDAEMON_H
