#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

typedef struct _WORKER_CONTEXT {
    HANDLE event;
    HANDLE semaphore;
} WORKER_CONTEXT;

static DWORD WINAPI worker_thread(LPVOID parameter)
{
    WORKER_CONTEXT *context = (WORKER_CONTEXT *)parameter;

    Sleep(200);
    printf("worker: signaling event\n");
    SetEvent(context->event);

    Sleep(200);
    printf("worker: releasing semaphore\n");
    ReleaseSemaphore(context->semaphore, 1, NULL);

    return 0;
}

int main(void)
{
    HANDLE objects[2];
    HANDLE worker;
    WORKER_CONTEXT context;
    DWORD result;

    objects[0] = CreateEventW(NULL, TRUE, FALSE, NULL);
    objects[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    if (objects[0] == NULL || objects[1] == NULL) {
        fprintf(stderr, "Could not create the wait objects\n");
        if (objects[0] != NULL) CloseHandle(objects[0]);
        if (objects[1] != NULL) CloseHandle(objects[1]);
        return 1;
    }

    context.event = objects[0];
    context.semaphore = objects[1];
    worker = CreateThread(NULL, 0, worker_thread, &context, 0, NULL);
    if (worker == NULL) {
        fprintf(stderr, "CreateThread failed: %lu\n", GetLastError());
        CloseHandle(objects[1]);
        CloseHandle(objects[0]);
        return 1;
    }

    printf("main: waiting for both objects\n");
    result = WaitForMultipleObjects(2, objects, TRUE, 5000);
    printf("main: %s\n",
           result == WAIT_OBJECT_0 ? "both objects are signaled" : "wait failed");

    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);
    CloseHandle(objects[1]);
    CloseHandle(objects[0]);

    return result == WAIT_OBJECT_0 ? 0 : 1;
}
