//
// Created by oleg on 05.06.24.
//

#include "constants.h"
#include <sys/sem.h>
#include <error.h>
#include "wc_daemon.h"
#include "WebCam.h"

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
    if (not msg->dataLength) {
        throw std::logic_error("Error - frame dataLength cannot be 0");
    }
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