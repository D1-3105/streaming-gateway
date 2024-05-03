//
// Created by oleg on 22.04.24.
//

#include "WebCam.h"

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void wc_daemon::WebCamStreamDaemon::PutOnSHMQueue(void *iter_holder) {
    if (!iter_holder) {
        BOOST_LOG_TRIVIAL(error) << "Error: iter_holder is a null pointer!";
        return;
    }

    std::lock_guard<std::mutex> guard(mu_);
    auto stream = static_cast<webcam::WebCamStream*>(iter_holder);
    cv::Mat mat = stream->begin().operator*();

    if (!mat.isContinuous()) {
        mat = mat.clone();
    }

    size_t data_bytes = mat.total() * mat.elemSize();

    auto* msg = new ipc_queue::Message;
    msg->dataLength = data_bytes;
    msg->data = new u_char[data_bytes];
    memcpy(msg->data, mat.data, data_bytes);
    msg->nextMessageStart = 0;
    ipc_queue::Enqueue(*memoryManager, msg, region_name_);
}


void* wait_for_messages(const ulong* value, void *arg)
{
    pthread_mutex_lock(&mutex);
    while (*value <= 0)
    {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

void wc_daemon::WebCamStreamDaemon::ListenSHMQueue
(
        std::function<void*(const ipc_queue::Message *, size_t)> callback,
        long long prefetch_count
)
{
    std::lock_guard<std::mutex> guard(mu_);

    auto* message_buffer = new ipc_queue::Message[prefetch_count];
    auto metric = ipc_queue::GetQueueMetric(*memoryManager, region_name_);
    for (size_t i = 0; i < prefetch_count; i++)
    {
        wait_for_messages(&(metric->message_cnt), nullptr);
        message_buffer[i] = *ipc_queue::Deque(*memoryManager, region_name_);
    }
    callback(message_buffer, prefetch_count);
}


