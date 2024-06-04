//
// Created by oleg on 04.06.24.
//

#ifndef STREAMINGGATEWAYCROW_TCPSTREAMER_H
#define STREAMINGGATEWAYCROW_TCPSTREAMER_H

#include "../src/HandleStreamDaemon.h"

#include "../src/constants.h"
#include "opencv2/opencv.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

namespace tcp_stream {

    class TCPMessageIterator {
    private:
        int sockfd_;
    public:
        TCPMessageIterator(std::string& host, short port) {
            sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);

            if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
                perror("inet_pton");
                close(sockfd_);
                throw std::runtime_error("Invalid socket addr");
            }
        }

        [[nodiscard]] std::vector<unsigned char> new_message() const
        {
            size_t message_size;
            if (recv(sockfd_, &message_size, sizeof(size_t), 0) < 0) {
                perror("recv");
                close(sockfd_);
                throw std::runtime_error("Error during message_size retrieve");
            }
            std::vector<unsigned char> buffer(message_size);
            if (recv(sockfd_, buffer.data(), message_size, 0) < 0) {
                perror("recv");
                close(sockfd_);
                throw std::runtime_error("Error during buffer retrieve");
            }
            return buffer;
        }
    };

    class TCPMessageStream {
    public:
        explicit TCPMessageStream(std::string& host, short port) : begin_(host, port) {}

        [[nodiscard]] TCPMessageIterator begin() const { return begin_; }
    private:
        TCPMessageIterator begin_;
    };
}

namespace tcp_streamer {


    class TCPStreamer: public stream_daemon::HandleStreamDaemon {
    public:
        explicit TCPStreamer(
                SharedMemoryManager* memManager,
        const char* region_name
        ): stream_daemon::HandleStreamDaemon(memManager, region_name) {};
        void PutOnSHMQueue(void* iter_holder) override;
    };
}


#endif //STREAMINGGATEWAYCROW_TCPSTREAMER_H
