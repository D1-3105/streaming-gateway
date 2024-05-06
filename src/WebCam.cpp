//
// Created by oleg on 22.04.24.
//

#include <sys/sem.h>
#include "WebCam.h"
#include "constants.h"
#include "utils.h"


void wc_daemon::WebCamStreamDaemon::PutOnSHMQueue(void *iter_holder) {
    if (!initialized_publisher_) {
        shm_queue::InitializeQueue(*memoryManager_, region_name_);
        int sema_desc = GetSema();

        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        } semarg;
        pid_t p = getppid();
        semarg.val = crc::crc8((uint8_t*)&p, sizeof(pid_t) / sizeof(uint8_t));  // Set semaphore to parent process ID
        int sem_res = semctl(sema_desc, 0, SETVAL, semarg);
        if (sem_res == -1)
        {
            perror("semctl");
            throw std::runtime_error("Error during semaphore acquiring!");
        }
        BOOST_LOG_TRIVIAL(info) << "Semaphore #" << sema_desc << " unlocked with value: " << semarg.val;
    }

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

void* wait_for_messages(SharedMemoryManager& manager, const char* region_name)
{
    while(true)
    {
        auto metric = shm_queue::GetQueueMetric(manager, region_name);
        std::cout << "METRIC: " << metric->message_cnt << "; " << metric->head_position << std::endl;
        if (metric->message_cnt) {
            delete metric;
            break;
        } else {
            delete metric;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return nullptr;
}

void wc_daemon::WebCamStreamDaemon::ListenSHMQueue
        (
                std::function<void*(const shm_queue::Message *, size_t)> callback,
                long long prefetch_count
        )
{
    std::lock_guard<std::mutex> guard(mu_);

    if (!initialized_publisher_)
    {
        BOOST_LOG_TRIVIAL(info) << "Waiting for publisher...";
        int semaphore_desc = GetSema();
        pid_t p = getppid();
        int sem_value = crc::crc8((uint8_t*)&p, sizeof(pid_t) / sizeof(uint8_t));
        BOOST_LOG_TRIVIAL(info) << "Expected handshake: " << sem_value;
        utils::waitForSemaphoreValue(semaphore_desc, sem_value);
        BOOST_LOG_TRIVIAL(info) << "Publisher online!";
    }
    initialized_publisher_ = true;

    auto* message_buffer = new shm_queue::Message[prefetch_count];
    size_t prefetched = 0;

    for (size_t i = 0; i < prefetch_count; i++)
    {
        wait_for_messages(*memoryManager_, region_name_);
        try {
            message_buffer[i] = *shm_queue::Deque(*memoryManager_, region_name_);
            prefetched ++;
        } catch(shm_queue::QueueException& e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            break;
        }
    }
    callback(message_buffer, prefetched);
}
