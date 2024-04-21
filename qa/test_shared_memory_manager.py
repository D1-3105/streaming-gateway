import ctypes
import threading
import unittest

libshared_memory_manager = ctypes.CDLL('../cmake-build-debug/libshared_memory_manager.so')


SharedMemoryManager_new = libshared_memory_manager.SharedMemoryManager_new
SharedMemoryManager_new.restype = ctypes.c_void_p

SharedMemoryManager_delete = libshared_memory_manager.SharedMemoryManager_delete
SharedMemoryManager_delete.argtypes = [ctypes.c_void_p]

RegisterSystemSharedMemory = libshared_memory_manager.RegisterSystemSharedMemory
RegisterSystemSharedMemory.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]

Unregister = libshared_memory_manager.Unregister
Unregister.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

GetMemoryInfo = libshared_memory_manager.GetMemoryInfo
GetMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p]

UnlockManagerMutex = libshared_memory_manager.UnlockManagerMutex
UnlockManagerMutex.argtypes = [ctypes.c_void_p]

MutexState = libshared_memory_manager.MutexState
MutexState.argtypes = [ctypes.c_void_p]


class TestSharedMemoryManager(unittest.TestCase):

    def setUp(self):
        self.lock = threading.Lock()
        self.manager = SharedMemoryManager_new()

    def tearDown(self):
        SharedMemoryManager_delete(self.manager)

    def test_register_and_unregister(self):
        name = b"test_shm11"
        shm_key = b"test_key32"
        offset = 0
        byte_size = 1024

        with self.lock:
            print("RegisterSystemSharedMemory in test_register_and_unregister")

            RegisterSystemSharedMemory(self.manager, name, shm_key, offset, byte_size)
            self.assertFalse(MutexState(self.manager))

            print("Unregister in test_register_and_unregister")
            Unregister(self.manager, name)
            self.assertFalse(MutexState(self.manager))

    def test_get_memory_info(self):
        name = b"test_shm111"
        shm_key = b"test_key23"
        offset = 0
        byte_size = 1024

        with self.lock:
            print("RegisterSystemSharedMemory in test_get_memory_info")
            RegisterSystemSharedMemory(self.manager, name, shm_key, offset, byte_size)
            self.assertFalse(MutexState(self.manager))

            shm_mapped_addr = ctypes.c_void_p()

            print("GetMemoryInfo in test_get_memory_info")
            GetMemoryInfo(self.manager, name, offset, byte_size, ctypes.pointer(shm_mapped_addr))
            self.assertFalse(MutexState(self.manager))
            self.assertNotEqual(shm_mapped_addr.value, 0)


if __name__ == '__main__':
    unittest.main()
