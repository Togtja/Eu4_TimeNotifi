#pragma once

#ifdef _WIN32
#    include "WindowsMem.hpp"
#else
#endif

#include <string>

class CrossMemory {
private:
    std::string m_exec_name{};
#ifdef _WIN32
    WindowsMem* mem = nullptr;
#else
#endif
public:
    template<class T>
    std::vector<const uint8_t*> scan_memory(T find) {
        return mem->scan_memory(find);
    }

    template<typename T>
    void bytes_to_T(const uint8_t* addr, T& val) {
        return mem->bytes_to_T(addr, val);
    }

    template<class T>
    std::vector<const uint8_t*> find_byte_from_vector(T find, const std::vector<const uint8_t*>& vector) {
        return mem->find_byte_from_vector(find, vector);
    }

    // void scan_inject(int find, bool new_run) { return mem->scan_inject(find, new_run); }

    CrossMemory(const std::string& exec_name);
    ~CrossMemory();
};

CrossMemory::CrossMemory(const std::string& exec_name) : m_exec_name(exec_name) {
    mem = new WindowsMem(exec_name);
}

CrossMemory::~CrossMemory() {
    if (mem) {
        delete mem;
    }
}
