//
// Created by oleg on 04.06.24.
//

#include "TCPStreamer.h"
#include "zlib.h"

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
    size_t packet_num = *(size_t*) tcp_msg.data();
    tcp_msg.erase(tcp_msg.begin(), tcp_msg.begin() + sizeof(size_t));
    BOOST_LOG_TRIVIAL(info) << packet_num << " received at " << std::chrono::system_clock::now().time_since_epoch().count();
    BOOST_LOG_TRIVIAL(info) << "stream->begin().new_message() done";

    if (tcp_msg.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Error: Received empty message!";
        return;
    }

    std::vector<unsigned char> received_decomp;
    if (enable_gzip_) {
        // Extract the decompressed size from the beginning of the message
        size_t decompressed_size = 0;
        memcpy(&decompressed_size, tcp_msg.data(), sizeof(size_t));

        // Decompress the tcp_msg using gzip
        std::vector<uchar> decompressed_message(decompressed_size);

        int result = uncompress(decompressed_message.data(), &decompressed_size, tcp_msg.data() + sizeof(size_t), tcp_msg.size() - sizeof(size_t));

        if (result != Z_OK) {
            BOOST_LOG_TRIVIAL(error) << "Failed to decompress TCP message";
            return;
        }
        received_decomp.assign(decompressed_message.begin(), decompressed_message.end());
    } else {
        received_decomp.assign(tcp_msg.begin(), tcp_msg.end());
    }

    auto msg = std::make_unique<shm_queue::Message>();

    // Allocate memory for msg->data and ensure it is large enough
    msg->data = new unsigned char[received_decomp.size()];
    if (!msg->data) {
        return;
    }

    memcpy(msg->data, received_decomp.data(), received_decomp.size());

    msg->nextMessageStart = nullptr;
    msg->dataLength = received_decomp.size();

    shm_queue::Enqueue(*memoryManager_, msg.get(), region_name_);
    BOOST_LOG_TRIVIAL(info) << packet_num << " Enqueue at " << std::chrono::system_clock::now().time_since_epoch().count();
    BOOST_LOG_TRIVIAL(info) << "shm_queue::Enqueue done";

    // Clean up the allocated memory
    delete[] msg->data;
    msg->data = nullptr;
}