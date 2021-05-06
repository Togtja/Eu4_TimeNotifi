// Reference https://www.youtube.com/watch?v=IBwoVUR1gt8 by Zer0Mem0ry
#pragma once
// OS specific
#include <Windows.h>

#include <TlHelp32.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

// WARNING: This struct must be idetical to the one in the DLL (injection.cpp)
struct Arguments {
    char dll_path[MAX_PATH];

    bool new_run = false; // If we should create new vector of found bytes
    int eu4      = 10;
};

class DllInjector {
private:
    HANDLE m_proc;
    DWORD m_proc_id;
    std::string m_dllPath;

public:
    DllInjector(const std::string& dllPath, const std::string& program);

    ~DllInjector(){};

    void inject(int find, bool new_run);
};
