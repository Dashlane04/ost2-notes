#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef PVOID (NTAPI *RtlAllocateHeapFn)(
    PVOID heap_handle,
    ULONG flags,
    SIZE_T size
);

typedef BOOLEAN (NTAPI *RtlFreeHeapFn)(
    PVOID heap_handle,
    ULONG flags,
    PVOID base_address
);

int main(void)
{
    static const char message[] = "hello from the RTL heap";
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    HANDLE heap = GetProcessHeap();
    union {
        FARPROC raw;
        RtlAllocateHeapFn typed;
    } allocate_heap;
    union {
        FARPROC raw;
        RtlFreeHeapFn typed;
    } free_heap;
    char *buffer;

    if (ntdll == NULL || heap == NULL) {
        fprintf(stderr, "Could not get ntdll or the process heap\n");
        return 1;
    }

    allocate_heap.raw = GetProcAddress(ntdll, "RtlAllocateHeap");
    free_heap.raw = GetProcAddress(ntdll, "RtlFreeHeap");
    if (allocate_heap.raw == NULL || free_heap.raw == NULL) {
        fprintf(stderr, "RTL heap functions were not found\n");
        return 1;
    }

    buffer = (char *)allocate_heap.typed(heap, 0, sizeof(message));
    if (buffer == NULL) {
        fprintf(stderr, "RtlAllocateHeap failed\n");
        return 1;
    }

    memcpy(buffer, message, sizeof(message));
    printf("%s\n", buffer);

    if (!free_heap.typed(heap, 0, buffer)) {
        fprintf(stderr, "RtlFreeHeap failed\n");
        return 1;
    }

    return 0;
}
