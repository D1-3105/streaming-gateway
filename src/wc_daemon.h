//
// Created by oleg on 05.06.24.
//

#ifndef STREAMINGGATEWAYCROW_WC_DAEMON_H
#define STREAMINGGATEWAYCROW_WC_DAEMON_H

#include "shared_memory_manager.h"
#include "opencv2/opencv.hpp"
#include "mutex"
#include <pthread.h>
#include "HandleStreamDaemon.h"
#include <boost/log/trivial.hpp>

namespace wc_daemon {
    class WebCamStreamDaemon : public stream_daemon::HandleStreamDaemon {
    public:
        explicit WebCamStreamDaemon(
                SharedMemoryManager* memManager,
                const char* region_name
        ): HandleStreamDaemon(memManager, region_name) {};
        void PutOnSHMQueue(void* iter_holder) override;
    };
}


#endif //STREAMINGGATEWAYCROW_WC_DAEMON_H
