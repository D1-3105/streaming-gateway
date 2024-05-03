//
// Created by oleg on 24.04.24.
//

#ifndef STREAMINGGATEWAYCROW_EXPORTERS_H
#define STREAMINGGATEWAYCROW_EXPORTERS_H


#include "HandleStreamDaemon.h"
#include "opencv2/opencv.hpp"
#include "ipc_message_queue.h"
#include "regex"

namespace exporters {
    class BaseMessageHandler {
    protected:
        virtual void* ExportPartial(const ipc_queue::Message* marshaled_frame_matrix_, long long cnt) = 0;
        stream_daemon::HandleStreamDaemon* daemon_fetcher_ = nullptr;
    public:
        BaseMessageHandler() = default;
        explicit BaseMessageHandler(stream_daemon::HandleStreamDaemon* daemonFetcher): daemon_fetcher_(daemonFetcher){};
        virtual void* HandleMessages() = 0;
    };


    class HLSMessageHandler: public BaseMessageHandler {
    private:
        const size_t frame_rate_;
    public:
        explicit HLSMessageHandler(stream_daemon::HandleStreamDaemon* daemonFetcher, const size_t frame_rate):
                BaseMessageHandler(daemonFetcher), frame_rate_(frame_rate) {};

        [[noreturn]] void* HandleMessages() override;
    };

    class FileBaseHLSMessageHandler: public HLSMessageHandler {
    protected:
        const char* video_repository_;
        void* ExportPartial(const ipc_queue::Message* marshaled_frame_matrix_, long long cnt);
        static void unmarshalMessages(const ipc_queue::Message* marshaled_frame_matrix_, cv::Mat* output, long long cnt);

        void CreateHLSVideoSegment(cv::Mat* frames, int cnt);

    public:
        explicit FileBaseHLSMessageHandler(stream_daemon::HandleStreamDaemon *daemonFetcher, const size_t frame_rate,
                                           const char *video_repo) :
                HLSMessageHandler(daemonFetcher, frame_rate), video_repository_(video_repo) {};
    };
};


#endif //STREAMINGGATEWAYCROW_EXPORTERS_H
