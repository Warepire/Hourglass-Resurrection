/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../global.h"

HOOKFUNC HWND WINAPI MyCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
	LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND hwnd = CreateWindowExA(dwExStyle, lpClassName,
		lpWindowName, dwStyle, X, Y, nWidth, nHeight,
		hWndParent, hMenu, hInstance, lpParam);

	MessageBoxA(hwnd, "Hello from the hook!", "Messagebox!", MB_OK | MB_ICONINFORMATION);

	return hwnd;
}
HOOKFUNC HWND WINAPI MyCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
	LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND hwnd = CreateWindowExW(dwExStyle, lpClassName,
		lpWindowName, dwStyle, X, Y, nWidth, nHeight,
		hWndParent, hMenu, hInstance, lpParam);

	MessageBoxW(hwnd, L"Hello from the hook!", L"Messagebox!", MB_OK | MB_ICONINFORMATION);

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
