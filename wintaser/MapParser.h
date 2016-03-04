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
                    std::string preferred_load_address = " Preferred load address is ";
                    std::size_t offset = file.find(preferred_load_address) + preferred_load_address.size();
                    if (offset != std::string::npos)
                    {
                        unsigned long base_address = strtoul(file.substr(offset, 8).c_str(), nullptr, 16);
                        if (base_address > 0 && base_address != ULONG_MAX)
                        {
                            std::regex function_name("[ ][?]([A-Za-z0-9]+?)[@]");
                            std::regex function_locations("[ ]([0-9A-Fa-f]+?)[ ][A-Za-z0-9 ]+.obj");

                            std::sregex_iterator end;
                            std::sregex_iterator function_matches(file.begin(), file.end(), function_name);
                            std::sregex_iterator location_matches(file.begin(), file.end(), function_locations);
                            for (; function_matches != end && location_matches != end;
                                   function_matches++, location_matches++)
                            {
                                function_map[function_matches->format("$1")] = strtoul(location_matches->format("$1").c_str(), nullptr, 16) - base_address;
                            }
                            rv = true;
                        }
                    }
                }
            }
        }
        CloseHandle(map);
        return rv;
    }
private:
    std::map<std::string, DWORD> function_map;
};