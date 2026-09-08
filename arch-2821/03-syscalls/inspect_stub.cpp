#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>
#include <iomanip>
#include <iostream>

int main()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        std::cerr << "ntdll.dll is not loaded\n";
        return 1;
    }

    auto stub = reinterpret_cast<const unsigned char *>(
        GetProcAddress(ntdll, "NtClose")
    );
    if (stub == nullptr) {
        std::cerr << "NtClose was not found\n";
        return 1;
    }

    std::cout << "ntdll!NtClose: " << static_cast<const void *>(stub) << '\n';
    std::cout << "first bytes:   ";
    for (int i = 0; i < 24; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned int>(stub[i]) << ' ';
    }
    std::cout << '\n';

#if defined(_M_X64) || defined(__x86_64__)
    const bool common_x64_stub =
        stub[0] == 0x4c && stub[1] == 0x8b && stub[2] == 0xd1 && stub[3] == 0xb8;

    if (common_x64_stub) {
        unsigned int service_number = 0;
        std::memcpy(&service_number, stub + 4, sizeof(service_number));
        std::cout << "service number: 0x" << service_number << '\n';
    } else {
        std::cout << "The bytes do not match the common x64 stub layout.\n";
    }
#else
    std::cout << "Service number decoding is only included for x64.\n";
#endif

    return 0;
}
