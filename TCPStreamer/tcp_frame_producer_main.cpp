//
// Created by oleg on 05.06.24.
//

#include "tcp_producing.h"
#include "../src/logging.h"


int main() {
    logging::init_logging();
    std::string host = getenv("HOST");
    auto port = (short) atoi(getenv("PORT"));
    bool enable_gzip = false;
    const char* gzip_env = getenv("GZIP");
    if (gzip_env != nullptr) {
        enable_gzip = std::strcmp(gzip_env, "true") == 0;
    }
    TCPFrameProducer prod(host, port, 0, enable_gzip);
    prod.Run();
}
