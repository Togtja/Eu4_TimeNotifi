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

public:
    WindowsMem(const std::string& program);
    ~WindowsMem();

    std::vector<uint8_t> read_address(const uint8_t* address, size_t size);

    template<class T>
    std::vector<const uint8_t*> find_byte_from_vector(T find, const std::vector<const uint8_t*>& vector) {
        std::vector<const uint8_t*> out;
        for (auto k : vector) {
            std::vector<uint8_t> byte_check;
            if constexpr (std::is_same_v<T, std::string>) {
                byte_check = std::vector<uint8_t>(find.begin(), find.end());
            }
            else {
                uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&find));
                byte_check    = std::vector<uint8_t>(data, data + sizeof(find));
            }
            std::vector<uint8_t> mem_chunk;
            mem_chunk.resize(byte_check.size());
            size_t read;
            if (ReadProcessMemory(m_proc, k, mem_chunk.data(), mem_chunk.size(), &read)) {
                if (mem_chunk == byte_check) {
                    out.push_back(k);
                }
            }
        }
        return out;
    }

    template<typename T>
    void bytes_to_T(const uint8_t* addr, T& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            auto bytes = read_address(addr, val.size());
            val        = std::string(bytes.begin(), bytes.end());
        }
        else {
            T res;
            auto bytes   = read_address(addr, sizeof(T));
            uint8_t* ptr = (uint8_t*)&res;
            for (int i = 0; i < bytes.size(); i++) {
                *ptr++ = bytes[i];
            }
            val = res;
        }
    }

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

    template<class T>
    std::vector<const uint8_t*>
    scan_memory(uint8_t* address_low, uint8_t* address_high, const T& find, const uint32_t nr_threads) {
        std::vector<uint8_t> byte_check;
        if constexpr (std::is_same_v<T, std::string>) {
            byte_check = std::vector<uint8_t>(find.begin(), find.end());
        }
        else {
            const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&find));
            byte_check          = std::vector<uint8_t>(data, data + sizeof(find));
        }

        // all readable pages: adjust this as required
        const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

        MEMORY_BASIC_INFORMATION info{};

        uint8_t* address = address_low;

        size_t read;
        std::mutex g_queue;
        std::queue<MemInfo> mem_queue{};

        std::mutex g_found;
        std::vector<const uint8_t*> addresses_found;

        uint32_t MAX_T = std::thread::hardware_concurrency() * 2;
        if (nr_threads < MAX_T) {
            MAX_T = nr_threads;
        }
        std::vector<std::thread> worker_threads(MAX_T);
        std::atomic<bool> searching = true;

        for (size_t i = 0; i < MAX_T; i++) {
            worker_threads[i] = std::thread(WindowsMem::scan_memory_func,
                                            std::ref(g_queue),
                                            std::ref(mem_queue),
                                            std::ref(g_found),
                                            std::ref(addresses_found),
                                            std::ref(searching),
                                            std::ref(byte_check),
                                            m_proc);
        }

        while (address < address_high && VirtualQueryEx(m_proc, address, &info, sizeof(info))) {
            if ((info.State == MEM_COMMIT) && (info.Protect & pmask) && !(info.Protect & PAGE_GUARD)) {
                std::lock_guard guard(g_queue);
                mem_queue.push({info.BaseAddress, info.RegionSize});
            }
            address += info.RegionSize;
        }
        searching = false;
        for (auto&& i : worker_threads) {
            if (i.joinable()) {
                i.join();
            }
        }

        return addresses_found;
    }

    // Scan Memory
    template<class T>
    std::vector<const uint8_t*> scan_memory(uint8_t* address_low, uint8_t* address_high, const T& find) {
        std::vector<const uint8_t*> addresses_found;

        std::vector<uint8_t> byte_check;
        if constexpr (std::is_same_v<T, std::string>) {
            byte_check = std::vector<uint8_t>(find.begin(), find.end());
        }
        else {
            const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&find));
            byte_check          = std::vector<uint8_t>(data, data + sizeof(find));
        }

        // all readable pages: adjust this as required
        const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

        MEMORY_BASIC_INFORMATION info{};

        uint8_t* address = address_low;

        size_t read;
        std::vector<uint8_t> mem_chunk{};

        while (address < address_high && VirtualQueryEx(m_proc, address, &info, sizeof(info))) {
            if ((info.State == MEM_COMMIT) && (info.Protect & pmask) && !(info.Protect & PAGE_GUARD)) {
                if (info.RegionSize >= mem_chunk.max_size()) {
                    continue;
                }
                mem_chunk.reserve(info.RegionSize);
                if (ReadProcessMemory(m_proc, info.BaseAddress, mem_chunk.data(), info.RegionSize, &read)) {
                    auto start = mem_chunk.data(), end = start + read, pos = start;

                    if (address > info.BaseAddress) {
                        pos += (address - static_cast<uint8_t*>(info.BaseAddress));
                    }

                    while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                        addresses_found.push_back(static_cast<const uint8_t*>(info.BaseAddress) + (pos - start));
                        pos += byte_check.size();
                    }
                }
            }
            address += info.RegionSize;
            info = {};
            mem_chunk.clear();
        }

        return addresses_found;
    }

    template<class T>
    std::vector<const uint8_t*> scan_memory(const T& find, const uint32_t nr_threads) {
        DWORD_PTR baseAddress = 0;
        DWORD_PTR maxAddress  = 0;
        DWORD cbNeeded;

        if (EnumProcessModules(m_proc, NULL, 0, &cbNeeded)) {
            if (cbNeeded) {
                LPBYTE moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, cbNeeded);

                if (moduleArrayBytes) {
                    uint32_t moduleCount;

                    moduleCount          = cbNeeded / sizeof(HMODULE);
                    HMODULE* moduleArray = (HMODULE*)moduleArrayBytes;

                    if (EnumProcessModules(m_proc, moduleArray, cbNeeded, &cbNeeded)) {
                        MODULEINFO minfo{};
                        GetModuleInformation(m_proc, moduleArray[moduleCount - 1], &minfo, sizeof(minfo));
                        baseAddress = (DWORD_PTR)moduleArray[0];
                        maxAddress  = (DWORD_PTR)moduleArray[moduleCount - 1] + minfo.SizeOfImage;
                    }

                    LocalFree(moduleArrayBytes);
                }
            }
        }
        std::vector<const uint8_t*> ret{};
        auto start_timer = std::chrono::steady_clock::now();
        if (nr_threads <= 1) {
            ret = scan_memory(nullptr, reinterpret_cast<uint8_t*>(baseAddress), find);
        }
        else {
            ret = scan_memory(nullptr, reinterpret_cast<uint8_t*>(baseAddress), find, nr_threads);
        }
        auto end = std::chrono::steady_clock::now();
        spdlog::debug("spendt {}ms looking for results, and found {} results",
                      std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count(),
                      ret.size());
        return ret;
    }

    // void scan_inject(int find, bool new_run) { m_dll_inject.inject(find, new_run); }
};
