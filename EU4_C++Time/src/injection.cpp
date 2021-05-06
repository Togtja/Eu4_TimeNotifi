#include <Windows.h>

#include <psapi.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> read_address(const uint8_t* address, size_t size) {
    std::vector<uint8_t> mem_chunk;
    mem_chunk.resize(size);
    SIZE_T read;
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
        int cool;
        bytes_to_int((const uint8_t*)address, cool);
        std::cout << "Address: " << std::hex << address << " has value: " << std::dec << cool << "\n";
    }

    return addresses;
    /**

     */
}

std::vector<const void*> scan_memory(void* address_low, void* address_max, const std::vector<uint8_t>& bytes_to_find) {
    std::vector<const void*> addresses_found;

    // all readable pages: adjust this as required
    const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

    MEMORY_BASIC_INFORMATION mbi{};

    uint8_t* address = static_cast<uint8_t*>(address_low);

    while (address < address_max && VirtualQuery(address, std::addressof(mbi), sizeof(mbi))) {
        if ((mbi.State == MEM_COMMIT) && (mbi.Protect & pmask) && !(mbi.Protect & PAGE_GUARD)) {
            const uint8_t* begin = static_cast<const BYTE*>(mbi.BaseAddress);
            const uint8_t* end   = begin + mbi.RegionSize;

            const uint8_t* found = std::search(begin, end, bytes_to_find.begin(), bytes_to_find.end());
            while (found != end) {
                addresses_found.push_back(found);
                found = std::search(found + 1, end, bytes_to_find.begin(), bytes_to_find.end());
            }
        }

        address += mbi.RegionSize;
        mbi = {};
    }

    return addresses_found;
}

std::vector<const void*> scan_memory2(void* address_low, const std::vector<uint8_t>& bytes_to_find) {
    std::vector<const void*> addresses_found;

    // all readable pages: adjust this as required
    const DWORD pmask = PAGE_READONLY | PAGE_READWRITE;

    MEMORY_BASIC_INFORMATION mbi{};

    uint8_t* address = static_cast<uint8_t*>(address_low);

    while (VirtualQuery(address, std::addressof(mbi), sizeof(mbi))) {
        if ((mbi.State == MEM_COMMIT) && (mbi.Protect & pmask) && !(mbi.Protect & PAGE_GUARD)) {
            std::vector<uint8_t> mem_chunk;
            mem_chunk.reserve(mbi.RegionSize);
            SIZE_T read{};
            if (ReadProcessMemory(GetCurrentProcess(), mbi.BaseAddress, mem_chunk.data(), mbi.RegionSize, &read)) {
                auto start = mem_chunk.data(), end = start + read, pos = start;
                if (address > mbi.BaseAddress) {
                    pos += (address - static_cast<uint8_t*>(mbi.BaseAddress));
                }
                while ((pos = std::search(pos, end, bytes_to_find.begin(), bytes_to_find.end())) != end) {
                    addresses_found.emplace_back(static_cast<uint8_t*>(mbi.BaseAddress) + (pos - start));
                    pos += bytes_to_find.size();
                }
            }
        }

        address += mbi.RegionSize;
        mbi = {};
    }

    return addresses_found;
}

std::vector<const void*> scan_memory(const std::vector<uint8_t>& bytes_to_find) {
    DWORD_PTR baseAddress = 0;
    DWORD_PTR maxAddress  = 0;
    DWORD bytesRequired;
    HANDLE processHandle = GetCurrentProcess();
    if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired)) {
        if (bytesRequired) {
            LPBYTE moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

            if (moduleArrayBytes) {
                unsigned int moduleCount;

                moduleCount          = bytesRequired / sizeof(HMODULE);
                HMODULE* moduleArray = (HMODULE*)moduleArrayBytes;

                if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired)) {
                    MODULEINFO minfo{};
                    GetModuleInformation(processHandle, moduleArray[moduleCount - 1], &minfo, sizeof(minfo));
                    baseAddress = (DWORD_PTR)moduleArray[0];
                    maxAddress  = (DWORD_PTR)moduleArray[moduleCount - 1] + minfo.SizeOfImage;
                    for (int i = 0; i < moduleCount; i++) {
                        GetModuleInformation(processHandle, moduleArray[i], &minfo, sizeof(minfo));
                        std::cout << "Module " << i << "Address: " << std::hex << (void*)moduleArray[i] << "\n" << std::dec;
                    }
                }

                LocalFree(moduleArrayBytes);
            }
        }
    }

    std::cout << "MINAddress: " << std::hex << (void*)baseAddress << "\n";

    std::cout << "MAXAddress: " << std::hex << (void*)maxAddress << "\n" << std::dec;

    auto start_timer = std::chrono::steady_clock::now();
    auto out         = scan_memory((void*)baseAddress, (void*)maxAddress, bytes_to_find);
    auto end         = std::chrono::steady_clock::now();
    std::cout << "Spendt (scan_memory) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
              << "ms to find value\n";

    start_timer = std::chrono::steady_clock::now();
    auto out2   = scan_memory2((void*)baseAddress, bytes_to_find);
    end         = std::chrono::steady_clock::now();
    std::cout << "Spendt (scan_memory2) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
              << "ms to find value\n";

    start_timer = std::chrono::steady_clock::now();
    auto out3   = scan_memory(nullptr, (void*)baseAddress, bytes_to_find);
    end         = std::chrono::steady_clock::now();
    std::cout << "Spendt (scan_memory null) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
              << "ms to find value\n";

    start_timer = std::chrono::steady_clock::now();
    auto out4   = scan_memory2(nullptr, bytes_to_find);
    end         = std::chrono::steady_clock::now();
    std::cout << "Spendt (scan_memory2 null) " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_timer).count()
              << "ms to find value\n";

    std::cout << "scan_memory found: " << out.size() << "\n";
    std::cout << "scan_memory2 found: " << out2.size() << "\n";
    std::cout << "scan_memory null found: " << out3.size() << "\n";
    std::cout << "scan_memory2 null found: " << out4.size() << "\n";

    return out3;
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
    std::cout << "RUNNING FIND BYTES\n";
    if (args && args->new_run) {
        std::cout << "A WHOLE NEW RUN\n";
        int eu4             = args->eu4_date;
        const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&eu4));
        auto bytes          = std::vector<uint8_t>(data, data + sizeof(eu4));

        address = scan_memory(bytes);
        std::cout << "found entries " << address.size() << "\n";
        for (auto&& addr : address) {
            std::cout << "found on memory address: " << std::hex << addr << "\n" << std::dec;
        }
    }
    else if (args) {
        std::cout << "LOOKING IN ADDRESS LIST\n";
        int eu4             = args->eu4_date;
        const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&eu4));
        auto bytes          = std::vector<uint8_t>(data, data + sizeof(eu4));
        address             = scan_memory(address, bytes);

        std::cout << "found entries " << address.size() << "\n";
        for (auto&& addr : address) {
            std::cout << "found on memory address: " << std::hex << addr << "\n" << std::dec;
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