#pragma once

#ifdef _WIN32
#    include "WindowsMem.hpp"
#else
#    include "UnixMem.hpp"
#endif

#include <memory>
#include <string>
#include <type_traits> //std::is_same_v
#include <vector>

class CrossMemory {
private:
    std::string m_exec_name{};
#ifdef _WIN32
    std::unique_ptr<WindowsMem> mem = nullptr;
#else
    std::unique_ptr<UnixMem> mem = nullptr;
#endif
    template<class T>
    std::vector<uint8_t> to_byte_vector(const T& find) {
        if constexpr (std::is_same_v<T, std::string>) {
            return std::vector<uint8_t>(find.begin(), find.end());
        }
        else {
            const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&find));
            return std::vector<uint8_t>(data, data + sizeof(find));
        }
    }

public:
    template<class T>
    std::vector<const uint8_t*> scan_memory(const T& find, const uint32_t nr_threads) {
        return mem->scan_memory(to_byte_vector(find), nr_threads);
    }

    template<typename T>
    void bytes_to_T(const uint8_t* addr, T& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            auto bytes = mem->read_address(addr, val.size());
            val        = std::string(bytes.begin(), bytes.end());
        }
        else {
            T res;
            auto bytes   = mem->read_address(addr, sizeof(T));
            uint8_t* ptr = (uint8_t*)&res;
            for (int i = 0; i < bytes.size(); i++) {
                *ptr++ = bytes[i];
            }
            val = res;
        }
    }

    template<class T>
    std::vector<const uint8_t*> find_byte_from_vector(const T& find, const std::vector<const uint8_t*>& vector) {
        return mem->find_byte_from_vector(to_byte_vector(find), vector);
    }

    // void scan_inject(int find, bool new_run) { return mem->scan_inject(find, new_run); }

    CrossMemory(const std::string& exec_name);
    ~CrossMemory();
};

CrossMemory::CrossMemory(const std::string& exec_name) : m_exec_name(exec_name) {
#ifdef _WIN32
    mem.reset(new WindowsMem(exec_name));
#else
    mem.reset(new UnixMem(exec_name));
#endif
}

CrossMemory::~CrossMemory() {}
