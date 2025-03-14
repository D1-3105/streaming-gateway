//
// Created by oleg on 18.04.24.
//

#ifndef STREAMING_GATEWAY_CROW_SHARED_MEMORY_MANAGER_H
#define STREAMING_GATEWAY_CROW_SHARED_MEMORY_MANAGER_H
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <thread>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>


template<typename Mutex>
class SharedLockGuard {
private:
    std::lock_guard<Mutex> lg;
public:
    typedef Mutex mutex_type;
    SharedLockGuard() = default;

    explicit SharedLockGuard(mutex_type& m): lg(m) {
        BOOST_LOG_TRIVIAL(debug) << "Lock guard enter";
    }

    ~SharedLockGuard() {
        BOOST_LOG_TRIVIAL(debug) << "Lock guard exit";
    }
};


class SharedMemoryException : public std::exception {
public:
    explicit SharedMemoryException(std::string  message) : msg_(std::move(message)) {}
    [[nodiscard]] const char* what() const noexcept override {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

struct SharedMemoryInfo {
    SharedMemoryInfo(
            std::string  name, std::string shm_key,
            const size_t offset, const size_t byte_size, const int shm_fd,
            void* mapped_addr)
            : name_(std::move(name)), shm_key_(std::move(shm_key)), offset_(offset),
              byte_size_(byte_size), shm_fd_(shm_fd), mapped_addr_(mapped_addr)

    {};

    std::string name_;
    std::string shm_key_;
    size_t offset_;
    size_t byte_size_;
    int shm_fd_;
    void* mapped_addr_;
};
using SharedMemoryStateMap =
        std::map<std::string, std::unique_ptr<SharedMemoryInfo>>;


class SharedMemoryManager {
public:
    SharedMemoryManager() = default;

    SharedMemoryManager(SharedMemoryManager& manager);

    ~SharedMemoryManager();

    nullptr_t RegisterSystemSharedMemory(
            const std::string& name, const std::string& shm_key, size_t offset,
            size_t byte_size);

    nullptr_t GetMemoryInfo(
            const std::string& name, size_t offset, size_t byte_size,
            void** shm_mapped_addr);

    nullptr_t Unregister(
            const std::string& name);

    nullptr_t UnregisterAll();

    nullptr_t UnlockMu();

    bool IsLocked();

    SharedMemoryInfo* GetMemoryMap(const std::string& name);

private:
    // A map between the name and the details of the associated
    // shared memory block
    SharedMemoryStateMap shared_memory_map_;
    // A mutex to protect the concurrent access to shared_memory_map_
    std::mutex mu_;

    nullptr_t UnregisterHelper(const std::string &name);
};


SharedMemoryManager* SharedMemoryManager_new();

void SharedMemoryManager_delete(SharedMemoryManager* manager);

void RegisterSystemSharedMemory(SharedMemoryManager* manager, const char* name, const char* shm_key, int offset, int byte_size);

void Unregister(SharedMemoryManager* manager, const char* name);

std::string GetMemoryInfo(
        SharedMemoryManager* manager,
        const std::string& name,
        size_t offset,
        size_t byte_size,
        void **shm_mapped_addr);

void UnlockManagerMutex(SharedMemoryManager* manager);

bool MutexState(SharedMemoryManager* manager);



#endif //STREAMING_GATEWAY_CROW_SHARED_MEMORY_MANAGER_H
