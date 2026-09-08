#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main(void)
{
    const char *path = "win32-created.txt";
    HANDLE file;
    DWORD open_result;

    SetLastError(ERROR_SUCCESS);
    file = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    open_result = GetLastError();

    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFileA failed: %lu\n", open_result);
        return 1;
    }

    printf("path:   %s\n", path);
    printf("handle: %p\n", file);
    printf("result: %s\n",
           open_result == ERROR_ALREADY_EXISTS ? "opened existing file"
                                               : "created new file");

    CloseHandle(file);
    return 0;
}
