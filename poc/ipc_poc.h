#pragma once

#include <Windows.h>

#include "../shared/ipc.h"

namespace IPC
{
    void SendIPCMessage(IPCCommand command, DWORD data_size, LPVOID data);
}
