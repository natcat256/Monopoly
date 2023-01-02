#include "pch.h"
#include "RemoteAssembler.h"
#include "MemoryTools.h"

using namespace Monopoly;

RemoteAssembler::RemoteAssembler(HANDLE processHandle, DWORD_PTR address)
    : m_processHandle(processHandle),
    m_address(address)
{
}

void RemoteAssembler::WriteInstruction(LPCVOID data, const SIZE_T size)
{
    MemoryTools::TryWriteMemory(m_processHandle, m_address, data, size);

    m_address += size;
}

void RemoteAssembler::MovRaxImmediate64(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x48, 0xB8 };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovRcxImmediate64(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x48, 0xB9 };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovRcxRax()
{
    constexpr SIZE_T instructionSize = 3;
    BYTE instructionBytes[instructionSize] = { 0x48, 0x8B, 0xC8 };

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovRdxImmediate64(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x48, 0xBA };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovR8Immediate64(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x49, 0xB8 };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovR9Immediate64(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x49, 0xB9 };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::MovOffsetRax(const DWORD_PTR value)
{
    constexpr SIZE_T instructionSize = 10;
    BYTE instructionBytes[instructionSize] = { 0x48, 0xA3 };
    *reinterpret_cast<DWORD_PTR*>(instructionBytes + 2) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::SubRspImmediate32(const DWORD value)
{
    constexpr SIZE_T instructionSize = 7;
    BYTE instructionBytes[instructionSize] = { 0x48, 0x81, 0xEC };
    *reinterpret_cast<DWORD*>(instructionBytes + 3) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void Monopoly::RemoteAssembler::AddRspImmediate32(const DWORD value)
{
    constexpr SIZE_T instructionSize = 7;
    BYTE instructionBytes[instructionSize] = { 0x48, 0x81, 0xC4 };
    *reinterpret_cast<DWORD*>(instructionBytes + 3) = value;

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::CallRaxNear()
{
    constexpr SIZE_T instructionSize = 2;
    BYTE instructionBytes[instructionSize] = { 0xFF, 0xD0 };

    WriteInstruction(instructionBytes, instructionSize);
}

void RemoteAssembler::RetNear()
{
    constexpr SIZE_T instructionSize = 1;
    BYTE instructionBytes[instructionSize] = { 0xC3 };

    WriteInstruction(instructionBytes, instructionSize);
}
