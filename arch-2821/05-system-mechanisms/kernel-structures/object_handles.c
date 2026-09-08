#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main(void)
{
    HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE duplicate = NULL;
    DWORD wait_result;

    if (event == NULL) {
        fprintf(stderr, "CreateEventW failed: %lu\n", GetLastError());
        return 1;
    }

    if (!DuplicateHandle(
            GetCurrentProcess(),
            event,
            GetCurrentProcess(),
            &duplicate,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS)) {
        fprintf(stderr, "DuplicateHandle failed: %lu\n", GetLastError());
        CloseHandle(event);
        return 1;
    }

    printf("original handle:  %p\n", event);
    printf("duplicate handle: %p\n", duplicate);

    SetEvent(event);
    wait_result = WaitForSingleObject(duplicate, 1000);
    printf("wait through duplicate: %s\n",
           wait_result == WAIT_OBJECT_0 ? "signaled" : "failed");

    CloseHandle(duplicate);
    CloseHandle(event);
    return wait_result == WAIT_OBJECT_0 ? 0 : 1;
}
