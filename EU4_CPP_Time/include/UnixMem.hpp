#pragma once
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/uio.h>

// External Libaries
#include <spdlog/spdlog.h>

class UnixMem {
private:
    static constexpr uint32_t BUFF_SIZE = 4096;

    std::string m_program;
    pid_t m_pid;

    // trim from start (in place)
    static inline void ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string& s) {
        ltrim(s);
        rtrim(s);
    }

    std::string exec(std::string_view cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
        if (!pipe) {
            return {};
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    pid_t getPid() {
        std::string cmd_str{"pidof -s " + m_program};
        auto pid_txt = exec(cmd_str);
        if (pid_txt.empty()) {
            spdlog::error("Fuck!");
            // TODO: throw runtime error
        }
        return std::stoul(pid_txt.c_str(), nullptr, 10);
    }

    struct MemSearch {
        uint8_t* address_low  = nullptr;
        uint8_t* address_high = nullptr;
    };

    std::vector<MemSearch> getMemoryLocations() {
        std::vector<MemSearch> out{};
        std::string cmd_str = fmt::format("cat /proc/{}/maps", m_pid);
        std::stringstream mem_map;
        std::string range, perms, offset, dev, inode, pathname;
        mem_map << exec(cmd_str);
        std::string line;
        while (std::getline(mem_map, line)) {
            std::istringstream iss(line);
            int a, b;
            if (!(iss >> range >> perms >> offset >> dev >> inode)) {
                spdlog::error("failed to parse memory map {}", line);
                break;
            } // error
            std::getline(iss, pathname);
            trim(pathname);
            spdlog::debug("{} {} {} {} {} {}", range, perms, offset, dev, inode, pathname);
            auto low       = range.substr(0, range.find("-"));
            auto high      = range.substr(range.find("-") + 1);
            auto low_void  = (uint8_t*)std::stoull(low, nullptr, 16);
            auto high_void = (uint8_t*)std::stoull(high, nullptr, 16);
            out.emplace_back(MemSearch{low_void, high_void});
        }
        return out;
    }

public:
    UnixMem(const std::string& program);
    ~UnixMem();

    std::vector<uint8_t> read_address(const uint8_t* address, size_t size) {
        std::vector<uint8_t> mem_buff(size);
        // Build iovec structs
        struct iovec local[1];
        local[0].iov_base = mem_buff.data();
        local[0].iov_len  = mem_buff.size();

        struct iovec remote[1];
        remote[0].iov_base = (void*)address;
        remote[0].iov_len  = mem_buff.size();
        ssize_t nread      = process_vm_readv(m_pid, local, 1, remote, 1, 0);
        if (nread < 0) {
            // spdlog::debug("mem at {}", fmt::ptr(remote[0].iov_base));
            switch (errno) {
                case EINVAL: spdlog::debug("ERROR: INVALID ARGUMENTS"); break;
                // case EFAULT: std::cout << "ERROR: UNABLE TO ACCESS TARGET MEMORY ADDRESS.\n"; break;
                case ENOMEM: spdlog::debug("ERROR: UNABLE TO ALLOCATE MEMORY"); break;
                case EPERM: spdlog::debug("ERROR: INSUFFICIENT PRIVILEGES TO TARGET PROCESS"); break;
                case ESRCH:
                    spdlog::debug("ERROR: PROCESS DOES NOT EXIST");
                    break;
                    // default: spdlog::debug("ERROR: AN UNKNOWN ERROR HAS OCCURRED");
            }
            return {};
        }
        else {
            return mem_buff;
        }
    }

    std::vector<const uint8_t*> find_byte_from_vector(const std::vector<uint8_t>& byte_check,
                                                      const std::vector<const uint8_t*>& vector) {
        std::vector<const uint8_t*> out;
        for (auto&& i : vector) {
            if (read_address(i, byte_check.size()) == byte_check) {
                out.emplace_back(i);
            }
        }
        return out;
    }

    std::vector<const uint8_t*>
    scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check, const uint32_t nr_threads) {
        return {};
    }

    std::vector<const uint8_t*> scan_memory(uint8_t* address_low, uint8_t* address_high, const std::vector<uint8_t>& byte_check) {
        std::vector<const uint8_t*> addresses_found{};
        while (address_low < address_high) {
            auto mem_buff = read_address(address_low, BUFF_SIZE);

            if (!mem_buff.empty()) {
                auto start = mem_buff.data(), end = start + mem_buff.size(), pos = start;
                while ((pos = std::search(pos, end, byte_check.begin(), byte_check.end())) != end) {
                    auto mem_loc = address_low + (pos - start);
                    addresses_found.push_back(static_cast<const uint8_t*>(mem_loc));
                    pos += byte_check.size();
                }
            }

            address_low += BUFF_SIZE;
        }
        return addresses_found;
    }

    std::vector<const uint8_t*> scan_memory(const std::vector<uint8_t>& byte_check, const uint32_t nr_threads) {
        std::vector<const uint8_t*> out;
        auto searchList = getMemoryLocations();
        for (auto&& [address_low, address_high] : searchList) {
            auto sub_out = scan_memory(address_low, address_high, byte_check);
            out.insert(out.end(), sub_out.begin(), sub_out.end());
        }

        return out;
    }
};

UnixMem::UnixMem(const std::string& program) : m_program(program), m_pid(getPid()) {}

UnixMem::~UnixMem() {}
