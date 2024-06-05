//
// Created by oleg on 22.04.24.
//

#ifndef STREAMINGGATEWAYCROW_WEBCAM_H
#define STREAMINGGATEWAYCROW_WEBCAM_H

#include <boost/log/trivial.hpp>
#include "mutex"

#include "opencv2/opencv.hpp"

namespace webcam{

    class WebcamIterator {
    public:
        explicit WebcamIterator(int device = 0)
        {
            cap.open(device, cv::CAP_GSTREAMER);
            double fps = cap.get(cv::CAP_PROP_FPS);
            BOOST_LOG_TRIVIAL(info) << "Frequency of camera: " << fps;
        }

        ~WebcamIterator()
        {
            cap.release();
        }

        cv::Mat matrix()
        {
            size_t retries = 0;
            cv::Mat frame;
            while (true) {
                cap >> frame;
                if (!frame.empty())
                {
                    break;
                }
                BOOST_LOG_TRIVIAL(info) << retries++;
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

        double fps()
        {
            return cap.get(cv::CAP_PROP_FPS);
        }

    private:
        cv::VideoCapture cap;
    };

    class WebCamStream{
    public:
        explicit WebCamStream(int device = 0) : begin_(device) {}

        [[nodiscard]] WebcamIterator begin() const { return begin_; }
    private:
        WebcamIterator begin_;
    };
};

#endif //STREAMINGGATEWAYCROW_WEBCAM_H
