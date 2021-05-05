#include "DllInjector.hpp"

DllInjector::DllInjector(const std::string& dllPath, const std::string& program) : m_dllPath(dllPath) {
    if (!std::filesystem::exists(dllPath)) {
        throw std::runtime_error(std::string("failed to dll " + dllPath));
    }
    PROCESSENTRY32
    entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (_stricmp(entry.szExeFile, program.c_str()) == 0) {
                m_proc_id = entry.th32ProcessID;
                m_proc    = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                CloseHandle(snapshot);
                return;
            }
        }
    }

    CloseHandle(snapshot);
    m_proc = NULL;
    throw std::runtime_error(std::string("failed to find program: " + program));
}

void DllInjector::inject(int find, bool new_run) {
    Arguments args;
    strncpy(args.dll_path, m_dllPath.c_str(), MAX_PATH);
    args.eu4     = find;
    args.new_run = new_run;

    // std::vector<uint8_t> bytes{};
    // if constexpr (std::is_same_v<T, std::string>) {
    //    bytes = std::vector<uint8_t>(find.begin(), find.end());
    //}
    // else {
    //    const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(&find));
    //    bytes               = std::vector<uint8_t>(data, data + sizeof(find));
    //}

    // using VecType        = typename std::decay<decltype(*args.find_bytes.begin())>::type;
    // size_t vector_sizeof = args.find_bytes.size() * sizeof(VecType);
    size_t dll_size    = MAX_PATH + sizeof(args); // + vector_sizeof;
    LPVOID dll_memAddr = VirtualAllocEx(m_proc, 0, dll_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    WriteProcessMemory(m_proc, dll_memAddr, &args, sizeof(args), 0);

    auto routine     = reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA);
    HANDLE ex_thread = CreateRemoteThread(m_proc, 0, 0, routine, dll_memAddr, 0, 0);

    WaitForSingleObject(ex_thread, INFINITE);

    auto get_dll = LoadLibrary("injection.dll");
    if (get_dll == NULL) {
        std::cout << "Failed find the start_scan function error" << GetLastError() << "\n";
        // Free the memory allocated
        VirtualFreeEx(m_proc, dll_memAddr, dll_size, MEM_RELEASE);

        if (ex_thread) {
            CloseHandle(ex_thread);
        }
        return;
    }
    auto dll_func = GetProcAddress(get_dll, "start_scan");

    if (!dll_func) {
        std::cout << "Failed to find dll func\n";
        VirtualFreeEx(m_proc, dll_memAddr, dll_size, MEM_RELEASE);

        if (ex_thread) {
            CloseHandle(ex_thread);
        }
        return;
    }
    while (FreeLibrary(get_dll)) {};
    Sleep(500);
    ex_thread = CreateRemoteThread(m_proc, 0, 0, (LPTHREAD_START_ROUTINE)dll_func, dll_memAddr, 0, 0);

    WaitForSingleObject(ex_thread, INFINITE);

    std::stringstream str;
    str << std::hex << dll_memAddr;
    spdlog::log(spdlog::level::info, "Injected at {}", str.str());

    // Free the memory allocated
    VirtualFreeEx(m_proc, dll_memAddr, dll_size, MEM_RELEASE);

    if (ex_thread) {
        CloseHandle(ex_thread);
    }
}