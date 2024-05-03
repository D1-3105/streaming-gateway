//
// Created by oleg on 24.04.24.
//

#include "exporters.h"

[[noreturn]] void* exporters::HLSMessageHandler::HandleMessages() {
    while(true)
    {
        auto boundFunc = [this](auto && PH1, auto && PH2)
        {
            return ExportPartial(
                    std::forward<decltype(PH1)>(PH1),
                    std::forward<decltype(PH2)>(PH2)
            );
        };
        daemon_fetcher_->ListenSHMQueue(boundFunc, 5 * frame_rate_);
    }
}


void *exporters::FileBaseHLSMessageHandler::ExportPartial(const ipc_queue::Message *marshaled_frame_matrix_,
                                                          long long int cnt)
{
    auto *frames = new cv::Mat[cnt];
    unmarshalMessages(marshaled_frame_matrix_, frames, cnt);

    return nullptr;
}

void exporters::FileBaseHLSMessageHandler::unmarshalMessages(const ipc_queue::Message *marshaled_frame_matrix_,
                                                             cv::Mat *output, long long cnt)
{
    for(size_t i = 0; i < cnt; i++)
    {
        auto message = marshaled_frame_matrix_[i];
        output[i] = *((cv::Mat*) message.data);
    }
}

int readLastSegmentNumber(const std::string& playlistPath) {
    std::ifstream m3u8File(playlistPath);
    std::string line;
    std::regex tsRegex("output(\\d+)\\.ts");
    int lastSegment = -1;

    if (m3u8File.is_open()) {
        while (std::getline(m3u8File, line)) {
            std::smatch match;
            if (std::regex_search(line, match, tsRegex) && match.size() > 1) {
                lastSegment = std::max(lastSegment, stoi(match[1].str()));
            }
        }
        m3u8File.close();
    }
    return lastSegment;
}

void exporters::FileBaseHLSMessageHandler::CreateHLSVideoSegment(cv::Mat *frames, int cnt)
{
    std::string playlistPath(std::string(video_repository_) + "/index.m3u8");
    int segmentNumber = readLastSegmentNumber(playlistPath) + 1;
    std::string filename = "output" + std::to_string(segmentNumber) + ".ts";

    cv::Size frameSize = frames[0].size();

    cv::VideoWriter writer(
            std::string(video_repository_) + "/" + filename,
            cv::VideoWriter::fourcc('H', '2', '6', '4'),
            30,
            frameSize,
            true
    );

    for (int i = 0; i < cnt; ++i) {
        writer.write(frames[i]);
    }
    writer.release();

    std::ofstream m3u8File(playlistPath, std::ios::app);
    m3u8File << "#EXTINF:10.0,\n" << filename << "\n";
    m3u8File.close();
}
