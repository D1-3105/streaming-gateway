//
// Created by oleg on 18.04.24.
//

#ifndef STREAMING_GATEWAY_CROW_HTTP_H
#define STREAMING_GATEWAY_CROW_HTTP_H
#include "../include/crow/crow_all.h"

namespace httpServer
{
    crow::SimpleApp buildApp();
    void serveApp(int port);
}

#endif //STREAMING_GATEWAY_CROW_HTTP_H
