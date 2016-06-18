/*
* (c) 2016- Hourglass Resurrection Team
* Hourglass Resurrection is licensed under GPL v2.
* Refer to the file COPYING.txt in the project root.
*/

#include <Windows.h>

#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

std::map<std::string, std::map<std::string, DWORD>> function_map;
std::map<std::string, DWORD> ipc_frame_loc;

bool ReadMapFile(const std::string& filename)
{
    bool rv = false;
    HANDLE map = CreateFileA(filename.c_str(),
                             GENERIC_READ,
                             0,
                             nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                             nullptr);
    if (map != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size;
        BOOL res = GetFileSizeEx(map, &size);
        if (res && size.HighPart == 0)
        {
            DWORD read = 0;
            std::string file;
            file.resize(size.LowPart);
            res = ReadFile(map, const_cast<char*>(file.c_str()), size.LowPart, &read, nullptr);
            if (res && read == size.LowPart)
            {
                // TODO: Special identification rules for ordinals.
                std::regex entry("[ ]([0-9]+?)[:]([0-9a-fA-F]+)[ ]+?[?]((?:My|Tramp)[A-Za-z0-9]+?)[@].+[ ]([0-9a-zA-Z]+?).obj");
                //std::regex ipc("[ ]([0-9]+?)[:]([0-9a-fA-F]+)[ ]+?[?](ipc_frame+?)[@]");

                std::sregex_iterator end;
                for (std::sregex_iterator entry_matches(file.begin(), file.end(), entry);
                    entry_matches != end; entry_matches++)
                {
                    //printf("%s\n", entry_matches->format("$1 : $2 as $3 in $4").c_str());
                    std::string dll = entry_matches->format("$4");
                    std::string method = entry_matches->format("$3");
                    DWORD location = strtoul(entry_matches->format("$2").c_str(), nullptr, 16);
                    location += (0x1000 * strtoul(entry_matches->format("$1").c_str(), nullptr, 10));
                    function_map[dll][method] = location;
                }
                //for (std::sregex_iterator entry_matches(file.begin(), file.end(), ipc);
                //    entry_matches != end; entry_matches++)
                //{
                //    printf("%s\n", entry_matches->format("$1 : $2 as $3").c_str());
                //    ipc_frame_loc[entry_matches->format("$3")] = strtoul(entry_matches->format("$2").c_str(), nullptr, 16) + (0x1000 * strtoul(entry_matches->format("$1").c_str(), nullptr, 10));
                //}
                rv = true;
            }
        }
    }
    CloseHandle(map);
    return rv;
}



int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s [map file]\n", argv[0]);
        return 1;
    }
    bool rv = ReadMapFile(argv[1]);
    //printf("rv = %d\n", rv);
    if (rv)
    {
        for (auto& d : function_map)
        {
            printf("{ \"%s.dll\", {", d.first.c_str());
            for (auto& f : d.second)
            {
                // Quick dumb hack
                printf("{ \"%s\", 0x%X, false },", f.first.find("My") == 0 ? f.first.substr(strlen("My")).c_str() : f.first.substr(strlen("Tramp")).c_str(), f.second);
            }
            printf("},\n");
        }
    }
    return 0;
}
