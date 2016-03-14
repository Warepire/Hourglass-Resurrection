/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define CreateWindowExA TrampCreateWindowExA
TRAMPFUNC HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
	LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) TRAMPOLINE_DEF
#define CreateWindowExW TrampCreateWindowExW
TRAMPFUNC HWND WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
	LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) TRAMPOLINE_DEF

#define MessageBoxA TrampMessageBoxA
TRAMPFUNC int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) TRAMPOLINE_DEF
#define MessageBoxW TrampMessageBoxW
TRAMPFUNC int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) TRAMPOLINE_DEF

#define OutputDebugStringA TrampOutputDebugStringA
TRAMPFUNC void WINAPI OutputDebugStringA(LPCSTR lpOutputString) TRAMPOLINE_DEF_VOID
