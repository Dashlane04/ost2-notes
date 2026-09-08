#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

typedef VOID (NTAPI *RtlInitUnicodeStringFn)(
    PUNICODE_STRING destination,
    PCWSTR source
);

int main(void)
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    union {
        FARPROC raw;
        RtlInitUnicodeStringFn typed;
    } init_unicode_string;
    UNICODE_STRING value;
    WCHAR text[] = L"Architecture 2821";

    if (ntdll == NULL) {
        fprintf(stderr, "ntdll.dll is not loaded\n");
        return 1;
    }

    init_unicode_string.raw = GetProcAddress(
        ntdll,
        "RtlInitUnicodeString"
    );
    if (init_unicode_string.raw == NULL) {
        fprintf(stderr, "RtlInitUnicodeString was not found\n");
        return 1;
    }

    init_unicode_string.typed(&value, text);

    wprintf(L"Buffer:        %.*ls\n",
            (int)(value.Length / sizeof(WCHAR)),
            value.Buffer);
    wprintf(L"Length:        %hu bytes\n", value.Length);
    wprintf(L"MaximumLength: %hu bytes\n", value.MaximumLength);
    wprintf(L"Same buffer:   %ls\n", value.Buffer == text ? L"yes" : L"no");

    return 0;
}
