//
// Created by oleg on 04.06.24.
//

#include "TCPStreamer.h"

void tcp_streamer::TCPStreamer::PutOnSHMQueue(void *iter_holder)
{
    InitStreaming();

    initialized_publisher_ = true;
    if (!iter_holder) {
        BOOST_LOG_TRIVIAL(error) << "Error: iter_holder is a null pointer!";
        return;
    }

    std::lock_guard<std::mutex> guard(mu_);
    auto stream = static_cast<tcp_stream::TCPMessageStream*>(iter_holder);
    auto tcp_msg = stream->begin().new_message();

    auto msg = std::make_unique<shm_queue::Message>();

    memcpy(msg->data, tcp_msg.data(), tcp_msg.size());

    msg->nextMessageStart = nullptr;

    shm_queue::Enqueue(*memoryManager_, msg.get(), region_name_);
}
