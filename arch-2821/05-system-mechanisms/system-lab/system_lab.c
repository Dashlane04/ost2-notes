#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <string.h>

static HANDLE g_release_event;
static HANDLE g_worker_ready_event;

static void print_last_error(const char *operation)
{
    fprintf(stderr, "%s failed: %lu\n", operation, GetLastError());
}

static void wait_for_enter(const char *message, int automatic)
{
    int ch;

    puts(message);
    if (automatic) {
        return;
    }

    fputs("Press ENTER to continue... ", stdout);
    fflush(stdout);
    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);
}

static void print_ntcreatefile_stub(void)
{
    union {
        FARPROC procedure;
        const unsigned char *bytes;
    } address;
    HMODULE ntdll;
    const unsigned char *stub;
    unsigned int service_number;
    unsigned int i;

    ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == NULL) {
        print_last_error("GetModuleHandleW");
        return;
    }

    address.procedure = GetProcAddress(ntdll, "NtCreateFile");
    if (address.procedure == NULL) {
        print_last_error("GetProcAddress(NtCreateFile)");
        return;
    }

    stub = address.bytes;
    printf("ntdll!NtCreateFile: %p\n", (const void *)stub);
    fputs("first 16 bytes:      ", stdout);
    for (i = 0; i < 16; ++i) {
        printf("%02x ", stub[i]);
    }
    putchar('\n');

#if defined(_M_X64) || defined(__x86_64__)
    if (stub[0] == 0x4c && stub[1] == 0x8b &&
        stub[2] == 0xd1 && stub[3] == 0xb8) {
        memcpy(&service_number, stub + 4, sizeof(service_number));
        printf("usual x64 service number: 0x%x\n", service_number);
    } else {
        puts("stub does not have the usual unmodified x64 layout");
    }
#else
    puts("service-number parsing in this sample is x64 only");
#endif
}

static DWORD WINAPI worker_main(LPVOID parameter)
{
    DWORD wait_result;

    (void)parameter;
    printf("worker TID: %lu (0x%lx)\n", GetCurrentThreadId(),
           GetCurrentThreadId());

    if (!SetEvent(g_worker_ready_event)) {
        print_last_error("SetEvent(worker ready)");
        return 1;
    }

    puts("worker: entering WaitForSingleObject(release event)");
    wait_result = WaitForSingleObject(g_release_event, INFINITE);
    printf("worker: wait returned 0x%lx\n", wait_result);

    return wait_result == WAIT_OBJECT_0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    const int automatic = argc == 2 && strcmp(argv[1], "--auto") == 0;
    static const char data[] = "Created by the Architecture 2821 system lab.\r\n";
    HANDLE worker;
    HANDLE file;
    DWORD bytes_written;
    DWORD worker_exit_code;

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("PID:      %lu (0x%lx)\n", GetCurrentProcessId(),
           GetCurrentProcessId());
    printf("main TID: %lu (0x%lx)\n", GetCurrentThreadId(),
           GetCurrentThreadId());
    printf("build:    %s\n", sizeof(void *) == 8 ? "x64" : "x86");

    g_release_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_worker_ready_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_release_event == NULL || g_worker_ready_event == NULL) {
        print_last_error("CreateEventW");
        if (g_release_event != NULL) {
            CloseHandle(g_release_event);
        }
        if (g_worker_ready_event != NULL) {
            CloseHandle(g_worker_ready_event);
        }
        return 1;
    }

    printf("release event handle: %p\n", g_release_event);
    worker = CreateThread(NULL, 0, worker_main, NULL, 0, NULL);
    if (worker == NULL) {
        print_last_error("CreateThread");
        CloseHandle(g_worker_ready_event);
        CloseHandle(g_release_event);
        return 1;
    }

    if (WaitForSingleObject(g_worker_ready_event, 5000) != WAIT_OBJECT_0) {
        fputs("worker did not become ready\n", stderr);
        SetEvent(g_release_event);
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
        CloseHandle(g_worker_ready_event);
        CloseHandle(g_release_event);
        return 1;
    }

    Sleep(100);
    print_ntcreatefile_stub();

    wait_for_enter(
        "\nStage 1: the worker is waiting. The next action calls CreateFileW.",
        automatic
    );

    file = CreateFileW(
        L"system-lab-output.txt",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) {
        print_last_error("CreateFileW");
        SetEvent(g_release_event);
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
        CloseHandle(g_worker_ready_event);
        CloseHandle(g_release_event);
        return 1;
    }

    if (!WriteFile(file, data, (DWORD)(sizeof(data) - 1),
                   &bytes_written, NULL)) {
        print_last_error("WriteFile");
        CloseHandle(file);
        SetEvent(g_release_event);
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
        CloseHandle(g_worker_ready_event);
        CloseHandle(g_release_event);
        return 1;
    }

    printf("CreateFileW returned handle %p; wrote %lu bytes\n",
           file, bytes_written);
    CloseHandle(file);

    wait_for_enter(
        "\nStage 2: file I/O is done. The worker is still waiting.",
        automatic
    );

    if (!SetEvent(g_release_event)) {
        print_last_error("SetEvent(release)");
    }

    WaitForSingleObject(worker, INFINITE);
    worker_exit_code = 1;
    GetExitCodeThread(worker, &worker_exit_code);

    CloseHandle(worker);
    CloseHandle(g_worker_ready_event);
    CloseHandle(g_release_event);

    printf("worker exit code: %lu\n", worker_exit_code);
    return worker_exit_code == 0 ? 0 : 1;
}
