#include "pch.h"
#include "MemoryTools.h"
#include "MonopolyException.h"

#include <Psapi.h>
#include <vector>
#include <string>

using namespace Monopoly;

DWORD_PTR MemoryTools::TryAllocateMemory(HANDLE processHandle, const SIZE_T size, const DWORD protectionFlags)
{
    const LPVOID allocated = VirtualAllocEx(processHandle, nullptr, size, MEM_COMMIT, protectionFlags);
    if (!allocated)
        throw gcnew MonopolyException("VirtualAllocEx failed");

    return reinterpret_cast<DWORD_PTR>(allocated);
}

void MemoryTools::TryFreeMemory(HANDLE processHandle, const DWORD_PTR address)
{
    if (!VirtualFreeEx(processHandle, reinterpret_cast<LPVOID>(address), 0, MEM_RELEASE))
        throw gcnew MonopolyException("VirtualFreeEx failed");
}

void MemoryTools::TryReadMemory(HANDLE processHandle, const DWORD_PTR address, LPVOID data, const SIZE_T size)
{
    if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), data, size, nullptr))
        throw gcnew MonopolyException("ReadProcessMemory failed");
}

void MemoryTools::TryWriteMemory(HANDLE processHandle, const DWORD_PTR address, LPCVOID data, const SIZE_T size)
{
    if (!WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), data, size, nullptr))
        throw gcnew MonopolyException("WriteProcessMemory failed");
}

HMODULE MemoryTools::TryGetMonoModule(HANDLE processHandle)
{
    std::vector<HMODULE> modules;
    DWORD sizeNeeded;

    if (!EnumProcessModules(processHandle, nullptr, 0, &sizeNeeded))
        throw gcnew MonopolyException("EnumProcessModules failed");

    modules.resize(sizeNeeded);

    if (!EnumProcessModules(processHandle, &modules[0], sizeNeeded, &sizeNeeded))
        throw gcnew MonopolyException("EnumProcessModules failed");

    std::string filename;
    filename.resize(MAX_PATH);

    for (HMODULE& module : modules)
    {
        if (!GetModuleFileNameEx(processHandle, module, &filename[0], static_cast<DWORD>(filename.size())))
            throw gcnew MonopolyException("GetModuleFileNameEx failed");

        if (filename.rfind("mono") != std::string::npos)
            return module;
    }

    throw gcnew MonopolyException("Mono module could not be found in process");
}

IMAGE_EXPORT_DIRECTORY MemoryTools::TryGetModuleExportTable(HANDLE processHandle, const DWORD_PTR moduleBaseAddress)
{
    IMAGE_DOS_HEADER dosHeader = { 0 };
    MemoryTools::TryReadMemory(processHandle, moduleBaseAddress, &dosHeader, sizeof(IMAGE_DOS_HEADER));

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        throw gcnew MonopolyException("Invalid DOS header magic");

    IMAGE_NT_HEADERS ntHeaders = { 0 };
    MemoryTools::TryReadMemory(processHandle, moduleBaseAddress + dosHeader.e_lfanew, &ntHeaders, sizeof(IMAGE_NT_HEADERS));

    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        throw gcnew MonopolyException("Invalid NT header magic");

    const DWORD exportDirectoryRva = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    IMAGE_EXPORT_DIRECTORY exportDirectory = { 0 };
    MemoryTools::TryReadMemory(processHandle, moduleBaseAddress + exportDirectoryRva, &exportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY));

    return exportDirectory;
}

DWORD_PTR MemoryTools::TryFindFunctionExportAddress(HANDLE processHandle, const DWORD_PTR moduleBaseAddress, const IMAGE_EXPORT_DIRECTORY& exportDirectory, LPCSTR name)
{
    const DWORD_PTR namesAddress = moduleBaseAddress + exportDirectory.AddressOfNames;
    const DWORD_PTR ordinalsAddress = moduleBaseAddress + exportDirectory.AddressOfNameOrdinals;
    const DWORD_PTR functionsAddress = moduleBaseAddress + exportDirectory.AddressOfFunctions;

    for (DWORD i = 0; i < exportDirectory.NumberOfNames; ++i)
    {
        DWORD nameRva;
        TryReadMemory(processHandle, namesAddress + (i * sizeof(DWORD)), &nameRva, sizeof(DWORD));

        std::string exportName;
        exportName.resize(strlen(name));
        TryReadMemory(processHandle, moduleBaseAddress + nameRva, &exportName[0], strlen(name) + 1);

        if (exportName == name)
        {
            WORD ordinal;
            TryReadMemory(processHandle, ordinalsAddress + (i * sizeof(WORD)), &ordinal, sizeof(WORD));

            DWORD exportRva;
            TryReadMemory(processHandle, functionsAddress + (ordinal * sizeof(DWORD)), &exportRva, sizeof(DWORD));

            return moduleBaseAddress + exportRva;
        }
    }

    throw gcnew MonopolyException("Could not find exported function in export table");
}

void MemoryTools::TryExecuteInNewThread(HANDLE processHandle, const DWORD_PTR address)
{
    HANDLE thread = CreateRemoteThread(processHandle, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(address), nullptr, 0, nullptr);
    if (!thread)
        throw gcnew MonopolyException("CreateRemoteThread failed");

    WaitForSingleObject(thread, INFINITE);

    CloseHandle(thread);
}
