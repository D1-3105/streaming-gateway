//
// Created by oleg on 22.04.24.
//

#include "WebCam.h"

int main() {
    try {
        webcam::WebCamStream stream(0);
        for (auto frame : stream) {
            cv::imshow("Webcam Stream", frame);
            if (cv::waitKey(1) == 'q') break;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "EXC: " << e.what() << std::endl;
        return -1;
    }
    cv::destroyAllWindows();
    return 0;
}