#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main(void)
{
    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY policy = {0};

    if (!GetProcessMitigationPolicy(
            GetCurrentProcess(),
            ProcessSystemCallDisablePolicy,
            &policy,
            sizeof(policy))) {
        fprintf(stderr, "GetProcessMitigationPolicy failed: %lu\n", GetLastError());
        return 1;
    }

    printf("Win32k system calls: %s\n",
           policy.DisallowWin32kSystemCalls ? "disabled" : "allowed");

    return 0;
}
