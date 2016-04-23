#include <Windows.h>

// TODO: Wrapper class around intrin.h to create CRT independent atomic lock
#include <intrin.h>
#include "../shared/ipc.h"

#include "ipc_poc.h"

namespace IPCInternal
{
    static IPC::IPCFrame ipc_frame;
    static volatile LONG ipc_lock;
}

namespace IPC
{
    void SendIPCMessage(IPCCommand command, DWORD data_size, LPVOID data)
    {
        while (_interlockedbittestandset(&IPCInternal::ipc_lock, 0)) {}
        IPCInternal::ipc_frame = { command, data_size, data };
        _asm { int 3 };
        _InterlockedExchange(&IPCInternal::ipc_lock, 0);
    }
}