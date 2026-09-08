#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static VOID CALLBACK apc_callback(ULONG_PTR value)
{
    printf("APC ran on thread %lu with value %llu\n",
           GetCurrentThreadId(),
           (unsigned long long)value);
}

int main(void)
{
    HANDLE current_thread = NULL;
    DWORD sleep_result;

    if (!DuplicateHandle(
            GetCurrentProcess(),
            GetCurrentThread(),
            GetCurrentProcess(),
            &current_thread,
            THREAD_SET_CONTEXT,
            FALSE,
            0)) {
        fprintf(stderr, "DuplicateHandle failed: %lu\n", GetLastError());
        return 1;
    }

    if (QueueUserAPC(apc_callback, current_thread, 2821) == 0) {
        fprintf(stderr, "QueueUserAPC failed: %lu\n", GetLastError());
        CloseHandle(current_thread);
        return 1;
    }

    printf("entering an alertable wait on thread %lu\n", GetCurrentThreadId());
    sleep_result = SleepEx(1000, TRUE);
    printf("SleepEx returned 0x%08lX\n", sleep_result);

    CloseHandle(current_thread);
    return sleep_result == WAIT_IO_COMPLETION ? 0 : 1;
}
