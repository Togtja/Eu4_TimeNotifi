#pragma once

#ifdef _WIN32
#    include "WindowsMem.hpp"
#else
#endif

#include <iostream>
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
    std::unordered_map<uint8_t*, T> find_byte(T find) {
        std::cout << "looking for: " << find << "\n";
        return mem->find_byte(find);
    }

    template<class T>
    std::vector<const uint8_t*> find_value(T find) {
        std::cout << "looking for: " << find << "\n";
        return mem->find_value(find);
    }

    template<class T>
    std::vector<void*> find_value2(T find) {
        std::cout << "looking for: " << find << "\n";
        return mem->find_value2(find);
    }

    template<class T>
    std::vector<const uint8_t*> scan_memory(T find) {
        std::cout << "looking for: " << find << "\n";
        return mem->scan_memory(find);
    }

    template<typename T>
    void bytes_to_T(uint8_t addr, T& val) {
        return mem->bytes_to_T(addr, val);
    }

    template<typename T>
    void bytes_to_T2(const uint8_t* addr, T& val) {
        return mem->bytes_to_T2(addr, val);
    }

    template<class T>
    std::unordered_map<uint8_t, T> find_byte_from_map(T find, const std::unordered_map<uint8_t, T>& map) {
        return mem->find_byte_from_map(find, map);
    }

    template<class T>
    std::vector<const uint8_t*> find_byte_from_vector(T find, const std::vector<const uint8_t*>& vector) {
        return mem->find_byte_from_vector(find, vector);
    }
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
