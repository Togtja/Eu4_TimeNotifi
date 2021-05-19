#include "WindowsMem.hpp"

WindowsMem::WindowsMem(const std::string& program) : m_program(program) {
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

std::vector<uint8_t> WindowsMem::read_address(const uint8_t* address, size_t size) {
    std::vector<uint8_t> mem_chunk;
    mem_chunk.resize(size);
    SIZE_T read;
    if (ReadProcessMemory(m_proc, address, mem_chunk.data(), mem_chunk.size(), &read)) {
        return mem_chunk;
    }
    return {};
}

std::vector<const uint8_t*> WindowsMem::find_byte_from_vector(const std::vector<uint8_t>& byte_check,
                                                              const std::vector<const uint8_t*>& vector) {
    std::vector<const uint8_t*> out;
    for (auto k : vector) {
        auto mem_chunk = read_address(k, byte_check.size());
        if (mem_chunk == byte_check) {
            out.push_back(k);
        }
    }
    return out;
}

std::vector<const uint8_t*> WindowsMem::scan_memory(uint8_t* address_low,
                                                    uint8_t* address_high,
                                                    const std::vector<uint8_t>& byte_check,
                                                    const uint32_t nr_threads) {
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
std::vector<const uint8_t*>
WindowsMem::scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check) {
    std::vector<const uint8_t*> addresses_found;

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

std::vector<const uint8_t*> WindowsMem::scan_memory(const std::vector<uint8_t>& byte_check, const uint32_t nr_threads) {
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
        ret = scan_memory(nullptr, reinterpret_cast<uint8_t*>(baseAddress), byte_check);
    }
    else {
        ret = scan_memory(nullptr, reinterpret_cast<uint8_t*>(baseAddress), byte_check, nr_threads);
    }
    auto end = std::chrono::steady_clock::now();
    spdlog::debug("spendt {}ms looking for results, and found {} results",
                  std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count(),
                  ret.size());
    return ret;
}