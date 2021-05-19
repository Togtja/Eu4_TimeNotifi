#pragma once
#include <string>
#include <vector>
class UnixMem {
private:
    std::string m_program;

public:
    UnixMem(const std::string& program);
    ~UnixMem();

    std::vector<uint8_t> read_address(const uint8_t* address, size_t size) { return {}; }

    std::vector<const uint8_t*> find_byte_from_vector(const std::vector<uint8_t>& byte_check,
                                                      const std::vector<const uint8_t*>& vector) {
        return {};
    }

    std::vector<const uint8_t*>
    scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check, const uint32_t nr_threads) {
        return {};
    }

    std::vector<const uint8_t*> scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check) {
        return {};
    }

    std::vector<const uint8_t*> scan_memory(const std::vector<uint8_t>& byte_check, const uint32_t nr_threads) { return {}; }
};

UnixMem::UnixMem(const std::string& program) : m_program(program) {}

UnixMem::~UnixMem() {}
