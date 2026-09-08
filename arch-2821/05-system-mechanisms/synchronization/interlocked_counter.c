#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define THREAD_COUNT 4
#define INCREMENTS_PER_THREAD 100000

static volatile LONG counter = 0;

static DWORD WINAPI increment_thread(LPVOID parameter)
{
    int index;
    (void)parameter;

    for (index = 0; index < INCREMENTS_PER_THREAD; ++index) {
        InterlockedIncrement(&counter);
    }

    return 0;
}

int main(void)
{
    HANDLE threads[THREAD_COUNT] = {0};
    DWORD result;
    int index;

    for (index = 0; index < THREAD_COUNT; ++index) {
        threads[index] = CreateThread(NULL, 0, increment_thread, NULL, 0, NULL);
        if (threads[index] == NULL) {
            fprintf(stderr, "CreateThread failed: %lu\n", GetLastError());
            while (--index >= 0) {
                WaitForSingleObject(threads[index], INFINITE);
                CloseHandle(threads[index]);
            }
            return 1;
        }
    }

    result = WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);
    for (index = 0; index < THREAD_COUNT; ++index) {
        CloseHandle(threads[index]);
    }

    if (result != WAIT_OBJECT_0) {
        fprintf(stderr, "WaitForMultipleObjects failed: %lu\n", GetLastError());
        return 1;
    }

    printf("expected: %d\n", THREAD_COUNT * INCREMENTS_PER_THREAD);
    printf("actual:   %ld\n", counter);
    return counter == THREAD_COUNT * INCREMENTS_PER_THREAD ? 0 : 1;
}
