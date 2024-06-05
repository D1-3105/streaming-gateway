//
// Created by oleg on 05.06.24.
//

#include "tcp_producing.h"
#include "../src/logging.h"


int main() {
    logging::init_logging();
    std::string host = getenv("HOST");
    auto port = (short) atoi(getenv("PORT"));
    TCPFrameProducer prod(host, port);
    prod.Run();
}
