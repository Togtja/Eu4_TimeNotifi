#include <Windows.h>

#include <psapi.h>

#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

std::vector<uint8_t> read_address(const uint8_t* address, size_t size) {
    std::vector<uint8_t> mem_chunk;
    mem_chunk.resize(size);
    size_t read;
    if (ReadProcessMemory(GetCurrentProcess(), address, mem_chunk.data(), mem_chunk.size(), &read)) {
        return mem_chunk;
    }
    return {};
}

void bytes_to_int(const uint8_t* addr, int& val) {
    int res;
    auto bytes   = read_address(addr, sizeof(int));
    uint8_t* ptr = (uint8_t*)&res;
    for (int i = 0; i < bytes.size(); i++) {
        *ptr++ = bytes[i];
    }
    val = res;
}

std::vector<const void*> scan_memory(std::vector<const void*> addresses, const std::vector<uint8_t>& bytes_to_find) {
    std::vector<const void*> addresses_found;

    // all readable pages: adjust this as required
    const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

    MEMORY_BASIC_INFORMATION mbi{};
    for (auto&& address : addresses) {
        std::vector<uint8_t> mem_chunk;
        mem_chunk.resize(bytes_to_find.size());
        size_t read;
        if (ReadProcessMemory(GetCurrentProcess(), address, mem_chunk.data(), mem_chunk.size(), &read)) {
            if (mem_chunk == bytes_to_find) {
                addresses_found.push_back(address);
            }
        }
    }

    return addresses_found;
}

std::vector<const void*> scan_memory(const std::vector<uint8_t>& bytes_to_find) {
    uint8_t *addr = nullptr, *base;
    MEMORY_BASIC_INFORMATION info;
    // all readable pages: adjust this as required
    const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

    const int max_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> worker;
    std::vector<std::future<std::vector<uint8_t*>>> worker_future;
    std::unordered_map<std::thread::id, bool> working_thread;

    std::mutex mutex;
    std::mutex working_mutex;

    std::vector<uint8_t*> found;
    auto start  = std::chrono::steady_clock::now();
    auto m_proc = GetCurrentProcess();
    while (VirtualQuery(addr, &info, sizeof(info))) {
        // TODO: Multi thread this
        base = static_cast<uint8_t*>(info.BaseAddress);
        if (worker.size() < max_threads) {
            if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE) && (info.Protect & pmask) &&
                !(info.Protect & PAGE_GUARD)) {
                auto info_copy = info;
                std::promise<std::vector<uint8_t*>> p;
                worker_future.push_back(p.get_future());
                worker.push_back(std::thread(
                    [&, proc = m_proc, info, bytes_to_find, working_thread, addr](
                        std::promise<std::vector<uint8_t*>>&& p) mutable {
                        {
                            std::lock_guard<std::mutex> guard(working_mutex);
                            working_thread[std::this_thread::get_id()] = true;
                        }
                        std::vector<uint8_t*> found;
                        std::vector<uint8_t> mem_chunk;
                        mem_chunk.reserve(info.RegionSize);
                        size_t read;
                        if (ReadProcessMemory(proc, info.BaseAddress, mem_chunk.data(), info.RegionSize, &read)) {
                            auto start = mem_chunk.data(), end = start + read, pos = start;
                            if (addr > info.BaseAddress) {
                                pos += (addr - info.BaseAddress);
                            }
                            while ((pos = std::search(pos, end, bytes_to_find.begin(), bytes_to_find.end())) != end) {
                                found.emplace_back(static_cast<uint8_t*>(info.BaseAddress) + (pos - start));
                                pos += bytes_to_find.size();
                            }
                        }
                        p.set_value(found);

                        {
                            std::lock_guard<std::mutex> guard(working_mutex);
                            working_thread[std::this_thread::get_id()] = false;
                        }
                    },
                    std::move(p)));
            }
        }
        while (worker.size() >= max_threads) {
            for (int i = 0; i < worker.size();) {
                if (!working_thread[worker[i].get_id()] && worker[i].joinable()) {
                    worker[i].join();
                    auto work = worker_future[i].get();
                    if (!work.empty()) {
                        found.reserve(found.size() + work.size());
                        found.insert(found.end(), work.begin(), work.end());
                    }
                    worker.erase(worker.begin() + i);
                    worker_future.erase(worker_future.begin() + i);
                }
                else {
                    i++;
                }
            }
        };

        addr = base + info.RegionSize;
    }

    for (auto&& i : worker) {
        if (i.joinable()) {
            i.join();
        }
    }
    worker.clear();
    std::vector<const void*> out;
    for (auto&& i : found) {
        out.emplace_back(i);
    }

    auto end = std::chrono::steady_clock::now();
    std::cout << "Spend " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms to find value\n";

    return out;
}

// Way of DLL arguments from
// https://guidedhacking.com/threads/how-to-pass-multiple-arguments-with-createremotethread-to-injected-dll.15373/
struct Arguments {
    char dll_path[MAX_PATH];

    bool new_run = false; // If we should create new vector of found bytes
    int eu4_date = 10;
};

HMODULE this_dll = NULL;
Arguments* args  = nullptr;

std::vector<const void*> address{};

void find_bytes(HMODULE hMoudle) {
    if (args && args->new_run) {
        std::cout << "A WHOLE NEW RUN\n";
        int eu4             = args->eu4_date;
        const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&eu4));
        auto bytes          = std::vector<uint8_t>(data, data + sizeof(eu4));

        address = scan_memory(bytes);
        if (address.size() == 1) {
            std::cout << "We most likely found the address point";
        }
    }
    else if (args) {
        std::cout << "LOOKING IN ADDRESS LIST\n";
        int eu4             = args->eu4_date;
        const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&eu4));
        auto bytes          = std::vector<uint8_t>(data, data + sizeof(eu4));
        address             = scan_memory(address, bytes);
        if (address.size() == 1) {
            std::cout << "We most likely found the address point";
        }
    }
    else {
        std::cout << "failed to get args\n";
    }
}

extern "C" __declspec(dllexport) DWORD start_scan(Arguments* argstruct) {
    args = argstruct;
    find_bytes(this_dll);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            std::cout << "Hello world ???\n";
            this_dll = hModule;
            break;
        case DLL_PROCESS_DETACH: std::cout << "Process detached\n";
        case DLL_THREAD_ATTACH: std::cout << "Thread attached\n";
        case DLL_THREAD_DETACH: std::cout << "Thread detached\n";
        default: break;
    }
    return TRUE;
}