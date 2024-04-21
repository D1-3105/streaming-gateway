//
// Created by oleg on 18.04.24.
//

#include <boost/log/trivial.hpp>
#include <memory>
#include <rapidjson/document.h>
#include "shared_memory_manager.h"

// includes

SharedMemoryManager* SharedMemoryManager_new()
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();
    BOOST_LOG_TRIVIAL(info) << "SharedMemoryManager_new - executed";
    auto res = new SharedMemoryManager();
    BOOST_LOG_TRIVIAL(info) << "SharedMemoryManager_new - done";
    BOOST_LOG_TRIVIAL(info) << "mu_ is locked: " << res->IsLocked();
    return res;
}

void SharedMemoryManager_delete(SharedMemoryManager* manager)
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();

    BOOST_LOG_TRIVIAL(info) << "SharedMemoryManager_delete - executed";
    delete manager;
    BOOST_LOG_TRIVIAL(info) << "SharedMemoryManager_delete - done";
}

void RegisterSystemSharedMemory(SharedMemoryManager* manager, const char* name, const char* shm_key, int offset, int byte_size)
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();

    BOOST_LOG_TRIVIAL(info) << "RegisterSystemSharedMemory - executed";
    manager->RegisterSystemSharedMemory(name, shm_key, offset, byte_size);
    BOOST_LOG_TRIVIAL(info) << "RegisterSystemSharedMemory - done";
    BOOST_LOG_TRIVIAL(info) << "mu_ is locked: " << manager->IsLocked();
}

void Unregister(SharedMemoryManager* manager, const char* name)
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();

    BOOST_LOG_TRIVIAL(info) << "Unregister - executed";
    manager->Unregister(name);
    BOOST_LOG_TRIVIAL(info) << "Unregister - done";
    BOOST_LOG_TRIVIAL(info) << "mu_ is locked: " << manager->IsLocked();
}

std::string GetMemoryInfo(
        SharedMemoryManager* manager,
        const std::string& name,
        size_t offset,
        size_t byte_size,
        void **shm_mapped_addr)
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();

    BOOST_LOG_TRIVIAL(info) << "mu_ is locked: " << manager->IsLocked();
    BOOST_LOG_TRIVIAL(info) << "GetMemoryInfo - executed";
    manager->GetMemoryInfo(name, offset, byte_size, shm_mapped_addr);
    BOOST_LOG_TRIVIAL(info) << "GetMemoryInfo - done";
    BOOST_LOG_TRIVIAL(info) << "mu_ is locked: " << manager->IsLocked();

    return name;
}

void UnlockManagerMutex(SharedMemoryManager* manager)
{
    BOOST_LOG_TRIVIAL(info) << "thread #" << std::this_thread::get_id();

    manager->UnlockMu();
}

bool MutexState(SharedMemoryManager* manager)
{

    return manager->IsLocked();
}


// source code



nullptr_t
GetSharedMemoryRegionSize(
        const std::string& shm_key, int shm_fd, size_t* shm_region_size)
{
    struct stat file_status;
    if (fstat(shm_fd, &file_status) == -1) {
        BOOST_LOG_TRIVIAL(error) << "fstat on shm_fd failed, errno: " << errno;
        throw SharedMemoryException("Exception during fstat");
    }

    // According to POSIX standard, type off_t can be negative, so for sake of
    // catching possible under/overflows, assert that the size is non-negative.
    if (file_status.st_size < 0) {
        BOOST_LOG_TRIVIAL(error) << "File size of shared memory region must be non-negative";
        throw SharedMemoryException("file_status.st_size < 0!");
    }
    BOOST_LOG_TRIVIAL(debug) << file_status.st_size;
    *shm_region_size = static_cast<size_t>(file_status.st_size);
    return nullptr;  // success
}


nullptr_t
CheckSharedMemoryRegionSize(
        const std::string& name, const std::string& shm_key, int shm_fd,
        size_t offset, size_t byte_size)
{
    size_t shm_region_size = 0;
    GetSharedMemoryRegionSize(shm_key, shm_fd, &shm_region_size);
    // User-provided offset and byte_size should not go out-of-bounds.
    if ((offset + byte_size) > shm_region_size) {
        BOOST_LOG_TRIVIAL(error) << std::string(
                                    "failed to register shared memory region '" + name +
                                    "': invalid args: ")
                                    .c_str() << (offset + byte_size) << " bytes of " << shm_region_size;
        throw SharedMemoryException("failed to register shared memory region");
    }

    return nullptr;  // success
}

nullptr_t
MapSharedMemory(
        const int shm_fd, const size_t offset, const size_t byte_size,
        void** mapped_addr)
{
    // map shared memory to process address space
    *mapped_addr =
            mmap(nullptr, byte_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, offset);
    if (*mapped_addr == MAP_FAILED) {
        BOOST_LOG_TRIVIAL(error) << std::string(
                            "unable to process address space" +
                            std::string(std::strerror(errno)))
                            .c_str();
        throw SharedMemoryException("Exception during mmap!");
    } else {
        BOOST_LOG_TRIVIAL(debug) << "Map - " << *mapped_addr;
    }

    return nullptr;
}

nullptr_t
OpenSharedMemoryRegion(const std::string& shm_key, int* shm_fd, size_t byte_size)
{
    // get shared memory region descriptor
    *shm_fd = shm_open(shm_key.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (*shm_fd == -1) {
        BOOST_LOG_TRIVIAL(error) << "shm_open failed, errno: " << errno;
        throw SharedMemoryException("shm_open failed");
    }
    // Set the size of the shared memory region
    if (ftruncate(*shm_fd, (long) byte_size) == -1) {
        BOOST_LOG_TRIVIAL(error) << "ftruncate failed, errno: " << errno;
        throw SharedMemoryException("ftruncate failed");
    }
    return nullptr;
}


nullptr_t
CloseSharedMemoryRegion(int shm_fd)
{
    int status = close(shm_fd);
    if (status == -1) {
        BOOST_LOG_TRIVIAL(error) << std::string(
                                    "unable to close shared memory descriptor, errno: " +
                                    std::string(std::strerror(errno)))
                                    .c_str();
        throw SharedMemoryException("Unable to close shared memory descriptor");
    }

    return nullptr;
}


nullptr_t
SharedMemoryManager::RegisterSystemSharedMemory(
        const std::string &name, const std::string &shm_key,
        const size_t offset, const size_t byte_size) {
    BOOST_LOG_TRIVIAL(debug) << "RegisterSystemSharedMemory - acquire lock " << MutexState(this);
    SharedLockGuard<std::mutex> guard(mu_);

    if (shared_memory_map_.find(name) != shared_memory_map_.end()) {
        throw SharedMemoryException(name + "  is not accessible on shared_memory_map_");
    }

    // register
    void* mapped_addr;
    int shm_fd = -1;

    // don't re-open if shared memory is already open
    for (auto & itr : shared_memory_map_) {
        if (itr.second->shm_key_ == shm_key) {
            // FIXME: Consider invalid file descriptors after close
            shm_fd = itr.second->shm_fd_;
            break;
        }
    }

    // open and set new shm_fd if new shared memory key
    if (shm_fd == -1) {
        BOOST_LOG_TRIVIAL(info) << "New shm_fd creation;";
        OpenSharedMemoryRegion(shm_key, &shm_fd, byte_size);
        BOOST_LOG_TRIVIAL(info) << "New shm_fd created;" << shm_key << ";" << byte_size;
    }

    // Enforce that registered region is in-bounds of shm file object.
    CheckSharedMemoryRegionSize(name, shm_key, shm_fd, offset, byte_size);

    // Mmap and then close the shared memory descriptor
    auto err_mmap = MapSharedMemory(shm_fd, offset, byte_size, &mapped_addr);
    auto err_close = CloseSharedMemoryRegion(shm_fd);

    shared_memory_map_.insert(std::make_pair(
            name, std::make_unique<SharedMemoryInfo>(
                    name, shm_key, offset, byte_size, shm_fd, mapped_addr)));
    BOOST_LOG_TRIVIAL(debug) << "RegisterSystemSharedMemory - completed";
    BOOST_LOG_TRIVIAL(debug) << "RegisterSystemSharedMemory - lock acquired, " << MutexState(this);
    return nullptr;  // success
}

SharedMemoryManager::~SharedMemoryManager() {
    UnregisterAll();
}

nullptr_t SharedMemoryManager::GetMemoryInfo(
        const std::string &name,
        size_t offset,
        size_t byte_size,
        void **shm_mapped_addr)
{
    BOOST_LOG_TRIVIAL(debug) << "GetMemoryInfo - acquire lock " << MutexState(this);
    SharedLockGuard<std::mutex> guard(mu_);

    BOOST_LOG_TRIVIAL(debug) << "GetMemoryInfo - lock acquired, " << MutexState(this);

    auto it = shared_memory_map_.find(name);
    if (it == shared_memory_map_.end()) {
        BOOST_LOG_TRIVIAL(error) << "Unable to find shared memory region: '" + name + "'";
        throw SharedMemoryException("Shared memory region not found");
    }

    // Calculate the end of the valid region
    size_t shm_region_end = it->second->offset_ + it->second->byte_size_;
    if (offset > shm_region_end || offset + byte_size > shm_region_end) {
        BOOST_LOG_TRIVIAL(error) << "Requested memory out of bounds for shared memory region: '" + name + "'";
        throw SharedMemoryException("Memory access out of bounds");
    }

    *shm_mapped_addr = static_cast<void*>(static_cast<char*>(it->second->mapped_addr_) + offset);
    return nullptr;
}


nullptr_t
SharedMemoryManager::GetStatus(const std::string &name, rapidjson::Document *shm_status)
{
    BOOST_LOG_TRIVIAL(debug) << "GetStatus - lock acquire, " << MutexState(this);
    SharedLockGuard<std::mutex> guard(mu_);

    if (name.empty()) {
        for (const auto& shm_info : shared_memory_map_) {
            rapidjson::Document::AllocatorType& allocator = shm_status->GetAllocator();
            rapidjson::Value shm_region(rapidjson::kObjectType);
            shm_region.AddMember("name", rapidjson::Value(shm_info.first.c_str(), allocator), allocator);
            shm_region.AddMember("key", rapidjson::Value(shm_info.second->shm_key_.c_str(), allocator), allocator);
            shm_region.AddMember("offset", shm_info.second->offset_, allocator);
            shm_region.AddMember("byte_size", shm_info.second->byte_size_, allocator);
            shm_status->PushBack(shm_region, allocator);
        }
    } else {
        auto it = shared_memory_map_.find(name);
        if (it == shared_memory_map_.end()) {
            BOOST_LOG_TRIVIAL(error) << "Unable to find system shared memory region";
            throw SharedMemoryException("Unable to find system shared memory region");
        }

        rapidjson::Document::AllocatorType& allocator = shm_status->GetAllocator();
        rapidjson::Value shm_region(rapidjson::kObjectType);
        shm_region.AddMember("name", rapidjson::Value(it->second->name_.c_str(), allocator), allocator);
        shm_region.AddMember("key", rapidjson::Value(it->second->shm_key_.c_str(), allocator), allocator);
        shm_region.AddMember("offset", it->second->offset_, allocator);
        shm_region.AddMember("byte_size", it->second->byte_size_, allocator);
        shm_status->PushBack(shm_region, allocator);
    }

    return nullptr; // success
}

nullptr_t
SharedMemoryManager::Unregister(const std::string &name) {
    // Serialize all operations that write/read current shared memory regions
    BOOST_LOG_TRIVIAL(debug) << "Unregister - lock " << MutexState(this);;
    SharedLockGuard<std::mutex> guard(mu_);


    return UnregisterHelper(name);
}

nullptr_t
SharedMemoryManager::UnregisterAll() {
    BOOST_LOG_TRIVIAL(debug) << "UnregisterAll - lock " << MutexState(this);

    SharedLockGuard<std::mutex> guard(mu_);

    std::string error_message = "Failed to unregister the following ";
    std::vector<std::string> unregister_fails;
    // Serialize all operations that write/read current shared memory regions
    error_message += "system shared memory regions: ";
    for (auto it = shared_memory_map_.cbegin(), next_it = it;
         it != shared_memory_map_.cend(); it = next_it) {
        ++next_it;
        auto err = UnregisterHelper(it->first);
    }

    if (!unregister_fails.empty()) {
        for (const auto& unreg_fail : unregister_fails) {
            error_message += unreg_fail + " ,";
        }
        BOOST_LOG_TRIVIAL(error) << error_message;
        throw SharedMemoryException(error_message);
    }

    return nullptr;
}

nullptr_t
UnmapSharedMemory(void* mapped_addr, size_t byte_size)
{
    int status = munmap(mapped_addr, byte_size);
    if (status == -1) {
        BOOST_LOG_TRIVIAL(error) << std::string(
                                    "unable to munmap shared memory region, errno: " +
                                    std::string(std::strerror(errno)))
                                    .c_str();
        throw std::exception();
    }
    return nullptr;
}

nullptr_t
SharedMemoryManager::UnregisterHelper(
        const std::string& name) {
    // Must hold the lock on register_mu_ while calling this function.
    auto it = shared_memory_map_.find(name);
    if (it != shared_memory_map_.end()) {
        UnmapSharedMemory(it->second->mapped_addr_, it->second->byte_size_);
    }
    // Remove region information from shared_memory_map_
    shared_memory_map_.erase(it);
    return nullptr;
}

nullptr_t SharedMemoryManager::UnlockMu() {
    mu_.unlock();
    return nullptr;
}

bool SharedMemoryManager::IsLocked() {
    return !mu_.try_lock() || (mu_.unlock(), false);
}
