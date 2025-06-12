#include "pch.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <stdio.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>
#include <psapi.h>
#include <shlwapi.h>

DWORD WINAPI PatchThread(LPVOID) {
    std::this_thread::sleep_for(std::chrono::seconds(2)); // CHANGE THIS IF TIMING FAILS (Recommended 1-3 seconds)
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring patchFile = std::wstring(exePath).substr(0, std::wstring(exePath).find_last_of(L"\\/") + 1) + L"patch.1337";

    std::ifstream file(patchFile);
    if (file) {
        std::string line;
        std::getline(file, line);

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            size_t colon = line.find(':');
            size_t arrow = line.find("->");
            if (colon == std::string::npos || arrow == std::string::npos) continue;

            try {
                uint64_t rva = std::stoull(line.substr(0, colon), nullptr, 16);
                void* address = (void*)((uintptr_t)GetModuleHandle(NULL) + rva);
                BYTE original = static_cast<BYTE>(std::stoul(line.substr(colon + 1, arrow - colon - 1), nullptr, 16));
                BYTE replacement = static_cast<BYTE>(std::stoul(line.substr(arrow + 2), nullptr, 16));

                BYTE current;
                if (ReadProcessMemory(GetCurrentProcess(), address, &current, 1, NULL) && current == original) {
                    DWORD oldProtect;
                    if (VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                        WriteProcessMemory(GetCurrentProcess(), address, &replacement, 1, NULL);
                        VirtualProtect(address, 1, oldProtect, &oldProtect);
                    }
                }
            }
            catch (...) {}
        }
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
