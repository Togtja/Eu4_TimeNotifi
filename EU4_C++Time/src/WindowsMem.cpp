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