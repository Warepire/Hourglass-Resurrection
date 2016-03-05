/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

// API for actually performing interception/hooking

// transforms all calls by the current process to the function at dwAddressToIntercept
// into calls to the provided function dwReplaced,
// and transforms all calls to the function at dwTrampoline
// into calls to the "old" function that was at dwAddressToIntercept before.
//BOOL InterceptGlobalFunction(FARPROC dwAddressToIntercept, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, BOOL rvOnSkip=TRUE);

// same as dwAddressToIntercept, except it automatically calculates dwAddressToIntercept
// based on the DLL name (c_szDllName) and function name (c_szApiName) that you provide.
//BOOL InterceptAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, bool forceLoad=false, bool allowTrack=true, BOOL rvOnSkip=TRUE, bool ordinal = false);

// these were for "mass-hooking" functions with the same name in multiple DLLs. not used anymore.
//void InterceptAllDllAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false); // expects c_szDllName, dwReplaced, and dwTrampoline to actually be pointers to equally-sized null-terminated arrays of their respective types
//BOOL InterceptAPIDynamicTramp(const char* c_szDllName, const char* c_szApiName, FARPROC*& dwReplacedArrayPtr, FARPROC*& dwTrampolineArrayPtr, bool trampolineOnly, const char**& c_szDllNameArrayPtr);

// call this when a new DLL gets loaded dynamically,
// to catch functions that InterceptAPI originally failed on due to the DLL not being loaded yet.
// you don't have to specify which functions to hook,
// because it remembers that based on which previous calls to InterceptAPI failed.
//void RetryInterceptAPIs(const char* c_szDllName);

// call this when a DLL gets unloaded dynamically,
// otherwise if it gets loaded again later,
// RetryInterceptAPIs will think it's already hooked and won't know it needs to re-hook it.
//void UnInterceptUnloadingAPIs(const char* c_szDllName);

//struct InterceptDescriptor
//{
//    const char* dllName;
//    const char* functionName;
//    FARPROC replacementFunction;
//    FARPROC trampolineFunction;
//
//    /*
//     * The meaning of "enabled" (the first parameter to MAKE_INTERCEPT) is:
//     *  0 means to point our trampoline at it if it exists, and nothing else
//     *  1 means to point it at our replacement function and point our trampoline at it if it exists
//     *  2 means the same as 1 except force dll to load if it does not exist so our trampoline always works
//     * -1 means the same as 0 except force dll to load if it does not exist so our trampoline always works
//     */
//    int enabled;
//
//    bool ordinal;
//    //const char** dllNameArray;
//};

//#define MAKE_INTERCEPT(enabled, dll, name) {#dll".dll", #name, (FARPROC)My##name, (FARPROC)Tramp##name, enabled, false}
//#define MAKE_INTERCEPT2(enabled, dll, name, myname) {#dll".dll", #name, (FARPROC)My##myname, (FARPROC)Tramp##myname, enabled, false}
//#define MAKE_INTERCEPT3(enabled, dllWithExt, name, suffix) {#dllWithExt, #name, (FARPROC)My##name##_##suffix, (FARPROC)Tramp##name##_##suffix, enabled, false}
//#define MAKE_INTERCEPT_ORD(enabled, dll, name, ordinal) {#dll".dll", reinterpret_cast<const char*>(ordinal), (FARPROC)My##name, (FARPROC)Tramp##name, enabled, true}
//#define MAKE_INTERCEPT_DYNAMICTRAMP(enabled, dllWithExt, name) {#dllWithExt, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?3:-3, ArrayName##name}
//#define MAKE_INTERCEPT_ALLDLLS(enabled, name) {NULL, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?4:-4, (const char*)(void*)ArrayName##name}


//void ApplyInterceptTable(const InterceptDescriptor* intercepts, int count);

#include <string>
bool HookModule(std::string module, LPVOID base_address, HANDLE debuggee, LPVOID injectee);
