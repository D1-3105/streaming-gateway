//
// Created by oleg on 05.06.24.
//

#ifndef STREAMINGGATEWAYCROW_TCPPUBLISHER_H
#define STREAMINGGATEWAYCROW_TCPPUBLISHER_H


#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "../src/logging.h"


class TCPPublisher {
private:
    int sockfd_;

public:
    TCPPublisher() : sockfd_(-1) {}

    ~TCPPublisher() {
        close_socket();
    }

    bool init_socket(const std::string& host, int port) {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            BOOST_LOG_TRIVIAL(error) << ("Failed to create socket");
            return false;
        }

        sockaddr_in server_addr{};
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            BOOST_LOG_TRIVIAL(error) << ("Invalid address/ Address not supported");
            return false;
        }

        if (connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            BOOST_LOG_TRIVIAL(error) << ("Connection failed");
            return false;
        }

        return true;
    }

    bool publish(const std::vector<unsigned char>& message) const {
        if (sockfd_ == -1) {
            std::cerr << "Socket is not initialized" << std::endl;
            return false;
        }

        size_t total_bytes_sent = 0;
        size_t message_size = message.size();
        ssize_t bytes_size = send(sockfd_, &message_size, sizeof(size_t), 0);
        if (bytes_size < 0) {
            std::cerr << "Failed to send size: ";
            return false;
        }
        const char* data = reinterpret_cast<const char*>(message.data());

        while (total_bytes_sent < message_size) {
            ssize_t bytes_sent = send(sockfd_, data + total_bytes_sent, message_size - total_bytes_sent, 0);
            if (bytes_sent < 0) {
                std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
                return false;
            }
            total_bytes_sent += bytes_sent;
            std::cerr << "Sent bytes: " << bytes_sent << ", Total sent: " << total_bytes_sent << " of " << message_size << std::endl;
            // Add a small delay to prevent overwhelming the receiver
            usleep(3000); // Sleep for 3ms
        }

        return true;
    }

    void close_socket() {
        if (sockfd_ != -1) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }
};

#endif //STREAMINGGATEWAYCROW_TCPPUBLISHER_H
