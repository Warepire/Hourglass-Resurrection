/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../tramps/alltramps.h"

HOOKFUNC HWND WINAPI MyCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
	LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND hwnd = CreateWindowExA(dwExStyle, lpClassName,
		lpWindowName, dwStyle, X, Y, nWidth, nHeight,
		hWndParent, hMenu, hInstance, lpParam);

    static const char* hello = __FUNCTION__ ": Hello from the hook!\n";
    OutputDebugStringA(hello);
	return hwnd;
}
HOOKFUNC HWND WINAPI MyCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
	LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND hwnd = CreateWindowExW(dwExStyle, lpClassName,
		lpWindowName, dwStyle, X, Y, nWidth, nHeight,
		hWndParent, hMenu, hInstance, lpParam);

    static const char* hello = __FUNCTION__ ": Hello from the hook!\n";
    OutputDebugStringA(hello);
	return hwnd;
}

HOOKFUNC int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	return MessageBoxA(hWnd, lpText, lpCaption, uType);
}
HOOKFUNC int WINAPI MyMessageBoxW(HWND hWnd, LPWSTR lpText, LPWSTR lpCaption, UINT uType)
{
	return MessageBoxW(hWnd, lpText, lpCaption, uType);
}

HOOKFUNC void MyOutputDebugStringA(LPCSTR lpOutputString)
{
    return OutputDebugStringA(lpOutputString);
}
