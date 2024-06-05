//
// Created by oleg on 04.06.24.
//

#include "TCPStreamer.h"

void tcp_streamer::TCPStreamer::PutOnSHMQueue(void *iter_holder) {
    BOOST_LOG_TRIVIAL(info) << "InitStreaming() start";
    InitStreaming();
    BOOST_LOG_TRIVIAL(info) << "InitStreaming() done";
    initialized_publisher_ = true;

    if (!iter_holder) {
        BOOST_LOG_TRIVIAL(error) << "Error: iter_holder is a null pointer!";
        return;
    }

    std::lock_guard<std::mutex> guard(mu_);
    auto stream = static_cast<tcp_stream::TCPMessageStream*>(iter_holder);
    BOOST_LOG_TRIVIAL(info) << "stream->begin().new_message() call";
    auto tcp_msg = stream->begin().new_message();
    BOOST_LOG_TRIVIAL(info) << "stream->begin().new_message() done";

    if (tcp_msg.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Error: Received empty message!";
        return;
    }

    auto msg = std::make_unique<shm_queue::Message>();

    // Allocate memory for msg->data and ensure it is large enough
    msg->data = new unsigned char[tcp_msg.size()];
    if (!msg->data) {
        return;
    }

    memcpy(msg->data, tcp_msg.data(), tcp_msg.size());

    msg->nextMessageStart = nullptr;
    msg->dataLength = tcp_msg.size();

    shm_queue::Enqueue(*memoryManager_, msg.get(), region_name_);
    BOOST_LOG_TRIVIAL(info) << "shm_queue::Enqueue done";

    // Clean up the allocated memory
    delete[] msg->data;
    msg->data = nullptr;
}
