//
// Created by oleg on 05.06.24.
//

#include "tcp_producing.h"

#include "../src/constants.h"

void TCPFrameProducer::HandleWebcamFrame() {
    cv::Mat mat = iterator_->begin().matrix();
    if (mat.empty()) {
        BOOST_LOG_TRIVIAL(error) << ("Received empty frame from iterator");
        return;
    }

    size_t data_bytes = mat.total() * mat.elemSize();
    size_t mat_content_offset = 3 * video_constants::int2uchar + 2 * video_constants::size_t2uchar;
    size_t message_len = mat_content_offset + data_bytes;

    std::vector<uchar> tcp_message(message_len);
    uchar* initial = tcp_message.data();

    // Copy the header information
    size_t offset = 0;
    memcpy(initial + offset, &mat.cols, video_constants::int2uchar);
    offset += video_constants::int2uchar;

    memcpy(initial + offset, &mat.rows, video_constants::int2uchar);
    offset += video_constants::int2uchar;

    int type = mat.type();
    memcpy(initial + offset, &type, video_constants::int2uchar);
    offset += video_constants::int2uchar;

    size_t step = mat.step;
    memcpy(initial + offset, &step, video_constants::size_t2uchar);
    offset += video_constants::size_t2uchar;

    // Copy the matrix data
    memcpy(initial + offset, mat.data, data_bytes);

    // Publish the message
    if (!publisher_->publish(tcp_message)) {
        BOOST_LOG_TRIVIAL(error) << ("Failed to publish TCP message");
    }

    // Release the matrix
    mat.release();
}

TCPFrameProducer::TCPFrameProducer(std::string& host, short port, short stream_device) {
    publisher_ = new TCPPublisher();
    publisher_->init_socket(host, port);
    iterator_ = new webcam::WebCamStream(stream_device);
}

[[noreturn]] void TCPFrameProducer::Run() {
    while (true) {
        HandleWebcamFrame();
    }
}
