/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include <windows.h>
#include "logging.h"
#include "intercept.h"
#include "../shared/asm.h"
#include "MapParser.h"

#include <string>
#include <map>
#include <vector>
struct InterceptAPIArgs
{
    LPCSTR funcName;
    bool trampolineOnly;
    //bool ordinal;

    InterceptAPIArgs(LPCSTR a, bool b/*, bool c*/) :
        funcName(a), trampolineOnly(b)/*, ordinal(c)*/
    {
    }
};

extern DLLMapReader mr;
std::map<std::string, std::vector<InterceptAPIArgs>> hook_table =
{
    { "user32.dll", { { "CreateWindowExA", false },
                      { "CreateWindowExW", false },
                      { "MessageBoxA",     false },
                      { "MessageBoxW",     false },
                    }
    }
};

BOOL InterceptGlobalFunction(HANDLE debuggee, FARPROC dwAddressToIntercept, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly, BOOL rvOnSkip = FALSE)
{
    debugprintf("Hooking 0x%X (hook=0x%X tramp=0x%X)\n", dwAddressToIntercept, dwReplaced, dwTrampoline);
	if(!dwAddressToIntercept)
		return FALSE;

    // JMP_REL32 causes the CPU to read the next 4 bytes and add them to the instruction pointer,
    // in addition to the opcode length itself.
	enum
    {
        JMP_REL32 = 0xE9,
        JMP_REL32_OPCODE_LEN = 5,
    };

    BYTE buffer[32];
	BYTE* pTargetHead = (BYTE*)dwAddressToIntercept;
	BYTE* pTramp = (BYTE*)dwTrampoline;
	BYTE* pHook = (BYTE*)dwReplaced;
    DWORD dwOldProtect;
    DWORD read = 0;
    DWORD written = 0;

	// trampolines work by running a copy of the first few bytes of the target function
	// and then jumping to the remainder of the target function.
	// this totally breaks down if the first few bytes of the target function is a jump.
	// so, the first thing we do is detect if the target starts with a jump,
	// and if so, we follow where the jump goes instead of hooking the target directly.
    unsigned int i = 0;
    do {
        memset(buffer, 0, sizeof(buffer));
        VirtualProtectEx(debuggee, pTargetHead, sizeof(buffer), PAGE_EXECUTE_READWRITE, &dwOldProtect);
        ReadProcessMemory(debuggee, pTargetHead, buffer, sizeof(buffer), &read);
        VirtualProtectEx(debuggee, pTargetHead, sizeof(buffer), PAGE_EXECUTE, &dwOldProtect);
        //debugprintf("Buffer contains: %X %X %X %X %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
        if (buffer[0] != JMP_REL32)
        {
            break;
        }
		int diff = *reinterpret_cast<int*>(&(buffer[1])) + JMP_REL32_OPCODE_LEN;
		pTargetHead += diff;
		if(pTargetHead == pHook)
		{
			if(i == 0)
				debugprintf("already hooked. skipping.\n");
			else
				debugprintf("already hooked (from 0x%X to 0x%X), although the hook chain is longer than expected. skipping.\n", pTargetHead-diff, pTargetHead);
			return rvOnSkip;
		}
		else
		{
			if(i < 64)
				debugprintf("rejump detected in target (from 0x%X to 0x%X). following chain...\n", pTargetHead-diff, pTargetHead);
			else
			{
				debugprintf("hook chain is too long. target function jumps to itself? skipping.\n");
				return rvOnSkip;
			}
		}
        i++;
    } while (true);

	if(pTramp == pHook || pTargetHead == pTramp)
	{
		debugprintf("bad input? target=0x%X, hook=0x%X, tramp=0x%X. skipping hook.\n", pTargetHead, pHook, pTramp);
		return rvOnSkip;
	}
	//debuglog(LCF_HOOK, "want to hook 0x%X to call 0x%X, and want trampoline 0x%X to call 0x%X\n", pTargetHead, pHook, pTramp, pTargetTail);

	//if(*pTramp == JMP_REL32)
	//{
	//	int diff = *(DWORD*)(pTramp+1) + 5;
	//	debugprintf("rejump detected in trampoline (from 0x%X to 0x%X). giving up.\n", pTramp, pTramp+diff);
	//	return rvOnSkip;
	//}

	// we'll have to overwrite JMP_REL32_OPCODE_LEN bytes,
    // which means we have to backup at least JMP_REL32_OPCODE_LEN bytes (up to next instruction boundary)
	int offset = 0;
	while(offset < JMP_REL32_OPCODE_LEN)
		offset += instructionLength(buffer + offset);
    //debugprintf("Bytes to copy: %d\n", offset);

    memset(buffer + offset, 0, sizeof(buffer) - offset);
    buffer[offset] = JMP_REL32;
    *reinterpret_cast<int*>(&buffer[offset + 1]) = pTargetHead - (pTramp + /*JMP_REL32_OPCODE_LEN + */offset);
    //debugprintf("Trampoline:");
    //for (unsigned int i = 0; i < offset + JMP_REL32_OPCODE_LEN; i++)
    //{
    //    debugprintf(" %X", buffer[i]);
    //}
    //debugprintf("\n");

    // in the trampoline, write the first JMP_REL32_OPCODE_LEN+ bytes of the target function,
    // followed by a jump to the original code
    WriteProcessMemory(debuggee, dwTrampoline, buffer, JMP_REL32_OPCODE_LEN + offset, &written);

	if(!trampolineOnly)
	{
        memset(buffer, 0, sizeof(buffer));
        buffer[0] = JMP_REL32;
        *reinterpret_cast<int*>(&buffer[1]) = pHook - ((BYTE*)dwAddressToIntercept + JMP_REL32_OPCODE_LEN);
        //debugprintf("Hook:");
        //for (unsigned int i = 0; i < JMP_REL32_OPCODE_LEN; i++)
        //{
        //    debugprintf(" %X", buffer[i]);
        //}
        //debugprintf("\n");
        // overwrite the first JMP_REL32_OPCODE_LEN bytes of the target function with a jump to our hook
        WriteProcessMemory(debuggee, dwAddressToIntercept, buffer, JMP_REL32_OPCODE_LEN, &written);
	}

	// flush the instruction cache to make sure the modified code is executed
	FlushInstructionCache(debuggee, nullptr, 0); 

	return TRUE;
}



//BOOL InterceptAPI(const char* c_szDllName, const char* c_szApiName,
//				  FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly, bool forceLoad, bool allowTrack, BOOL rvOnSkip, bool ordinal)
//{
//	if (ordinal)
//	{
//		//debuglog(LCF_MODULE | LCF_HOOK, "InterceptAPI(%s.(LPCSTR)%p) %s\n", c_szDllName, c_szApiName, "started...");
//	}
//	else
//	{
//		//debuglog(LCF_MODULE | LCF_HOOK, "InterceptAPI(%s.%s) %s\n", c_szDllName, c_szApiName, "started...");
//	}
//
//	HMODULE hModule = GetModuleHandle(c_szDllName);
//	if(!hModule && forceLoad)
//	{
//		if (ordinal)
//		{
//			debugprintf("WARNING: Loading module %s to hook (LPCSTR)%p\n", c_szDllName, c_szApiName);
//		}
//		else
//		{
//			debugprintf("WARNING: Loading module %s to hook %s\n", c_szDllName, c_szApiName);
//		}
//		// dangerous according to the documentation,
//		// but the alternative is usually to fail spectacularly anyway
//		// and it seems to work for me...
//		hModule = LoadLibrary(c_szDllName);
//	}
//	FARPROC dwAddressToIntercept = GetProcAddress(hModule, (char*)c_szApiName); 
//	BOOL rv = InterceptGlobalFunction(dwAddressToIntercept, dwReplaced, dwTrampoline, trampolineOnly, rvOnSkip);
//	if (rv || allowTrack)
//	{
//		if (ordinal)
//		{
//			debugprintf("InterceptAPI(%s.(LPCSTR)%p) %s \t(0x%X, 0x%X)\n", c_szDllName, c_szApiName, rv ? (trampolineOnly ? "bypassed." : "succeeded.") : "failed!", dwAddressToIntercept, dwTrampoline);
//		}
//		else
//		{
//			debugprintf("InterceptAPI(%s.%s) %s \t(0x%X, 0x%X)\n", c_szDllName, c_szApiName, rv ? (trampolineOnly ? "bypassed." : "succeeded.") : "failed!", dwAddressToIntercept, dwTrampoline);
//		}
//	}
//
//	return rv;
//}

bool HookModule(std::string module, LPVOID base_address, HANDLE debuggee, LPVOID injectee)
{
    bool rv = false;
    auto it = hook_table.find(module.substr(module.rfind("\\") + 1));
    if (it == hook_table.end())
    {
        debugprintf("HookModule: Did not find module %s in list\n", module.c_str());
        return rv;
    }
    debugprintf("HookModule: Found module %s in list\n", module.c_str());
    std::vector<InterceptAPIArgs>& functions_to_hook = it->second;
    HMODULE mod = GetModuleHandleA(module.c_str());
    bool should_free = false;
    if (mod == nullptr)
    {
        debugprintf("HookModule: Module %s is not loaded\n", module.c_str());
        mod = LoadLibraryA(module.c_str());
        if (mod == nullptr)
        {
            debugprintf("HookModule: Module %s failed to load\n", module.c_str());
            return rv;
        }
        should_free = true;
    }
    debugprintf("HookModule: Module %s loaded\n", module.c_str());
    for (auto& h : functions_to_hook)
    {
        rv = false;
        auto tramp_it = mr.function_map.find(std::string("Tramp") + h.funcName);
        auto my_it = mr.function_map.find(std::string("My") + h.funcName);
        if (tramp_it == mr.function_map.end() || my_it == mr.function_map.end())
        {
            debugprintf("HookModule: Failed to find My/Tramp!\n");
            break;
        }
        FARPROC function = GetProcAddress(mod, h.funcName);
        if (function == nullptr)
        {
            debugprintf("HookModule: Failed to get ProcAddress!\n");
            break;
        }
        FARPROC target = reinterpret_cast<FARPROC>((reinterpret_cast<LPBYTE>(function) -
                                                    reinterpret_cast<LPBYTE>(mod)) +
                                                    reinterpret_cast<LPBYTE>(base_address));
        debugprintf("HookModule: Generated target addr 0x%X from 0x%X\n", target, function);

        BOOL success = InterceptGlobalFunction(debuggee,
                                               target,
                                               reinterpret_cast<FARPROC>(my_it->second + reinterpret_cast<LPBYTE>(injectee)),
                                               reinterpret_cast<FARPROC>(tramp_it->second + reinterpret_cast<LPBYTE>(injectee)),
                                               h.trampolineOnly);
        if (success == FALSE)
        {
            debugprintf("HookModule: Failed to hook %s\n", h.funcName);
            break;
        }
        debugprintf("HookModule: Hooked %s\n", h.funcName);
        rv = true;
    }
    if (should_free)
    {
        FreeLibrary(mod);
    }
    return rv;
}