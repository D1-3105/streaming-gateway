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
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <functional>

namespace tcp_stream {
    class TCPMessageIterator {
    private:
        int listen_sockfd_;
        int conn_sockfd_;
        std::vector<std::vector<unsigned char>> messages_;
        mutable std::mutex mutex_;
        mutable std::condition_variable cond_var_;
        std::thread listen_thread_;
        bool stop_thread_;

        std::vector<std::thread> thread_pool_;
        std::queue<std::function<void()>> task_queue_;
        std::mutex task_mutex_;
        std::condition_variable task_cond_var_;
        bool stop_thread_pool_;

        void listen_for_messages() {
            while (!stop_thread_) {
                size_t message_size;
                if (recv(conn_sockfd_, &message_size, sizeof(size_t), 0) <= 0) {
                    if (stop_thread_) break;
                    perror("recv");
                    continue;
                }

                if (message_size > 100000000) {  // Arbitrary large value to catch suspicious sizes
                    BOOST_LOG_TRIVIAL(error) << "Received suspiciously large message size: " << message_size;
                    throw std::bad_alloc();
                }

                std::vector<unsigned char> buffer;
                try {
                    buffer.resize(message_size);
                } catch (const std::bad_alloc& e) {
                    BOOST_LOG_TRIVIAL(error) << "Failed to allocate memory for message buffer: " << e.what() << "; size was: " << message_size;
                    continue;
                }
                BOOST_LOG_TRIVIAL(info) << "Receiving new message: " << message_size;
                size_t total_bytes_received = 0;
                while (total_bytes_received < message_size) {
                    ssize_t bytes_received = recv(conn_sockfd_, buffer.data() + total_bytes_received, message_size - total_bytes_received, 0);
                    if (bytes_received <= 0) {
                        perror("recv");
                        break;
                    }
                    total_bytes_received += bytes_received;
                }
                BOOST_LOG_TRIVIAL(info) << "NEW MESSAGE RECEIVED";

                {
                    std::lock_guard<std::mutex> lock(task_mutex_);
                    task_queue_.emplace([this, buffer = std::move(buffer)]() mutable {
                        std::lock_guard<std::mutex> lock(mutex_);
                        messages_.push_back(std::move(buffer));
                        cond_var_.notify_one();
                    });
                }
                task_cond_var_.notify_one();
            }
        }

        void thread_pool_worker() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(task_mutex_);
                    task_cond_var_.wait(lock, [this] { return stop_thread_pool_ || !task_queue_.empty(); });
                    if (stop_thread_pool_ && task_queue_.empty()) return;
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
                task();
            }
        }

    public:
        TCPMessageIterator(short port, size_t thread_pool_size = 4)
                : listen_sockfd_(-1), conn_sockfd_(-1), stop_thread_(false), stop_thread_pool_(false) {
            listen_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (listen_sockfd_ < 0) {
                perror("socket");
                throw std::runtime_error("Failed to create socket");
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            server_addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(listen_sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("bind");
                close(listen_sockfd_);
                throw std::runtime_error("Failed to bind socket");
            }

            if (listen(listen_sockfd_, 1) < 0) {
                perror("listen");
                close(listen_sockfd_);
                throw std::runtime_error("Failed to listen on socket");
            }

            std::cout << "Waiting for a connection on port " << port << "..." << std::endl;
            conn_sockfd_ = accept(listen_sockfd_, nullptr, nullptr);
            if (conn_sockfd_ < 0) {
                perror("accept");
                close(listen_sockfd_);
                throw std::runtime_error("Failed to accept connection");
            }

            std::cout << "Connection accepted" << std::endl;

            listen_thread_ = std::thread(&TCPMessageIterator::listen_for_messages, this);

            for (size_t i = 0; i < thread_pool_size; ++i) {
                thread_pool_.emplace_back(&TCPMessageIterator::thread_pool_worker, this);
            }
        }

        ~TCPMessageIterator() {
            stop_thread_ = true;
            if (listen_thread_.joinable()) {
                listen_thread_.join();
            }
            if (conn_sockfd_ != -1) {
                close(conn_sockfd_);
            }
            if (listen_sockfd_ != -1) {
                close(listen_sockfd_);
            }

            {
                std::lock_guard<std::mutex> lock(task_mutex_);
                stop_thread_pool_ = true;
            }
            task_cond_var_.notify_all();
            for (auto& thread : thread_pool_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        std::vector<unsigned char> new_message() {
            BOOST_LOG_TRIVIAL(info) << "new_message()";
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this] { return !messages_.empty(); });
            auto message = std::move(messages_.front());
            messages_.erase(messages_.begin());
            return message;
        }
    };

    class TCPMessageStream {
    public:
        explicit TCPMessageStream(short port) : iterator_(port) {}

        TCPMessageIterator& begin() { return iterator_; }
    private:
        TCPMessageIterator iterator_;
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
