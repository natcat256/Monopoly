#pragma once

namespace Monopoly
{
	private class RemoteAssembler
	{
	private:
		HANDLE m_processHandle;
		DWORD_PTR m_address;

		void WriteInstruction(LPCVOID data, const SIZE_T size);

	public:
		RemoteAssembler(HANDLE processHandle, DWORD_PTR address);

		void MovRaxImmediate64(const DWORD_PTR value);
		void MovRcxImmediate64(const DWORD_PTR value);
		void MovRcxRax();
		void MovRdxImmediate64(const DWORD_PTR value);
		void MovR8Immediate64(const DWORD_PTR value);
		void MovR9Immediate64(const DWORD_PTR value);
		void MovOffsetRax(const DWORD_PTR value);
		void SubRspImmediate32(const DWORD value);
		void AddRspImmediate32(const DWORD value);
		void CallRaxNear();
		void RetNear();
	};
}

