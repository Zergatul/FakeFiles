#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>

typedef HANDLE (WINAPI *CreateFileW_Def) (
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile);
CreateFileW_Def OldCreateFileW;
BYTE* CreateFileWJumper = new BYTE[9];

typedef BOOL (WINAPI *DeleteFileW_Def) (
	LPCTSTR lpFileName);
DeleteFileW_Def OldDeleteFileW;
BYTE* DeleteFileWJumper = new BYTE[9];

BOOL StartWith(LPCTSTR str1, LPCTSTR str2)
{
	if (str1 == NULL || str2 == NULL)
		return FALSE;
	int str1len = lstrlen(str1);
	int str2len = lstrlen(str2);

	if (str1len < str2len)
		return FALSE;

	for (TCHAR *ch1 = (TCHAR*)str1, *ch2 = (TCHAR*)str2; *ch2 != NULL; ch1++, ch2++)
		if (*ch1 != *ch2)
			return FALSE;

	return TRUE;
}

void ToLower(LPCTSTR str)
{
	if (str == NULL)
		return;
	if (lstrlen(str) == 0)
		return;

	/*DWORD buf;
	VirtualProtect((LPVOID)str, lstrlen(str) * 2, PAGE_EXECUTE_READWRITE, &buf);*/

	for (TCHAR *ch = (TCHAR*)str; *ch != NULL; ch++)
	{
		int charConverted = (int)(*ch);
		int charLower = (int)CharLowerW((LPWSTR)charConverted);
		*ch = (TCHAR)charLower;
	}
}

void Log(LPCTSTR str);
void FlushLog();

HANDLE WINAPI MyCreateFileW(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	int len = lstrlen(lpFileName);
	if (len > 0)
	{
		LPWSTR copy = (LPWSTR)(new TCHAR[len + 1]);
		lstrcpy(copy, lpFileName);
		ToLower(copy);
		if (StartWith(copy, L"c:\\users\\zergatul\\appdata\\"))
		{
			/*Log(L"CreateFileW: ");
			Log(lpFileName);
			Log(L"\n");
			FlushLog();*/
			/*wchar_t* drv = new wchar_t[len + 1];
			wchar_t* dir = new wchar_t[len + 1];
			wchar_t* name = new wchar_t[len + 1];
			wchar_t* ext = new wchar_t[len + 1];
			_wsplitpath(copy, drv, dir, name, ext);
			LPWSTR folder = L"e:\\ProgramsTemp\\IHateSkype\\";
			DWORD newLen = lstrlenW(folder) + lstrlenW(name) + lstrlenW(ext) + 1;
			LPWSTR lpFileNameNew = (LPWSTR)(new TCHAR[newLen]);
			lpFileNameNew[0] = 0;
			lstrcatW(lpFileNameNew, folder);
			lstrcatW(lpFileNameNew, name);
			lstrcatW(lpFileNameNew, ext);
			HANDLE result = OldCreateFileW(lpFileNameNew, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			/*delete [] copy;
			delete [] drv;
			delete [] dir;
			delete [] name;
			delete [] ext;
			delete [] lpFileNameNew;*/
			//return result;
		}
		//delete [] copy;
	}

	return OldCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI MyDeleteFileW(LPCTSTR lpFileName)
{
	int len = lstrlen(lpFileName);
	if (len > 0)
	{
		LPWSTR copy = (LPWSTR)(new TCHAR[len + 1]);
		lstrcpy(copy, lpFileName);
		ToLower(copy);
		if (StartWith(copy, L"c:\\users\\zergatul\\appdata\\"))
		{
			Log(L"DeleteFileW: ");
			Log(lpFileName);
			Log(L"\n");
			FlushLog();
		}
	}
	return OldDeleteFileW(lpFileName);
}

void RedirectFunction(LPVOID oldFunction, LPVOID newFunction, BYTE* jumper, LPVOID pointerToOldFunction)
{
	DWORD oldProtect;
	VirtualProtect(jumper, 9, PAGE_EXECUTE_READWRITE, &oldProtect);

	VirtualProtect(oldFunction, 5, PAGE_EXECUTE_READWRITE, &oldProtect);

	BYTE *pE9 = (BYTE*)oldFunction;
	DWORD *pOffset = (DWORD*)(pE9 + 1);

	memcpy(&jumper[0], oldFunction, 5);

	*pE9 = 0xE9;
	*pOffset = (DWORD)newFunction - (DWORD)oldFunction - 5;

	jumper[5] = 0xE9;
	DWORD offset = (DWORD)oldFunction - (DWORD)jumper - 5;
	memcpy(&jumper[6], &offset, 4);

	DWORD *pointer = (DWORD*)pointerToOldFunction;
	*pointer = (DWORD)jumper;
}

void RedirectFunctions()
{
	HMODULE user32 = GetModuleHandle(L"kernel32.dll");
	RedirectFunction(GetProcAddress(user32, "CreateFileW"), MyCreateFileW, CreateFileWJumper, &OldCreateFileW);
	RedirectFunction(GetProcAddress(user32, "DeleteFileW"), MyDeleteFileW, DeleteFileWJumper, &OldDeleteFileW);
}

FILE* file;

void InitLog()
{
	file = fopen("E:\\temp\\fakefiles.txt", "w");
}

void Log(LPCTSTR str)
{
	DWORD len = lstrlen(str);
	for (int i = 0; i < len; i++)
		fputc(str[i], file);
}

void FlushLog()
{
	fflush(file);
}

bool APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			InitLog();
			RedirectFunctions();
		break;
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}