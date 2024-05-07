//
// Created by oleg on 18.04.24.
//
#include "http.h"

void* readPlaylist(
        crow::response& resp, const std::string& play
)
{
    std::ifstream fragment_or_playlist(play);
    if (!fragment_or_playlist.is_open()) {
        resp.code = 404;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << fragment_or_playlist.rdbuf();
    std::string playlistContent = buffer.str();
    if(play.ends_with("m3u8"))
        resp.set_header("Content-Type", "application/x-mpegURL");
    else
        resp.set_header("Content-Type", "video/mp2t");
    resp.write(playlistContent);
    return nullptr;
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
    app.port(decoded->port).run();
    return nullptr;
}
