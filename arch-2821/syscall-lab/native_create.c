#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <wchar.h>

#ifndef FILE_OPEN_IF
#define FILE_OPEN_IF 0x00000003
#endif

#ifndef FILE_SYNCHRONOUS_IO_NONALERT
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#endif

#ifndef FILE_NON_DIRECTORY_FILE
#define FILE_NON_DIRECTORY_FILE 0x00000040
#endif

typedef VOID (NTAPI *RtlInitUnicodeStringFn)(
    PUNICODE_STRING destination,
    PCWSTR source
);

typedef NTSTATUS (NTAPI *NtCreateFileFn)(
    PHANDLE file_handle,
    ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes,
    PIO_STATUS_BLOCK io_status_block,
    PLARGE_INTEGER allocation_size,
    ULONG file_attributes,
    ULONG share_access,
    ULONG create_disposition,
    ULONG create_options,
    PVOID ea_buffer,
    ULONG ea_length
);

int wmain(void)
{
    WCHAR dos_path[MAX_PATH];
    WCHAR nt_path[MAX_PATH + 4];
    UNICODE_STRING object_name;
    OBJECT_ATTRIBUTES attributes = {0};
    IO_STATUS_BLOCK io_status = {0};
    HANDLE file = NULL;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    NTSTATUS status;
    DWORD path_length;
    int written;

    union {
        FARPROC raw;
        RtlInitUnicodeStringFn typed;
    } init_unicode_string;

    union {
        FARPROC raw;
        NtCreateFileFn typed;
    } nt_create_file;

    if (ntdll == NULL) {
        fwprintf(stderr, L"ntdll.dll is not loaded\n");
        return 1;
    }

    init_unicode_string.raw = GetProcAddress(ntdll, "RtlInitUnicodeString");
    nt_create_file.raw = GetProcAddress(ntdll, "NtCreateFile");
    if (init_unicode_string.raw == NULL || nt_create_file.raw == NULL) {
        fwprintf(stderr, L"Could not resolve the Native API functions\n");
        return 1;
    }

    path_length = GetFullPathNameW(
        L"native-created.txt",
        MAX_PATH,
        dos_path,
        NULL
    );
    if (path_length == 0 || path_length >= MAX_PATH) {
        fwprintf(stderr, L"GetFullPathNameW failed: %lu\n", GetLastError());
        return 1;
    }

    written = swprintf(
        nt_path,
        sizeof(nt_path) / sizeof(nt_path[0]),
        L"\\??\\%ls",
        dos_path
    );
    if (written < 0 || (size_t)written >= sizeof(nt_path) / sizeof(nt_path[0])) {
        fwprintf(stderr, L"The NT path is too long\n");
        return 1;
    }

    init_unicode_string.typed(&object_name, nt_path);

    attributes.Length = sizeof(attributes);
    attributes.ObjectName = &object_name;
    attributes.Attributes = OBJ_CASE_INSENSITIVE;

    status = nt_create_file.typed(
        &file,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &attributes,
        &io_status,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );

    if (status < 0) {
        fwprintf(stderr, L"NtCreateFile failed: 0x%08lX\n", (ULONG)status);
        return 1;
    }

    wprintf(L"NT path:     %ls\n", nt_path);
    wprintf(L"handle:      %p\n", file);
    wprintf(L"NTSTATUS:    0x%08lX\n", (ULONG)status);
    wprintf(L"information: %llu\n", (unsigned long long)io_status.Information);

    CloseHandle(file);
    return 0;
}
