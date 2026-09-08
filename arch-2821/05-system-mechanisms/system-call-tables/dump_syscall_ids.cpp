#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>
#include <iomanip>
#include <iostream>

static bool read_x64_service_number(const unsigned char *stub, unsigned int& value)
{
#if defined(_M_X64) || defined(__x86_64__)
    if (stub[0] != 0x4c || stub[1] != 0x8b ||
        stub[2] != 0xd1 || stub[3] != 0xb8) {
        return false;
    }

    std::memcpy(&value, stub + 4, sizeof(value));
    return true;
#else
    (void)stub;
    (void)value;
    return false;
#endif
}

int main()
{
    const char *names[] = {
        "NtClose",
        "NtCreateFile",
        "NtOpenProcess",
        "NtQueryInformationProcess",
        "NtWaitForSingleObject"
    };

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        std::cerr << "ntdll.dll is not loaded\n";
        return 1;
    }

    std::cout << std::left << std::setw(28) << "function"
              << std::setw(20) << "address" << "service number\n";

    for (const char *name : names) {
        auto stub = reinterpret_cast<const unsigned char *>(
            GetProcAddress(ntdll, name)
        );

        std::cout << std::left << std::setw(28) << name;
        if (stub == nullptr) {
            std::cout << "not exported\n";
            continue;
        }

        std::cout << std::setw(20) << static_cast<const void *>(stub);

        unsigned int service_number = 0;
        if (read_x64_service_number(stub, service_number)) {
            std::cout << "0x" << std::hex << service_number << std::dec << '\n';
        } else {
            std::cout << "unknown stub layout\n";
        }
    }

    return 0;
}
