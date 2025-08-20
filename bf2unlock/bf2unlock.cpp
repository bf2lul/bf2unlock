#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>

DWORD GetProcessId(const wchar_t* processName) {
    PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    DWORD pid = 0;
    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}


void PatchUnlocks(HANDLE hProc) {
    BYTE nop[2] = { 0x90, 0x90 };

    WriteProcessMemory(hProc, (LPVOID)(0x468CBE), nop, 2, nullptr);
    WriteProcessMemory(hProc, (LPVOID)(0x5028C5), nop, 2, nullptr);
}

void PatchVolume(HANDLE hProc) {
    DWORD ptr1 = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)0x99EF80, &ptr1, 4, nullptr) || ptr1 == 0)
        return;

    DWORD ptr2 = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)(ptr1 + 440), &ptr2, 4, nullptr) || ptr2 == 0)
        return;

    float zero = 0.0f;
    WriteProcessMemory(hProc, (LPVOID)(ptr2 + 84), &zero, 4, nullptr);
    WriteProcessMemory(hProc, (LPVOID)(ptr2 + 88), &zero, 4, nullptr);
}

void PatchSatelliteString(HANDLE hProc) {
    const DWORD sat_addr = 0x00930C84;
    BYTE zero = 0x00;
    WriteProcessMemory(hProc, (LPVOID)sat_addr, &zero, 1, nullptr);
}

int main() {
    std::wcout << L"BF2 memory patcher running...\n";

    while (true) {
        DWORD pid = GetProcessId(L"BF2.exe");
        if (pid) {
            HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
            if (hProc) {
                PatchUnlocks(hProc);
                PatchVolume(hProc);
                PatchSatelliteString(hProc);
                //std::wcout << L"Patches applied.\n";
            }
        }
        else {
            std::wcout << L"BF2.exe not running.\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }

    return 0;
}
