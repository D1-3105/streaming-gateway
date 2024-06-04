//
// Created by oleg on 22.04.24.
//

#include <sys/sem.h>
#include "WebCam.h"
#include "constants.h"


void wc_daemon::WebCamStreamDaemon::PutOnSHMQueue(void *iter_holder)
{
    InitStreaming();

    initialized_publisher_ = true;
    if (!iter_holder) {
        BOOST_LOG_TRIVIAL(error) << "Error: iter_holder is a null pointer!";
        return;
    }

    std::lock_guard<std::mutex> guard(mu_);
    auto stream = static_cast<webcam::WebCamStream*>(iter_holder);
    cv::Mat mat = stream->begin().matrix();

    size_t data_bytes = mat.total() * mat.elemSize();

    auto msg = std::make_unique<shm_queue::Message>();
    size_t mat_content_offset = 3 * video_constants::int2uchar + video_constants::size_t2uchar;

    msg->dataLength = data_bytes + mat_content_offset;
    msg->data = new u_char[msg->dataLength];
    int* cols = &mat.cols;
    int* rows = &mat.rows;
    size_t step = mat.step;
    int type = mat.type();

    memcpy(msg->data, cols,  video_constants::int2uchar);
    memcpy(msg->data + video_constants::int2uchar, rows, video_constants::int2uchar);
    memcpy(msg->data + 2 * video_constants::int2uchar, &type, video_constants::int2uchar);
    memcpy(msg->data + 3 * video_constants::int2uchar, &step, video_constants::size_t2uchar);
    memcpy(msg->data + mat_content_offset, mat.data, data_bytes);

    msg->nextMessageStart = nullptr;

    shm_queue::Enqueue(*memoryManager_, msg.get(), region_name_);
}
