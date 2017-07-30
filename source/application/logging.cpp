﻿#include <windows.h>

#include <sstream>

#include "logging.h"

#include <stdio.h>
//#include "../shared/logcat.h"
#include "shared/ipc.h"
#include "Config.h"
//#define ANONYMIZE_PRINT_NUMS // for simplifying diffs (debugging)

#include "DbgHelp/DbgHelp.h"

FILE* debuglogfile = NULL;

CRITICAL_SECTION g_debugPrintCS;

//extern TasFlags localTASflags;

void InitDebugCriticalSection()
{
	InitializeCriticalSection(&g_debugPrintCS);
}

int debugprintf(LPCWSTR fmt, ...)
{
	WCHAR str[4096];
	va_list args;
	va_start (args, fmt);
	int rv = vswprintf (str, fmt, args);
	va_end (args);
#ifdef ANONYMIZE_PRINT_NUMS
	{
		char* pstr = str;
		while(char c = *pstr)
		{
			if(c == '0')
			{
				while(char c2 = *pstr)
				{
					if(c2 >= '0' && c2 <= '9'
					|| c2 >= 'a' && c2 <= 'f'
					|| c2 >= 'A' && c2 <= 'F'
					|| c2 == 'x' || c2 == 'X')
					{
						*pstr = 'X';
					}
					else
					{
						break;
					}
					pstr++;
				}
			}
			pstr++;
		}
	}
#endif
    if (IsDebuggerPresent())
    {
        OutputDebugStringW(str);
    }
	EnterCriticalSection(&g_debugPrintCS);
    if (!debuglogfile)
    {
        debuglogfile = fopen("hourglasslog.txt", "w");
        if (debuglogfile)
        {
            fwide(debuglogfile, 1);
        }
    }
	if(debuglogfile)
	{
		fputws(str, debuglogfile);
		fflush(debuglogfile);
	}
	LeaveCriticalSection(&g_debugPrintCS);
	return rv;
}

void PrintLastError(LPCWSTR lpszFunction, DWORD dw)
{
	if(!dw)
		return;

	LPVOID lpMsgBuf;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL );

	debugprintf(L"%s failed, error %d: %s", lpszFunction, dw, lpMsgBuf);
    LocalFree(lpMsgBuf);
}

IDbgHelpStackWalkCallback::Action PrintStackTrace(IDbgHelpStackWalkCallback& data)
{
    /*
     * C++ dictates that classes inside functions can't declare template methods,
     * classes inside lamdas can however, so, just wrap it.
     * -- Warepire
     */
    auto dummy = [&data]() {
        std::wostringstream oss(L"STACK FRAME: ");
        oss.setf(std::ios_base::showbase);
        oss.setf(std::ios_base::hex, std::ios_base::basefield);

        /*
         * Add the module and function name.
         */
        oss << data.GetModuleName() << L"!0x" << data.GetProgramCounter();
        oss << L" : " << data.GetFunctionName() << L'(';

        size_t arg_number = 1;
        for (const auto& parameter : data.GetParameters())
        {
            if (arg_number > 1)
            {
                oss << L", ";
            }

            /*
             * Add the parameter type and name.
             */
            oss << parameter.m_type.GetName() << L' ' << parameter.m_name << L" = ";

            /*
             * Add the parameter value.
             */
            if (parameter.m_value.has_value())
            {
#pragma message(__FILE__ ": TODO: change to constexpr-if lambdas when they are supported (VS2017 Preview 3)")
                class visitor
                {
                    std::wostringstream& m_oss;

                public:
                    visitor(std::wostringstream& oss) : m_oss(oss) {}

                    /*
                     * Print chars.
                     */
                    void operator()(char value)
                    {
                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                    }

                    void operator()(wchar_t value)
                    {
                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                    }

                    void operator()(char16_t value)
                    {
                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                    }

                    void operator()(char32_t value)
                    {
                        m_oss << static_cast<int>(value) << L" \'" << value << L'\'';
                    }

                    /*
                     * Pointers ignore showbase.
                     */
                    void operator()(void* value)
                    {
                        m_oss << L"0x" << value;
                    }

                    template<typename T>
                    void operator()(T value)
                    {
                        m_oss << value;
                    }
                };

                std::visit(visitor(oss), parameter.m_value.value());
            }

            ++arg_number;
        }

        oss << L')';

        /*
         * Add the unsure status display.
         */
        if (data.GetUnsureStatus() > 0)
        {
            oss << L'?';
        }

        oss << L'\n';

        debugprintf(L"%s", oss.str().c_str());

        return IDbgHelpStackWalkCallback::Action::CONTINUE;
    };
    return dummy();
};