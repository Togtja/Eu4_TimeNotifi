#pragma once

// OS specific
#include <Windows.h>

#include <TlHelp32.h>

#include <algorithm>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Note: Read only
class WindowsMem {
private:
    HANDLE m_proc;

public:
    WindowsMem(std::string program);
    ~WindowsMem();

    template<class T>
    std::unordered_map<uint8_t, T> find_byte(T find) {
        std::cout << "Searching for value\n";

        std::unordered_map<uint8_t, T> mapping;

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
        for (uint8_t* addr = nullptr; VirtualQueryEx(m_proc, addr, &info, sizeof(info)) == sizeof(info);
             addr += info.RegionSize) {
            if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE)) {
                size_t read{};
                std::vector<uint8_t> mem_chunk;
                mem_chunk.resize(info.RegionSize);
                if (ReadProcessMemory(m_proc, addr, mem_chunk.data(), mem_chunk.size(), &read)) {
                    mem_chunk.resize(read);
                    for (auto pos = mem_chunk.begin();
                         mem_chunk.end() != (pos = std::search(pos, mem_chunk.end(), byte_check.begin(), byte_check.end()));
                         pos++) {
                        auto lpcvoid       = (addr + (pos - mem_chunk.begin()));
                        auto int_addr      = reinterpret_cast<const char*>(lpcvoid);
                        mapping[*int_addr] = find;
                    }
                }
            }
        }
        return mapping;
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
};

WindowsMem::WindowsMem(std::string program) {
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
