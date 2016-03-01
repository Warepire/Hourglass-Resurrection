/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef TRAMPS_H_INCL
#define TRAMPS_H_INCL

// declares trampolines for all global hooked functions
// and uses macros to locally replace direct calls to them with calls to their trampolines.
// also defines the trampoline functions if DEFINE_TRAMPS is defined.
// thus, exactly one file must define DEFINE_TRAMPS before including this.

/*
	In case you are totally lost about how this hooking stuff works,
	this explanation might clear things up:

	Hooking functions basically works like this:
	I want some function that's defined in a DLL (like timeGetTime)
	to call my own function instead of what it normally does.
	So I get the address of that function (using GetProcAddress),
	then I overwrite the first instruction inside that function
	with a jump instruction that goes to my own function (e.g. MytimeGetTime).
	
	This works great in some cases, but the problem is,
	what if I ever need to call the REAL timeGetTime from my own code?
	So what I do is also define another function (called a "trampoline function")
	whose purpose is to act like the original one when I call it.
	
	The way this trampoline function is implemented is that,
	before I overwrite the first instruction of timeGetTime,
	I copy that instruction into my trampoline function
	(otherwise it would be lost forever) and then immediately after that
	I write a jump to the second instruction of the original function.
	
	There's no normal code I could put in the trampoline function to do that,
	so instead I put some dummy code inside it that should never get called,
	which is later overwritten by those assembly instructions.
	I could dynamically allocate trampolines as they're needed, but
	I chose to (let the compiler) allocate them statically, to keep things simple.
	
	Finally, for my own code, I #define the original function
	to be the trampoline function.
	This way I can call the original as I normally would
	and behind the scenes it actually goes through the trampoline function
	to call it. (If I need to call the hooked version in my own code
	I can simply call my own function like MytimeGetTime directly,
	although that's usually a bad idea.)
*/

// include windows headers because... not being able to use types from windows headers
// would be a huge pain considering we're declaring things like win32 API replacements.
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#define _WIN32_WINNT 0x0500
//#define WINVER 0x0501
#include <windows.h>

#define TRAMPFUNC __declspec(noinline)
#define HOOKFUNC //__declspec(noinline)

#define TRAMPOLINE_DEF  INTERNAL_TRAMPOLINE_DEF
#define TRAMPOLINE_DEF_VOID  INTERNAL_TRAMPOLINE_DEF_VOID
#define TRAMPOLINE_DEF_CUSTOM(...)          { __VA_ARGS__ ; }

// this macro is used to fill trampolines with some default code as its original contents,
// that's big enough to get overwritten with a jump to the target.
// (more than big enough, and slightly different per function, to stay on the safe side.)
#define TRAMPOLINE_CONTENTS \
	static int x__ = 0; \
	x__++; \
	x__++; \
	if(x__==2){ _asm{int 3} } \
	x__++; \
	x__++;
#define INTERNAL_TRAMPOLINE_DEF { TRAMPOLINE_CONTENTS return 0; }
#define INTERNAL_TRAMPOLINE_DEF_VOID { TRAMPOLINE_CONTENTS }

// the trampoline's original contents should never get called.
// mistakes in the code that could cause it to get called are:
// 1: if you forget to use the TRAMPFUNC macro, and the compiler inlines the trampoline contents into the caller.
// 2: if you're adding code that gets called from DllMain before hooks are set up, and you forget to check notramps.
// 3: if you're calling the trampoline to a function that's in a DLL that isn't loaded by the game.
// if you expect 2 or 3 to be a problem somewhere, you could put a call to the real function
// inside the initial trampoline contents, but beware the linking requirements that can add.
// mistake 3 can be fixed by setting the hook mode to force load that DLL so the trampoline is valid,
// but I try to minimize this to keep the program as lightweight as I reasonably can.

#ifndef DEFINE_TRAMPS
	// temporarily change these macros from definition to declaration
	#undef TRAMPOLINE_DEF
	#undef TRAMPOLINE_DEF_VOID
	#undef TRAMPOLINE_DEF_CUSTOM
	#define TRAMPOLINE_DEF ;
	#define TRAMPOLINE_DEF_VOID ;
	#define TRAMPOLINE_DEF_CUSTOM(...) ;
#endif

#include "windowtramps.h"

#ifndef DEFINE_TRAMPS
	// restore definition macros
	#undef TRAMPOLINE_DEF
	#undef TRAMPOLINE_DEF_VOID
	#undef TRAMPOLINE_DEF_CUSTOM
	#define TRAMPOLINE_DEF  INTERNAL_TRAMPOLINE_DEF
	#define TRAMPOLINE_DEF_VOID  INTERNAL_TRAMPOLINE_DEF_VOID
	#define TRAMPOLINE_DEF_CUSTOM(...)          { __VA_ARGS__ ; }
#else
	bool notramps = true;
#endif

// if notramps is true, it is unsafe to call any trampoline functions.
// notramps is only true for a brief time during startup (roughly until DllMain returns),
// and shouldn't need to be checked in many places.
// it especially needn't be checked when calling the trampoline from the same function's hook.
extern bool notramps;

#undef DEFINE_TRAMPS
#else
#error DEFINE_TRAMPS was defined too late in the file
#endif
