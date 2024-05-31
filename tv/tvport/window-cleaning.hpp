#ifndef WINDOW_CLEANING_HPP
#define WINDOW_CLEANING_HPP
#include <windows.h>

class WindowCleaningUtils {
public:
	static void PrintError(TCHAR const* msg);
	static DWORD FindProcessItem(const wchar_t* name);
	static bool CleanProcessByTermination(DWORD ProcessID);
	static bool CleanProcessByName(const wchar_t* name);
};

#endif