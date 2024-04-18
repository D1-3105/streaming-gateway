//
// Created by oleg on 18.04.24.
//
#include "http.h"

crow::SimpleApp httpServer::buildApp()
{
    crow::SimpleApp app;
    CROW_ROUTE(app, "/")([](){
        return "Hello world";
    });
    return app;
}

void httpServer::serveApp(int port)
{
    auto app = buildApp();
    app.port(port).run();
}
