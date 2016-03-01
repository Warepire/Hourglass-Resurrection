/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#define DEFINE_TRAMPS // main file definition, must define at top of exactly one c or cpp file that includes alltramps.h

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "global.h"

BOOL APIENTRY DllMain(HMODULE hModule, 
                      DWORD  fdwReason, 
                      LPVOID lpReserved)
{
    return TRUE;
}
