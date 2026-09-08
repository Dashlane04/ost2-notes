#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <iomanip>
#include <iostream>

int wmain()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << '\n';
        return 1;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Process32FirstW(snapshot, &entry)) {
        std::cerr << "Process32FirstW failed: " << GetLastError() << '\n';
        CloseHandle(snapshot);
        return 1;
    }

    std::wcout << std::left << std::setw(8) << L"PID"
               << std::setw(8) << L"PPID" << L"Image\n";

    do {
        std::wcout << std::left << std::setw(8) << entry.th32ProcessID
                   << std::setw(8) << entry.th32ParentProcessID
                   << entry.szExeFile << L'\n';
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return 0;
}
