#pragma once

#include <Windows.h>

#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "logging.h"

class DLLMapReader
{
public:
    DLLMapReader() {}
    ~DLLMapReader() {}
    bool ReadMapFile(const std::wstring& filename)
    {
        bool rv = false;
        HANDLE map = CreateFileW(filename.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (map != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            BOOL res = GetFileSizeEx(map, &size);
            if (res)
            {
                DWORD read = 0;
                std::string file;
                file.resize(size.QuadPart);
                res = ReadFile(map, const_cast<char*>(file.c_str()), size.QuadPart, &read, nullptr);
                if (res && read == size.QuadPart)
                {
                    std::regex entry("[ ]([0-9]+?)[:]([0-9a-fA-F]+)[ ]+?[?]([A-Za-z0-9]+?)[@].+[ ]([0-9a-zA-Z]+?).obj");

                    std::sregex_iterator end;
                    for (std::sregex_iterator entry_matches(file.begin(), file.end(), entry);
                         entry_matches != end; entry_matches++)
                    {
                        debugprintf("%s\n", entry_matches->format("$1 : $2 as $3 in $4").c_str());
                        function_map[entry_matches->format("$3")] = strtoul(entry_matches->format("$2").c_str(), nullptr, 16) + (0x400 * strtoul(entry_matches->format("$1").c_str(), nullptr, 10));
                    }
                    rv = true;
                }
            }
        }
        CloseHandle(map);
        return rv;
    }

    std::map<std::string, DWORD> function_map;
};