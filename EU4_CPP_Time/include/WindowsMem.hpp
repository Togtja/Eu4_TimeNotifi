#pragma once
// C++ Standard library
#include <algorithm>   //std::find
#include <chrono>      //std::chrono
#include <mutex>       //std::lockgyard, std::mutex
#include <queue>       //std::queue
#include <string>      //std::string
#include <thread>      //std::thread
#include <type_traits> //std::is_same_v
#include <vector>      //std::vector

// External Libaries
#include <spdlog/spdlog.h>

// OS specific
#include <Windows.h>

#include <TlHelp32.h>
#include <psapi.h>

class WindowsMem {
private:
    HANDLE m_proc;
    std::string m_program;

    struct LowHighAddress {
        uint8_t* address_low  = nullptr;
        uint8_t* address_high = nullptr;
    };

    struct MemInfo {
        PVOID baseAddr;
        SIZE_T regionSize;
    };

    static void scan_memory_func(std::mutex& g_queue,
                                 std::queue<MemInfo>& mem_queue,
                                 std::mutex& g_found,
                                 std::vector<const uint8_t*>& addresses_found,
                                 const std::atomic<bool>& searching,
                                 const std::vector<uint8_t>& byte_check,
                                 HANDLE proc) {
        bool queue_empty = false;
        MemInfo info{};
        while (searching || !queue_empty) {
            {
                std::lock_guard<std::mutex> guard(g_queue);
                if (mem_queue.empty()) {
                    queue_empty = true;
                    continue;
                }
                else {
                    info = mem_queue.front();
                    mem_queue.pop();
                    queue_empty = false;
                }
            }

            std::vector<uint8_t> mem_chunk{};
            size_t read;
            mem_chunk.reserve(info.regionSize);
            if (ReadProcessMemory(proc, info.baseAddr, mem_chunk.data(), info.regionSize, &read)) {
                auto start = mem_chunk.data(), end = start + read, pos = start;
                while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                    {
                        std::lock_guard<std::mutex> guard(g_found);
                        addresses_found.push_back(static_cast<const uint8_t*>(info.baseAddr) + (pos - start));
                    }
                    pos += byte_check.size();
                }
            }
        }
    }

public:
    WindowsMem(const std::string& program);
    ~WindowsMem();

    std::vector<uint8_t> read_address(const uint8_t* address, size_t size);

    std::vector<const uint8_t*> find_byte_from_vector(const std::vector<uint8_t>& byte_check,
                                                      const std::vector<const uint8_t*>& vector);

    std::vector<const uint8_t*>
    scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check, const uint32_t nr_threads);

    std::vector<const uint8_t*> scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check);

    std::vector<const uint8_t*> scan_memory(const std::vector<uint8_t>& byte_check, const uint32_t nr_threads);

    // void scan_inject(int find, bool new_run) { m_dll_inject.inject(find, new_run); }
};
