/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <memory>
#include <vector>

/*
 * Default location of the DIA SDK within VS2015 Community edition.
 */
#include <../../DIA SDK/include/dia2.h>

#include "DbgHelp.h"

namespace
{
    /*
     * Custom unique_ptr deleter for COM objects.
     * TODO: Put in some sort of utils header / namespace?
     */
    template<class T>
    class COMObjectDeleter
    {
    public:
        void operator()(T* ptr)
        {
            ptr->Release();
        }
    };
}

class DbgHelpPriv
{
public:
    bool LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path);
private:
    std::map<DWORD64, std::unique_ptr<IDiaDataSource, COMObjectDeleter<IDiaDataSource>>> m_symbols;
};

bool DbgHelpPriv::LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path)
{
    IDiaDataSource* data_source = nullptr;
    bool rv = (CoCreateInstance(CLSID_DiaSource,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(IDiaDataSource),
                                reinterpret_cast<LPVOID*>(&data_source)) == S_OK);
    if (!rv)
    {
        return rv;
    }

    rv = data_source->loadDataForExe(exec.c_str(), search_path.c_str(), nullptr);
    if (!rv)
    {
        return rv;
    }

    m_symbols.emplace(module_base, std::move(data_source));
}

DbgHelp::DbgHelp(HANDLE process) :
    m_process(process)
{
    std::array<WCHAR, 0x1000> buffer;
    buffer.fill('\0');

    if (GetCurrentDirectoryW(buffer.size(), buffer.data()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_ALTERNATIVE_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"SYSTEMROOT", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data())
                      .append(L";").append(buffer.data()).append(L"\\system32")
                      .append(L";").append(buffer.data()).append(L"\\SysWOW64");
    }
    /*
    * TODO: Enable when SymbolServer support is added. Needs symsrv.dll.
    * This will enable downloading of symbols for the system DLLs making stack traces more accurate.
    * -- Warepire
    */
    //symbol_paths.append(";").append("SRV*%SYSTEMDRIVE%\\websymbols*http://msdl.microsoft.com/download/symbols");
}

DbgHelp::~DbgHelp()
{

}

void DbgHelp::LoadSymbols(HANDLE module_file, LPCSTR module_name, DWORD64 module_base)
{
    if (m_private->LoadSymbols(module_base, module_name, m_symbol_paths))
    {
        m_loaded_modules.emplace(module_base, module_name);
    }
}

std::vector<DbgHelp::StacktraceInfo> DbgHelp::Stacktrace(HANDLE thread, INT max_depth)
{
    std::vector<DbgHelp::StacktraceInfo> trace;
    CONTEXT thread_context;
    STACKFRAME64 stack_frame;

    thread_context.ContextFlags = CONTEXT_FULL;
    if (m_loaded_modules.empty())
    {
        return trace;
    }
    if (GetThreadContext(thread, &thread_context) == FALSE)
    {
        return trace;
    }

    stack_frame.AddrPC.Offset = thread_context.Eip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = thread_context.Esp;
    stack_frame.AddrStack.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = thread_context.Ebp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;

    for (INT i = 0; ; i++)
    {
        BOOL rv = StackWalk64Pointer(IMAGE_FILE_MACHINE_I386, m_process, thread,
                                     &stack_frame, &thread_context, nullptr,
                                     SymFunctionTableAccess64Pointer, SymGetModuleBase64Pointer,
                                     nullptr);
        if (rv == FALSE)
        {
            break;
        }
        DWORD64 address = stack_frame.AddrPC.Offset;

        /*
         * lower_bound() will return an iterator to the location in the map where 'address' should
         * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
         * module 'address' belongs to by getting this iterator, and then iterate backwards once.
         */
        auto module_it = m_loaded_modules.lower_bound(address);
        module_it--;
        trace.emplace_back(address, module_it->second);

        if (i >= max_depth)
        {
            break;
        }
    }
    return trace;
}

std::vector<std::wstring> DbgHelp::GetFunctionTrace(const std::vector<StacktraceInfo>& trace)
{
    std::vector<std::wstring> functions(trace.size());
    DWORD64 mod_address;
    ULONG type_index;

    for (auto& i = trace.begin(); i != trace.end(); i++)
    {
        if (GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
            GetSymbolTag(mod_address, type_index) == SymTagFunction)
        {
            functions.emplace_back(GetSymbolName(mod_address, type_index));
        }
        else
        {
            functions.emplace_back(L"?");
        }
    }
    return functions;
}

std::vector<std::wstring> DbgHelp::GetFullTrace(const std::vector<StacktraceInfo>& trace)
{
    std::vector<std::wstring> full_trace(trace.size());

    DWORD64 mod_address;
    ULONG type_index;

    for (auto& i = trace.begin(); i != trace.end(); i++)
    {
        if (!GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
            GetSymbolTag(mod_address, type_index) != SymTagFunction)
        {
            full_trace.emplace_back(L"?");
            continue;
        }

        std::wstring symbol;
        DWORD params = GetParamCountForClass(mod_address, type_index);
        DWORD class_index = GetParentClass(mod_address, type_index);
        if (params == GetParamCount(mod_address, type_index) && class_index != 0)
        {
            symbol.append(L"static ");
        }
        symbol.append(GetTypeName(mod_address, type_index));
        if (class_index != 0)
        {
            symbol.append(GetSymbolName(mod_address, class_index)).append(L"::");
        }
        symbol.append(GetSymbolName(mod_address, type_index)).append(L"(");

        full_trace.emplace_back(symbol);
    }

    return full_trace;
}

BOOL CALLBACK DbgHelp::LoadSymbolsCallback(PSYMBOL_INFO symbol_info, ULONG symbol_size, PVOID user_data)
{
    return TRUE;
}

bool DbgHelp::GetModuleBaseAndSymIndex(DWORD64 address, PDWORD64 mod_base, PULONG index)
{
    if (mod_base == nullptr || index == nullptr)
    {
        return false;
    }

    BOOL rv;
    DWORD64 symbol_displacement;
    std::array<BYTE, MAX_SYM_NAME + sizeof(SYMBOL_INFO)> symbol_buffer;
    PSYMBOL_INFO symbol_info = reinterpret_cast<PSYMBOL_INFO>(symbol_buffer.data());
    symbol_info->MaxNameLen = MAX_SYM_NAME;
    symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);

    rv = SymFromAddrPointer(m_process, address, &symbol_displacement, symbol_info);
    if (rv == FALSE)
    {
        return false;
    }

    *mod_base = symbol_info->ModBase;
    *index = symbol_info->TypeIndex;

    return true;
}

DWORD DbgHelp::GetSymbolTag(DWORD64 module_base, ULONG type_index)
{
    DWORD symbol_tag = SymTagNull;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_SYMTAG, &symbol_tag) == FALSE)
    {
        symbol_tag = SymTagNull;
    }
    return symbol_tag;
}

DWORD DbgHelp::GetCallingConvention(DWORD64 module_base, ULONG type_index)
{
    DWORD call_conv = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CALLING_CONVENTION, &call_conv) == FALSE)
    {
        call_conv = 0;
    }
    return call_conv;
}

DWORD DbgHelp::GetParamCount(DWORD64 module_base, ULONG type_index)
{
    DWORD param_count = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_COUNT, &param_count) == FALSE)
    {
        param_count = 0;
    }
    return param_count;
}

DWORD DbgHelp::GetParamCountForClass(DWORD64 module_base, ULONG type_index)
{
    DWORD param_count = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CHILDRENCOUNT, &param_count) == FALSE)
    {
        param_count = 0;
    }
    return param_count;
}

DWORD DbgHelp::GetParentClass(DWORD64 module_base, ULONG type_index)
{
    DWORD class_id = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CLASSPARENTID, &class_id) == FALSE)
    {
        class_id = 0;
    }
    return class_id;
}

DWORD DbgHelp::GetThisPtrOffset(DWORD64 module_base, ULONG type_index)
{
    DWORD offset = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_THISADJUST, &offset) == FALSE)
    {
        offset = 0;
    }
    return offset;
}

std::wstring DbgHelp::GetTypeName(DWORD64 module_base, ULONG type_index)
{
    DWORD type_id = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_TYPEID, &type_id))
    {
        return GetSymbolName(module_base, type_id);
    }
    return L"?";
}

std::wstring DbgHelp::GetSymbolName(DWORD64 module_base, ULONG type_index)
{
    std::wstring name(L"?");
    LPWSTR symbol_name;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_SYMNAME, &symbol_name) &&
        symbol_name != nullptr)
    {
        name = symbol_name;
        /*
         * Docs don't mention which function to use, 3rd party code examples seems to use
         * LocalFree(). Assume that is correct, if there are issues, try something else...
         * like GlobalFree().
         * -- Warepire
         */
        LocalFree(symbol_name);
    }
    return name;
}