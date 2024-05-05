//
// Created by oleg on 05.05.24.
//
#include "utils.h"
#include <thread>
#include <chrono>

uint16_t crc16_table[256] = {0};

void utils::waitForSemaphoreValue(int sem_desc, int value)
{
    int sem_value = semctl(sem_desc, 0, GETVAL);
    while (sem_value != value) {
        sem_value = semctl(sem_desc, 0, GETVAL);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void generate_crc16_table()
{
    const uint16_t polynomial = 0xA001;
    for (uint16_t i = 0; i < 256; ++i) {
        uint16_t crc = i;
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
        crc16_table[i] = crc;
    }
}

uint16_t crc::crc16(const uint8_t *data, size_t length)
{
    if (crc16_table[0] == 0) {
        generate_crc16_table();
    }

    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; ++i) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }

    return ~crc;
}

uint8_t crc8_table[256] = {0};

void generate_crc8_table()
{
    const uint8_t polynomial = 0x1D;
    for (uint16_t i = 0; i < 256; ++i) {
        uint8_t crc = i;
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 0x80) ? (crc << 1) ^ polynomial : (crc << 1);
        }
        crc8_table[i] = crc;
    }
}

uint8_t crc::crc8(const uint8_t *data, size_t length)
{
    if (crc8_table[0] == 0) {
        generate_crc8_table();
    }

    uint8_t crc = 0;

    for (size_t i = 0; i < length; ++i) {
        crc = crc8_table[crc ^ data[i]];
    }

    return crc;
}

