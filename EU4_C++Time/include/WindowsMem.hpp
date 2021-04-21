#pragma once

// OS specific
#include <Windows.h>

#include <TlHelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Note: Read only
class WindowsMem {
private:
    HANDLE m_proc;
    std::string m_program;

public:
    WindowsMem(std::string program);
    ~WindowsMem();

    template<class T>
    std::unordered_map<uint8_t*, T> find_byte(T find) {
        std::cout << "Searching for value\n";

        std::unordered_map<uint8_t*, T> mapping;

        // Turn the T bytes to a vector to use nice C++ functions
        std::vector<uint8_t> byte_check;
        if constexpr (std::is_same_v<T, std::string>) {
            byte_check = std::vector<uint8_t>(find.begin(), find.end());
        }
        else {
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&find));
            byte_check    = std::vector<uint8_t>(data, data + sizeof(find));
        }
        MEMORY_BASIC_INFORMATION info;
        for (uint8_t* addr = nullptr; VirtualQueryEx(m_proc, addr, &info, sizeof(info));) {
            auto base = static_cast<uint8_t*>(info.BaseAddress);
            if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE)) {
                size_t read{};
                std::vector<uint8_t> mem_chunk;
                mem_chunk.reserve(info.RegionSize);
                if (ReadProcessMemory(m_proc, addr, mem_chunk.data(), info.RegionSize, &read)) {
                    for (auto pos = mem_chunk.begin();
                         mem_chunk.end() != (pos = std::search(pos, mem_chunk.end(), byte_check.begin(), byte_check.end()));
                         pos += byte_check.size()) {
                        uint8_t* int_addr_ptr = (addr + (pos - mem_chunk.begin()));
                        mapping[int_addr_ptr] = find;
                    }
                }
            }
            addr = base + info.RegionSize;
        }
        return mapping;
    }

    template<class T>
    std::vector<void*> find_value2(T find) {
        std::cout << "Searching for value\n";

        std::vector<void*> found;

        uint8_t* data         = static_cast<uint8_t*>(static_cast<void*>(&find));
        const auto byte_check = std::vector<uint8_t>(data, data + sizeof(find));

        uint8_t* addr = nullptr;
        std::mutex lock;
        std::vector<std::thread> workers;
        std::vector<std::future<std::vector<uint8_t*>>> worker_future;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
            std::promise<std::vector<uint8_t*>> p;
            worker_future.emplace_back(p.get_future());
            workers.emplace_back(
                [&, m_proc = m_proc, byte_check](std::promise<std::vector<uint8_t*>>&& p) mutable {
                    MEMORY_BASIC_INFORMATION info;
                    std::vector<uint8_t> mem_chunk;
                    std::vector<uint8_t*> found_adr;
                    while (true) {
                        {
                            std::lock_guard guard(lock);
                            if (!VirtualQueryEx(m_proc, addr, &info, sizeof(info))) {
                                break;
                            }
                        }
                        auto base = static_cast<uint8_t*>(info.BaseAddress);

                        if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE)) {
                            mem_chunk.reserve(info.RegionSize);

                            size_t read;
                            if (ReadProcessMemory(m_proc, info.BaseAddress, mem_chunk.data(), info.RegionSize, &read)) {
                                std::lock_guard guard(lock);
                                auto start = mem_chunk.data(), end = start + read, pos = start;

                                if (addr > base) {
                                    pos += (addr - base);
                                }
                                while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                                    found_adr.push_back(base + (pos - start));
                                    pos += byte_check.size();
                                }
                            }
                        }
                        addr = base + info.RegionSize;
                    }
                    p.set_value(found_adr);
                },
                std::move(p));
        }

        for (auto&& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        for (auto&& f : worker_future) {
            auto sub_found = f.get();
            if (!sub_found.empty()) {
                found.insert(found.end(), sub_found.begin(), sub_found.end());
            }
        }

        auto end = std::chrono::steady_clock::now();
        std::cout << "Spend " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms to find value\n";
        return found;
    }

    template<class T>
    static void find_value_sub(MEMORY_BASIC_INFORMATION info,
                               std::vector<uint8_t> mem_chunk,
                               std::vector<const uint8_t> byte_check,
                               const uint8_t* addr,
                               std::vector<const uint8_t*> found) {
        size_t read;
        if (ReadProcessMemory(m_proc, info.BaseAddress, mem_chunk.data(), info.RegionSize, &read)) {
            auto start = mem_chunk.data(), end = start + read, pos = start;

            if (addr > info.BaseAddress) {
                pos += (addr - info.BaseAddress);
            }

            while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                found.push_back(static_cast<uint8_t*>(info.BaseAddress) + (pos - start));
                pos += byte_check.size();
            }
        }
    }

    template<class T>
    std::vector<const uint8_t*> find_value(const T& find) {
        std::cout << "Searching for value\n";

        std::vector<uint8_t> byte_check;
        if constexpr (std::is_same_v<T, std::string>) {
            byte_check = std::vector<uint8_t>(find.begin(), find.end());
        }
        else {
            const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&find));
            byte_check          = std::vector<uint8_t>(data, data + sizeof(find));
        }

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
        auto start = std::chrono::steady_clock::now();
        while (VirtualQueryEx(m_proc, addr, &info, sizeof(info))) {
            // TODO: Multi thread this
            base = static_cast<uint8_t*>(info.BaseAddress);
            if (worker.size() < max_threads) {
                if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE) && (info.Protect & pmask) &&
                    !(info.Protect & PAGE_GUARD)) {
                    auto info_copy = info;
                    std::promise<std::vector<uint8_t*>> p;
                    worker_future.push_back(p.get_future());
                    worker.push_back(std::thread(
                        [&, proc = m_proc, info, byte_check, working_thread, addr](
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
                                while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                                    found.emplace_back(static_cast<uint8_t*>(info.BaseAddress) + (pos - start));
                                    pos += byte_check.size();
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
        std::vector<const uint8_t*> out;
        for (auto&& i : found) {
            out.emplace_back(i);
        }

        auto end = std::chrono::steady_clock::now();
        std::cout << "Spend " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms to find value\n";

        return out;
    }

    template<class T>
    std::unordered_map<uint8_t, T> find_byte_from_map(T find, const std::unordered_map<uint8_t, T>& map) {
        std::unordered_map<uint8_t, T> out;
        for (const auto [k, v] : map) {
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
            if (ReadProcessMemory(m_proc, &k, mem_chunk.data(), mem_chunk.size(), &read)) {
                if (mem_chunk == byte_check) {
                    out[k] = find;
                }
            }
        }
        return out;
    }

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

    std::vector<uint8_t> read_address(uint8_t address, size_t size) {
        std::vector<uint8_t> mem_chunk;
        mem_chunk.resize(size);
        size_t read;
        if (ReadProcessMemory(m_proc, &address, mem_chunk.data(), mem_chunk.size(), &read)) {
            return mem_chunk;
        }
        return {};
    }

    template<typename T>
    void bytes_to_T(uint8_t addr, T& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            auto bytes = read_address(addr, v.size());
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

    std::vector<uint8_t> read_address2(const uint8_t* address, size_t size) {
        std::vector<uint8_t> mem_chunk;
        mem_chunk.resize(size);
        size_t read;
        if (ReadProcessMemory(m_proc, address, mem_chunk.data(), mem_chunk.size(), &read)) {
            return mem_chunk;
        }
        return {};
    }

    template<typename T>
    void bytes_to_T2(const uint8_t* addr, T& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            auto bytes = read_address2(addr, v.size());
            val        = std::string(bytes.begin(), bytes.end());
        }
        else {
            T res;
            auto bytes   = read_address2(addr, sizeof(T));
            uint8_t* ptr = (uint8_t*)&res;
            for (int i = 0; i < bytes.size(); i++) {
                *ptr++ = bytes[i];
            }
            val = res;
        }
    }

    // Scan Memory
    template<class T>
    std::vector<const uint8_t*> scan_memory(void* address_low, std::size_t nbytes, const T& find) {
        std::vector<const uint8_t*> addresses_found;

        const uint8_t *find_begin, *find_end;
        size_t find_size;

        if constexpr (std::is_same_v<T, std::string>) {
            find_begin = reinterpret_cast<const uint8_t*>(find.c_str());
            find_size  = find.size();
        }
        else {
            find_begin = reinterpret_cast<const uint8_t*>(&find);
            find_size  = sizeof(find);
        }

        find_end = find_begin + find_size;

        // all readable pages: adjust this as required
        const DWORD pmask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE | PAGE_EXECUTE_READ |
                            PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

        ::MEMORY_BASIC_INFORMATION mbi{};

        uint8_t* address      = static_cast<uint8_t*>(address_low);
        uint8_t* address_high = address + nbytes;

        while (address < address_high && ::VirtualQueryEx(m_proc, address, std::addressof(mbi), sizeof(mbi))) {
            // committed memory, readable, wont raise exception guard page
            // if( (mbi.State==MEM_COMMIT) && (mbi.Protect|pmask) && !(mbi.Protect&PAGE_GUARD) )
            if ((mbi.State == MEM_COMMIT) && (mbi.Protect & pmask) && !(mbi.Protect & PAGE_GUARD)) {
                const uint8_t* begin = static_cast<const uint8_t*>(mbi.BaseAddress);
                const uint8_t* end   = begin + mbi.RegionSize;

                const uint8_t* found = std::search(begin, end, find_begin, find_end);
                while (found != end) {
                    addresses_found.push_back(found);
                    found = std::search(found + 1, end, find_begin, find_end);
                }
            }

            address += mbi.RegionSize;
            mbi = {};
        }

        return addresses_found;
    }

    template<class T>
    std::vector<const uint8_t*> scan_memory(const T& find) {
        HMODULE base[1024];
        HMODULE basee;
        DWORD cbNeeded;
        if (EnumProcessModules(m_proc, base, sizeof(base), &cbNeeded)) {
            for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                TCHAR szModName[MAX_PATH];

                // Get the full path to the module's file.

                if (GetModuleFileNameEx(m_proc, base[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                    // Print the module name and handle value.
                    std::string test_string(szModName, sizeof(szModName) / sizeof(TCHAR));
                    if (test_string.find(m_program)) {
                        basee = base[i];
                        break;
                    }
                }
            }
        }
        if (basee == nullptr)
            return {};

        MODULEINFO minfo{};
        ::GetModuleInformation(GetCurrentProcess(), basee, std::addressof(minfo), sizeof(minfo));
        return scan_memory(basee, minfo.SizeOfImage, find);
    }
};

WindowsMem::WindowsMem(std::string program) {
    m_program = program;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (_stricmp(entry.szExeFile, program.c_str()) == 0) {
                m_proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
                CloseHandle(snapshot);
                return;
            }
        }
    }

    CloseHandle(snapshot);
    m_proc = NULL;
    throw std::runtime_error(std::string("failed to find program: " + program));
}

WindowsMem::~WindowsMem() {
    if (m_proc != NULL) {
        CloseHandle(m_proc);
    }
}
