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
#include <iostream>

// OS specific
#include <Windows.h>

#include <TlHelp32.h>
#include <psapi.h>

#ifdef _WIN64
static const std::string dll_path = "H:\\_Programming\\C++\\Eu4_TimeNotifi\\build\\Visual Studio Build Tools 2017 Release - "
                                    "amd64\\Debug\\EU4_C++Time\\injection.dll";
#elif _WIN32
static const std::string dll_path = "H:\\_Programming\\C++\\Eu4_TimeNotifi\\build\\Visual Studio Build Tools 2017 Release - "
                                    "amd64_x86\\Debug\\EU4_C++Time\\injection.dll";
#endif

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

    template<class T>
    static void scan_memory_func(std::mutex& g_queue,
                                 std::mutex& g_found,
                                 std::vector<LowHighAddress>& address_queue,
                                 std::vector<const uint8_t*>& found,
                                 const T& find,
                                 WindowsMem* mem) {
        // Hey I will also work on the
        LowHighAddress address;
        while (true) {
            {
                std::lock_guard guard(g_queue);
                if (address_queue.empty()) {
                    // Nothing left in the queue, time to leave
                    break;
                }
                // Get next element in queue
                address = *address_queue.begin();
                address_queue.erase(address_queue.begin());
            }
            // Scan the memory of the assigned area we got
            auto sub_found = mem->scan_memory(address.address_low, address.address_high, find);
            if (!sub_found.empty()) {
                std::lock_guard guard(g_found);
                // Insert our finding
                found.insert(found.end(), sub_found.begin(), sub_found.end());
            }
            // Do it all again
        }
    }

    template<class T>
    std::vector<const uint8_t*> scan_memory_MT(uint8_t* address_low, uint8_t* address_high, const T& find) {
        std::mutex g_found{};
        std::vector<const uint8_t*> found;
        // -2 due to allready using atleast 1 thread
        const auto max_size = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(max_size);
        auto address_step = (address_high - address_low) / (max_size * 128);

        std::mutex g_queue{};
        std::vector<LowHighAddress> address_queue{};
        for (auto address = address_low; address < address_high; address += address_step) {
            address_queue.emplace_back(LowHighAddress{address, address + address_step});
        }
        for (size_t i = 0; i < max_size; i++) {
            threads[i] = std::thread(WindowsMem::scan_memory_func<T>,
                                     std::ref(g_queue),
                                     std::ref(g_found),
                                     std::ref(address_queue),
                                     std::ref(found),
                                     std::ref(find),
                                     this);
        }
        // This thread also joins the scanning
        scan_memory_func<T>(g_queue, g_found, address_queue, found, find, this);
        for (auto&& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        return found;
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
    std::vector<const uint8_t*> scan_memory(const T& find) {
        DWORD_PTR baseAddress = 0;
        DWORD_PTR maxAddress  = 0;
        DWORD cbNeeded;

        if (EnumProcessModules(m_proc, NULL, 0, &cbNeeded)) {
            if (cbNeeded) {
                LPBYTE moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, cbNeeded);

                if (moduleArrayBytes) {
                    unsigned int moduleCount;

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
        // Multi Thread
        std::cout << "MULTI TRACK DRIFTING\n";
        auto start_timer = std::chrono::steady_clock::now();
        auto out         = scan_memory_MT(nullptr, (uint8_t*)baseAddress, find);
        auto end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory_MT) "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count() << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        auto out2   = scan_memory_MT(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory_MT) "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count() << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory_MT(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory_MT) "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count() << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory_MT(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory_MT) "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count() << "ms to find value\n";

        // Single thread:
        std::cout << "\n\nSINGLE TRACK DRIFTING\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
                  << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
                  << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
                  << "ms to find value\n";
        start_timer = std::chrono::steady_clock::now();
        out2        = scan_memory(nullptr, (uint8_t*)baseAddress, find);
        end         = std::chrono::steady_clock::now();
        std::cout << "Spendt (scan_memory) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
                  << "ms to find value\n";
        return out;
    }

    // void scan_inject(int find, bool new_run) { m_dll_inject.inject(find, new_run); }
};
