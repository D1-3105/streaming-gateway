//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_WEBCAM_H
#define STREAMINGGATEWAYCROW_WEBCAM_H

#include <boost/log/trivial.hpp>
#include "HandleStreamDaemon.h"

#include "opencv2/opencv.hpp"

namespace webcam{

    class WebcamIterator {
    public:
        explicit WebcamIterator(int device = 0) {
            cap.open(device, cv::CAP_GSTREAMER);
        }

        ~WebcamIterator() {
            cap.release();
        }

        cv::Mat operator*() {
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) {
                BOOST_LOG_TRIVIAL(error) << "Empty frame captured!";
                throw std::runtime_error("Empty frame captured!");
            }
            return frame;
        }

        WebcamIterator& operator++() {
            return *this;
        }

        bool operator!=(const WebcamIterator& other) const {
            return cap.isOpened();
        }

    private:
        cv::VideoCapture cap;
    };

    class WebCamStream{
    public:
        explicit WebCamStream(int device = 0) : begin_(device), end_() {}

        [[nodiscard]] WebcamIterator begin() const { return begin_; }
        [[nodiscard]] WebcamIterator end() const { return end_; }
    private:
        WebcamIterator begin_;
        WebcamIterator end_;
    };
};

namespace wc_daemon {
    class WebCamStreamDaemon : public stream_daemon::HandleStreamDaemon {
    public:
        void PutOnSHMQueue(void* iter_holder) override;
    };
};

#endif //STREAMINGGATEWAYCROW_WEBCAM_H
