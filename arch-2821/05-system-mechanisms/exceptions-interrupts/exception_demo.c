#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define LAB_EXCEPTION ((DWORD)0xE0422821)

static LONG CALLBACK exception_handler(PEXCEPTION_POINTERS pointers)
{
    const EXCEPTION_RECORD *record = pointers->ExceptionRecord;

    if (record->ExceptionCode != LAB_EXCEPTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    printf("handler thread: %lu\n", GetCurrentThreadId());
    printf("exception code: 0x%08lX\n", record->ExceptionCode);
    printf("exception at:   %p\n", record->ExceptionAddress);

    return EXCEPTION_CONTINUE_EXECUTION;
}

int main(void)
{
    PVOID handler = AddVectoredExceptionHandler(1, exception_handler);

    if (handler == NULL) {
        fprintf(stderr, "AddVectoredExceptionHandler failed\n");
        return 1;
    }

    printf("raising a private software exception\n");
    RaiseException(LAB_EXCEPTION, 0, 0, NULL);
    printf("execution continued after the handler\n");

    RemoveVectoredExceptionHandler(handler);
    return 0;
}
