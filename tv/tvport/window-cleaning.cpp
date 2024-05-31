#include "window-cleaning.hpp"
#include <atlstr.h>
#include <tlhelp32.h>
#include <tchar.h>

DWORD WindowCleaningUtils::FindProcessItem(const wchar_t* name)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        PrintError(TEXT("CreateToolhelp32Snapshot (of processes)"));
        return 0;
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        PrintError(TEXT("Process32First")); // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return 0;
    }

    // Now walk the snapshot of processes, 
    do
    {
        if (_wcsicmp(name, pe32.szExeFile) == 0) {
            CloseHandle(hProcessSnap);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return 0;
}

void WindowCleaningUtils::PrintError(TCHAR const* msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    // Display the message
    _tprintf(TEXT("  WARNING: %s failed with error %d (%s)\n"), msg, eNum, sysMsg);
}

bool WindowCleaningUtils::CleanProcessByTermination(DWORD ProcessID) {
    HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessID);
    if (NULL != handle) {
        BOOL res = TerminateProcess(handle, 0);
        CloseHandle(handle);
        if (!res) {
            PrintError(TEXT("process opened but termination failed"));
            return false;
        }
        return res ? true : false;
    }
    else {
        PrintError(TEXT("cannot open with process terminate rights"));
    }
    return false;
}

bool WindowCleaningUtils::CleanProcessByName(const wchar_t* name) {
    DWORD pid = FindProcessItem(name);
    if (pid == 0) {
        return true;
    }
    bool res = CleanProcessByTermination(pid);
    return res;
}
