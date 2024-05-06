//
// Created by oleg on 18.04.24.
//
#include "http.h"

void* readPlaylist(
        crow::response& resp, const std::string& playlistPath
)
{
    std::ifstream m3u8File(playlistPath);
    if (!m3u8File.is_open()) {
        resp.code = 404;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << m3u8File.rdbuf();
    std::string playlistContent = buffer.str();
    resp.set_header("Content-Type", "application/x-mpegURL");
    resp.write(playlistContent);
}


crow::SimpleApp httpServer::buildApp(std::string& video_repository)
{
    crow::SimpleApp app;
    CROW_ROUTE(app, "/hls/<string>")
            ([&](const std::string& filename) {
                crow::response response;
                std::string playlistPath = video_repository + "/" + filename;
                readPlaylist(response, playlistPath);
                return response;
            });
    return app;
}

void* httpServer::serveApp(void* args)
{
    auto decoded = (ServerArguments*) args;
    auto app = buildApp(decoded->video_repository);
    app.port(decoded->port).multithreaded().run();
    return nullptr;
}
