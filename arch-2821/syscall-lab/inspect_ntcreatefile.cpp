#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>
#include <iomanip>
#include <iostream>

int main()
{
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

    if (kernel32 == nullptr || ntdll == nullptr) {
        std::cerr << "Could not find kernel32 or ntdll\n";
        return 1;
    }

    FARPROC create_file = GetProcAddress(kernel32, "CreateFileA");
    auto nt_create_file = reinterpret_cast<const unsigned char *>(
        GetProcAddress(ntdll, "NtCreateFile")
    );

    if (create_file == nullptr || nt_create_file == nullptr) {
        std::cerr << "Could not resolve CreateFileA or NtCreateFile\n";
        return 1;
    }

    std::cout << "CreateFileA:  "
              << reinterpret_cast<const void *>(create_file) << '\n';
    std::cout << "NtCreateFile: "
              << static_cast<const void *>(nt_create_file) << '\n';

    std::cout << "stub bytes:   ";
    for (int i = 0; i < 24; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned int>(nt_create_file[i]) << ' ';
    }
    std::cout << '\n';

#if defined(_M_X64) || defined(__x86_64__)
    const bool common_x64_stub =
        nt_create_file[0] == 0x4c &&
        nt_create_file[1] == 0x8b &&
        nt_create_file[2] == 0xd1 &&
        nt_create_file[3] == 0xb8;

    if (common_x64_stub) {
        unsigned int service_number = 0;
        std::memcpy(
            &service_number,
            nt_create_file + 4,
            sizeof(service_number)
        );
        std::cout << "service no.:  0x" << service_number << '\n';
    } else {
        std::cout << "The stub does not match the common x64 layout.\n";
    }
#else
    std::cout << "Service number decoding is only included for x64.\n";
#endif

    return 0;
}
