//
// Created by oleg on 05.06.24.
//

#ifndef STREAMINGGATEWAYCROW_TCP_PRODUCING_H
#define STREAMINGGATEWAYCROW_TCP_PRODUCING_H
#include "TCPPublisher.h"
#include "../src/WebCam.h"
class TCPFrameProducer {
private:
    TCPPublisher* publisher_;
    webcam::WebCamStream* iterator_;
protected:
    void HandleWebcamFrame();
public:
    TCPFrameProducer(std::string& host, short port, short stream_device = 0);

    [[noreturn]] void Run();
};

#endif //STREAMINGGATEWAYCROW_TCP_PRODUCING_H
