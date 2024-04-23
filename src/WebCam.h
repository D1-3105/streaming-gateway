//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_WEBCAM_H
#define STREAMINGGATEWAYCROW_WEBCAM_H

#include <boost/log/trivial.hpp>
#include "HandleStreamDaemon.h"
#include <pthread.h>
#include "mutex"

#include "opencv2/opencv.hpp"
#include "shared_memory_manager.h"

namespace webcam{

    class WebcamIterator {
    public:
        explicit WebcamIterator(int device = 0)
        {
            cap.open(device, cv::CAP_GSTREAMER);
        }

        ~WebcamIterator()
        {
            cap.release();
        }

        cv::Mat operator*()
        {
            cv::Mat frame;
            while (true) {
                cap >> frame;
                if (!frame.empty())
                {
                    break;
                }
                cv::waitKey(1);
            }
            return frame;
        }

        WebcamIterator& operator++()
        {
            return *this;
        }

        bool operator!=(const WebcamIterator& other) const
        {
            return cap.isOpened();
        }

    private:
        cv::VideoCapture cap;
    };

    class WebCamStream{
    public:
        explicit WebCamStream(int device = 0) : begin_(device), end_() {}

        WebcamIterator begin() const { return begin_; }
        WebcamIterator end() const { return end_; }
    private:
        WebcamIterator begin_;
        WebcamIterator end_;
    };
};

namespace wc_daemon {
    class WebCamStreamDaemon : public stream_daemon::HandleStreamDaemon {
    private:
        SharedMemoryManager* memoryManager;
        std::mutex mu_;
    protected:
        const char* region_name_;
    public:
        explicit WebCamStreamDaemon(
                SharedMemoryManager* memManager,
                const char* region_name
        ): memoryManager(memManager), region_name_(region_name) {};
        void PutOnSHMQueue(void* iter_holder) override;
        void ListenSHMQueue
                (
                        std::function<void*(const ipc_queue::Message *, size_t)> callback,
                        long long prefetch_count
                ) override;
    };
};

#endif //STREAMINGGATEWAYCROW_WEBCAM_H
