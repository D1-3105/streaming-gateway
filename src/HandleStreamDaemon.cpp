//
// Created by oleg on 18.04.24.
//

#include "HandleStreamDaemon.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "utils.h"

int stream_daemon::HandleStreamDaemon::GetSema() {
    const std::string shm_key = memoryManager_->GetMemoryMap(region_name_)->shm_key_;
    int pr_id = crc::crc16((uint8_t*)shm_key.data(), shm_key.length());
    int semid_ = semget(pr_id, 1, IPC_CREAT | 0666);
    BOOST_LOG_TRIVIAL(info) << "Connected to semaphore: " << pr_id;
    return semid_;
}
