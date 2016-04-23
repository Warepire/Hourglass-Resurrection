/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "logcat.h"
#include <mmsystem.h>

struct TasFlags
{
	int playback;
	int framerate;
	int keylimit;
	int forceSoftware;
	int windowActivateFlags;
	int threadMode;
	unsigned int threadStackSize;
	int timersMode;
	int messageSyncMode;
	int waitSyncMode;
	int aviMode;
	int emuMode;
	int forceWindowed;
	int fastForward;
	int forceSurfaceMemory;
	int audioFrequency;
	int audioBitsPerSecond;
	int audioChannels;
	int stateLoaded;
	int fastForwardFlags;
	int initialTime;
	int debugPrintMode;
	int timescale, timescaleDivisor;
	int frameAdvanceHeld;
	int allowLoadInstalledDlls, allowLoadUxtheme;
	int storeVideoMemoryInSavestates;
	int appLocale;
	unsigned int movieVersion;
	int osVersionMajor, osVersionMinor;
	LogCategoryFlag includeLogFlags;
	LogCategoryFlag excludeLogFlags;
#ifdef _USRDLL
	char reserved [256]; // just-in-case overwrite guard
#endif
};
#ifdef _USRDLL
extern TasFlags tasflags;
#endif

struct DllLoadInfo
{
	bool loaded; // true=load, false=unload
	char dllname [64];
};
struct DllLoadInfos
{
	int numInfos; // (must be the first thing in this struct, due to an assumption in AddAndSendDllInfo)
	DllLoadInfo infos [128];
};


struct InfoForDebugger // GeneralInfoFromDll
{
	int frames;
	int ticks;
	int addedDelay;
	int lastNewTicks;
};


enum
{
	CAPTUREINFO_TYPE_NONE, // nothing sent
	CAPTUREINFO_TYPE_NONE_SUBSEQUENT, // nothing sent and it's the same frame/time as last time
	CAPTUREINFO_TYPE_PREV, // reuse previous frame's image (new sleep frame)
	CAPTUREINFO_TYPE_DDSD, // locked directdraw surface description
};

struct LastFrameSoundInfo
{
	DWORD size;
	unsigned char* buffer;
	LPWAVEFORMATEX format;
};


enum
{
	EMUMODE_EMULATESOUND = 0x01,
	EMUMODE_NOTIMERS = 0x02,
	EMUMODE_NOPLAYBUFFERS = 0x04,
	EMUMODE_VIRTUALDIRECTSOUND = 0x08,
};

enum
{
	FFMODE_FRONTSKIP = 0x01,
	FFMODE_BACKSKIP = 0x02,
	FFMODE_SOUNDSKIP = 0x04,
	FFMODE_RAMSKIP = 0x08,
	FFMODE_SLEEPSKIP = 0x10,
	FFMODE_WAITSKIP = 0x20,
};


struct TrustedRangeInfo
{
	DWORD start, end;
};
struct TrustedRangeInfos
{
	int numInfos;
	TrustedRangeInfo infos [32]; // the first one is assumed to be the injected dll's range
};

#ifndef SUCCESSFUL_EXITCODE
#define SUCCESSFUL_EXITCODE 4242
#endif

// ===============================================================================

// PoC IPC
namespace IPC
{
    /*
     * This IPC Command enum contains all the command and reply IDs, they work similarly to the old
     * scheme of IPC where the direction of the buffer is decided by the ID, even ID comes from the DLL
     * and odd ID from the debugger. So the IDs also double as an IPC state.
     */
    enum IPCCommand : DWORD
    {
        /*
         * Skip commands 0 and 1 to be able to tell the difference between a newly memset buffer and
         * an actual command.
         */
        CMD_PRINT_MESSAGE = 0x00000002,
        CMD_PRINT_MESSAGE_REPLY,
    };

    struct PrintMessage
    {
        /* Bad "strcpy" to prevent use of _memset */
        PrintMessage(const char* string)
        {
            for (DWORD i = 0; i < sizeof(message) && string[i] != '\0'; i++)
            {
                message[i] = string[i];
            }
        }
        char message[4096];
    };

    struct IPCFrame
    {
        IPCCommand command;
        DWORD data_size;
        LPVOID data_location;
    };
}
