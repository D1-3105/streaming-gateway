//
// Created by oleg on 24.04.24.
//

#include "exporters.h"
#include "constants.h"

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


void *exporters::FileBaseHLSMessageHandler::ExportPartial(const shm_queue::Message *marshaled_frame_matrix,
                                                          long long int cnt)
{
    size_t true_size = 0;
    for(;true_size < cnt; true_size++)
    {
        if(marshaled_frame_matrix[true_size].data == nullptr)
            break;
    }
    auto *frames = new cv::Mat[true_size];
    unmarshalMessages(marshaled_frame_matrix, frames, (size_t) true_size);
    CreateHLSVideoSegment(frames, true_size);
    return nullptr;
}

void exporters::FileBaseHLSMessageHandler::unmarshalMessages(const shm_queue::Message *marshaled_frame_matrix_,
                                                             cv::Mat *output, long long cnt)
{
    int cols, rows, type;
    size_t step;
    size_t mat_content_offset = 3 * video_constants::int2uchar + video_constants::size_t2uchar;
    for(size_t i = 0; i < cnt; i++)
    {
        auto message = marshaled_frame_matrix_[i];
        memcpy(&cols, message.data, sizeof(int));
        memcpy(&rows, message.data + video_constants::int2uchar, sizeof(int));
        memcpy(&type, message.data + video_constants::int2uchar * 2, sizeof(int));
        memcpy(&step, message.data + video_constants::int2uchar * 3, sizeof(size_t));
        auto* data = new uchar[message.dataLength - mat_content_offset];
        memcpy(data, message.data + mat_content_offset, message.dataLength - mat_content_offset);
        output[i] = cv::Mat(rows, cols, type, (void*)data, step);
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
    std::string filename = "output" + std::to_string(segmentNumber) + ".mp4";

    cv::Size frameSize = frames[0].size();
    auto fn = std::string(video_repository_) + "/" + filename;
    cv::VideoWriter writer;
    writer.open(
            fn,
            cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
            (double) frame_rate_,
            frameSize
    );
    if(not writer.isOpened())
        throw exporters::ExporterException("VideoWriter failed to open " + fn);
    for (int i = 0; i < cnt; ++i) {
        if(frameSize != frames[i].size())
        {
            throw exporters::ExporterException("Invalid frame_size!");
        }
        writer.write(frames[i]);
    }

    std::ofstream m3u8File(playlistPath, std::ios::app);
    m3u8File << "#EXTINF:10.0,\n" << filename << "\n";
    m3u8File.close();
}
