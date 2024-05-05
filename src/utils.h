//
// Created by oleg on 04.05.24.
//

#ifndef STREAMINGGATEWAYCROW_UTILS_H
#define STREAMINGGATEWAYCROW_UTILS_H

#include <cstddef>
#include <cstdint>
#include <sys/sem.h>


namespace crc {
    uint16_t crc16(const uint8_t *data, size_t length);
    uint8_t crc8(const uint8_t *data, size_t length);
}

namespace utils {
    void waitForSemaphoreValue(int sem_desc, int value);
}

#endif //STREAMINGGATEWAYCROW_UTILS_H
