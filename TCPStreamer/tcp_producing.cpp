//
// Created by oleg on 05.06.24.
//

#include "tcp_producing.h"
#include "zlib.h"
#include "../src/constants.h"

size_t packet_num = 0;


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

    // Compress the tcp_message using gzip if enabled
    if (enable_gzip_) {
        auto decompressed_size = tcp_message.size();
        uLongf compressed_size = compressBound(decompressed_size);
        std::vector<uchar> compressed_message(compressed_size);

        int result = compress(compressed_message.data(), &compressed_size, tcp_message.data(), tcp_message.size());
        if (result != Z_OK) {
            BOOST_LOG_TRIVIAL(error) << ("Failed to compress TCP message");
            return;
        }

        // Resize the vector to the actual compressed size
        compressed_message.resize(compressed_size);

        // Serialize the decompressed size

        auto* decompressed_size_serialized = reinterpret_cast<unsigned char*>(&decompressed_size);
        compressed_message.insert(compressed_message.begin(), decompressed_size_serialized, decompressed_size_serialized + sizeof(decompressed_size));


        auto* packet_num_sr = (u_char*) &packet_num;
        compressed_message.insert(compressed_message.begin(), packet_num_sr, packet_num_sr+sizeof(size_t));

        BOOST_LOG_TRIVIAL(info) << packet_num << " sent at " << std::chrono::system_clock::now().time_since_epoch().count();

        // Publish the compressed message
        if (!publisher_->publish(compressed_message)) {
            BOOST_LOG_TRIVIAL(error) << ("Failed to publish compressed TCP message");
        }
    } else {
        BOOST_LOG_TRIVIAL(info) << packet_num << " sent at " << std::chrono::system_clock::now().time_since_epoch().count();
        auto* packet_num_sr = (u_char*) &packet_num;
        tcp_message.insert(tcp_message.begin(), packet_num_sr, packet_num_sr+sizeof(size_t));
        // Publish the uncompressed message
        if (!publisher_->publish(tcp_message)) {
            BOOST_LOG_TRIVIAL(error) << ("Failed to publish TCP message");
        }
    }
    packet_num++;
    // Release the matrix
    mat.release();
}

TCPFrameProducer::TCPFrameProducer(std::string& host, short port, short stream_device, bool enable_gzip) {
    publisher_ = new TCPPublisher();
    publisher_->init_socket(host, port);
    iterator_ = new webcam::WebCamStream(stream_device);
    enable_gzip_ = enable_gzip;
}

[[noreturn]] void TCPFrameProducer::Run() {
    while (true) {
        HandleWebcamFrame();
    }
}
