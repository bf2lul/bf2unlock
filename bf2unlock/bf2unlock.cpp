#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>

DWORD GetProcessId(const wchar_t* processName) {
    PROCESSENTRY32W procEntry = { sizeof(procEntry) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    DWORD processId = 0;
    if (Process32FirstW(snapshot, &procEntry)) {
        do {
            if (_wcsicmp(procEntry.szExeFile, processName) == 0) {
                processId = procEntry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &procEntry));
    }

    CloseHandle(snapshot);
    return processId;
}

void PatchUnlocks(HANDLE hProc) {
    const BYTE nop[2] = { 0x90, 0x90 };

    const uintptr_t addr1 = 0x468CBE;
    const uintptr_t addr2 = 0x5028C5;

    WriteProcessMemory(hProc, (LPVOID)addr1, nop, sizeof(nop), nullptr);
    WriteProcessMemory(hProc, (LPVOID)addr2, nop, sizeof(nop), nullptr);
}

void PatchVolume(HANDLE hProc) {
    uintptr_t base = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)0x99EF80, &base, sizeof(base), nullptr) || base == 0)
        return;

    uintptr_t ptr = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)(base + 440), &ptr, sizeof(ptr), nullptr) || ptr == 0)
        return;

    const float zero = 0.0f;
    const uintptr_t addr1 = ptr + 84;
    const uintptr_t addr2 = ptr + 88;

    WriteProcessMemory(hProc, (LPVOID)addr1, &zero, sizeof(zero), nullptr);
    WriteProcessMemory(hProc, (LPVOID)addr2, &zero, sizeof(zero), nullptr);
}

void PatchSatelliteString(HANDLE hProc) {
    const uintptr_t addr = 0x930C84;
    const BYTE zero = 0x00;

    WriteProcessMemory(hProc, (LPVOID)addr, &zero, sizeof(zero), nullptr);
}

int main() {
    std::wcout << L"BF2 memory patcher running...\n";

    while (true) {
        DWORD processId = GetProcessId(L"BF2.exe");
        if (processId) {
            HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
            if (hProc) {
                PatchUnlocks(hProc);
                PatchVolume(hProc);
                PatchSatelliteString(hProc);
                CloseHandle(hProc);
            }
        }
        else {
            std::wcout << L"BF2.exe not running.\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}
