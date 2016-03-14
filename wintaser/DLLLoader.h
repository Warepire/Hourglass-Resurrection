#pragma once

#include <Windows.h>

#include <string>
#include <vector>

#include "logging.h"
#include "trace\extendedtrace.h"

class DLLLoader
{
public:
    DLLLoader() {}
    ~DLLLoader() {}

    bool ReadDLL(const std::wstring& dll)
    {
        bool rv = false;
        HANDLE file = CreateFileW(dll.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            BOOL res = GetFileSizeEx(file, &size);
            if (res)
            {
                DWORD read = 0;
                buffer.resize(size.QuadPart);
                res = ReadFile(file, &buffer[0], size.QuadPart, &read, nullptr);
                if (res && read == size.QuadPart)
                {
                    rv = true;
                }
            }
            CloseHandle(file);
        }
        return rv;
    }

    bool ParseDLL()
    {
        bool rv = false;
        dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(&buffer[0]);
        pe_header = reinterpret_cast<PIMAGE_NT_HEADERS32>(&buffer[dos_header->e_lfanew]);
        if (memcmp(&(pe_header->Signature), "PE\0\0", 4) == 0 &&
            pe_header->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 && 
            pe_header->FileHeader.SizeOfOptionalHeader != 0 &&
            (pe_header->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_DLL)) &&
            pe_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
                debugprintf("Found headers, DLL has %d sections.\n", pe_header->FileHeader.NumberOfSections);
                section_headers = reinterpret_cast<PIMAGE_SECTION_HEADER>(&buffer[dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS32)]);
                for (unsigned int i = 0; i < pe_header->FileHeader.NumberOfSections; i++)
                {
                    debugprintf("Section %s found.\n", section_headers[i].Name);
                    debugprintf("    VirtualSize: 0x%X\n", section_headers[i].Misc.VirtualSize); //?
                    debugprintf("    VirtualAddress: 0x%X\n", section_headers[i].VirtualAddress);
                    debugprintf("    SizeOfRawData: 0x%X\n", section_headers[i].SizeOfRawData);
                    debugprintf("    PointerToRawData: 0x%X\n", section_headers[i].PointerToRawData);
                    debugprintf("    PointerToRelocations: 0x%X\n", section_headers[i].PointerToRelocations);
                    debugprintf("    PointerToLinenumbers: 0x%X\n", section_headers[i].PointerToLinenumbers);
                    debugprintf("    NumberOfRelocations: 0x%X\n", section_headers[i].NumberOfRelocations);
                    debugprintf("    NumberOfLinenumbers: 0x%X\n", section_headers[i].NumberOfLinenumbers);
                    debugprintf("    Characteristics: 0x%X\n", section_headers[i].Characteristics);
                    if (memcmp(section_headers[i].Name, ".reloc\0\0", 8) == 0)
                    {
                        reloc_section_number = i;
                        relocations = reinterpret_cast<PIMAGE_BASE_RELOCATION>(&buffer[section_headers[i].PointerToRawData]);
                        LPBYTE local_reloc_ptr = reinterpret_cast<LPBYTE>(relocations);
                        debugprintf("Relocation table found\n");
                        for (unsigned int j = 0; j < section_headers[i].Misc.VirtualSize;)
                        {
                            PIMAGE_BASE_RELOCATION this_reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(local_reloc_ptr + j);
                            DWORD number_of_relocs = (this_reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                            debugprintf("    VirtualAddress: 0x%X\n", this_reloc->VirtualAddress);
                            debugprintf("    SizeOfBlock: 0x%X\n", this_reloc->SizeOfBlock);
                            debugprintf("    Relocs: %d\n", number_of_relocs);
                            PWORD relocs = reinterpret_cast<PWORD>(local_reloc_ptr + j + sizeof(IMAGE_BASE_RELOCATION));
                            for (DWORD r = 0; r < number_of_relocs; r++)
                            {
                                debugprintf("        Type 0x%X   Offset 0x%X\n", GetRelocationType(relocs[r]), GetRelocationOffset(relocs[r]));
                            }
                            j += this_reloc->SizeOfBlock;
                        }
                        rv = true;
                    }
                }
        }
        return rv;
    }

    bool LoadDLLRemotely(HANDLE process, void** dll)
    {
        if (dll == nullptr)
        {
            return false;
        }
        *dll = VirtualAllocEx(process, nullptr, pe_header->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (*dll == nullptr)
        {
            return false;
        }
        debugprintf("Allocated 0x%X bytes of memory at 0x%X while preferred 0x%X.\n", pe_header->OptionalHeader.SizeOfImage, *dll, pe_header->OptionalHeader.ImageBase);
        // Write the headers.
        DWORD written;
        BOOL rv = WriteProcessMemory(process, *dll, buffer.data(), section_headers->PointerToRawData, &written);
        if (rv == FALSE || written < section_headers->PointerToRawData)
        {
            return false;
        }
        // Write the sections
        LPBYTE address = static_cast<LPBYTE>(*dll);
        for (WORD i = 0; i < pe_header->FileHeader.NumberOfSections; i++)
        {
            rv = WriteProcessMemory(process, address + section_headers[i].VirtualAddress, &buffer[section_headers[i].PointerToRawData], section_headers[i].SizeOfRawData, &written);
            if (rv == FALSE && written < section_headers[i].SizeOfRawData)
            {
                return false;
            }
        }
        if (reinterpret_cast<DWORD>(*dll) != pe_header->OptionalHeader.ImageBase)
        {
            DoRelocation(process, *dll);
        }
        LOADSYMBOLS2(process, "POC.dll", nullptr, *dll);
        return true;
    }

private:
    std::vector<BYTE> buffer;
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS32 pe_header;
    PIMAGE_SECTION_HEADER section_headers;
    DWORD reloc_section_number;
    PIMAGE_BASE_RELOCATION relocations;

    WORD GetRelocationType(WORD entry) const
    {
        return (entry & 0xF000) >> 12;
    }

    WORD GetRelocationOffset(WORD entry) const
    {
        return (entry & 0x0FFF);
    }

    bool DoRelocation(HANDLE process, void* dll)
    {
        // offset < 0 when loaded before preferred base, > 0 when loaded after.
        INT offset = reinterpret_cast<DWORD>(dll) - pe_header->OptionalHeader.ImageBase;
        debugprintf("Offsetting base with %d bytes\n", offset);
        LPBYTE local_reloc_ptr = reinterpret_cast<LPBYTE>(relocations);
        DWORD out;
        DWORD reloc_buffer;
        BOOL rv;
        for (UINT i = 0; i < section_headers[reloc_section_number].Misc.VirtualSize;)
        {
            PIMAGE_BASE_RELOCATION this_reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(local_reloc_ptr + i);
            DWORD number_of_relocs = (this_reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            PWORD relocs = reinterpret_cast<PWORD>(local_reloc_ptr + i + sizeof(IMAGE_BASE_RELOCATION));
            for (DWORD r = 0; r < number_of_relocs; r++)
            {
                LPBYTE address = reinterpret_cast<LPBYTE>(dll) + this_reloc->VirtualAddress + GetRelocationOffset(relocs[r]);
                rv = ReadProcessMemory(process, address, &reloc_buffer, sizeof(DWORD), &out);
                if (rv == FALSE || out != sizeof(DWORD))
                {
                    return false;
                }
                reloc_buffer += offset;
                rv = WriteProcessMemory(process, address, &reloc_buffer, sizeof(DWORD), &out);
                if (rv == FALSE || out != sizeof(DWORD))
                {
                    return false;
                }
            }
            i += this_reloc->SizeOfBlock;
        }
        return true;
    }
};
