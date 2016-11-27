/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <map>
#include <memory>
#include <string>
#include <vector>
 /*
  * Default location of the DIA SDK within VS2015 Community edition.
  */
#include <../../DIA SDK/include/dia2.h>

#include "../Utils/COM.h"
#include "DbgHelp.h"

class DbgHelpPrivate
{
public:
    DbgHelpPrivate(HANDLE process);
    ~DbgHelpPrivate();

    bool LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path);
    bool Stacktrace(HANDLE thread, INT max_depth, std::vector<DbgHelp::StackFrameInfo>* stack);

    HANDLE GetProcess() const;
    IDiaDataSource* GetDiaDataSource(DWORD64 virtual_address) const;
    IDiaSession* GetDiaSession(IDiaDataSource* source) const;
private:
    HANDLE m_process;
    std::map<DWORD64, Utils::COM::UniqueCOMPtr<IDiaDataSource>> m_sources;
    std::map<IDiaDataSource*, Utils::COM::UniqueCOMPtr<IDiaSession>> m_sessions;
    /*
     * Assume everything is compiled for the same platform.
     */
    CV_CPU_TYPE_e m_platform;
    bool m_platform_set;
    bool m_should_uninitialize;
};