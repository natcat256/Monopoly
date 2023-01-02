#pragma once

namespace Monopoly
{
	namespace MemoryTools
	{
		DWORD_PTR TryAllocateMemory(HANDLE processHandle, const SIZE_T size, const DWORD protectionFlags);
		void TryFreeMemory(HANDLE processHandle, const DWORD_PTR address);

		void TryReadMemory(HANDLE processHandle, const DWORD_PTR address, LPVOID data, const SIZE_T size);
		void TryWriteMemory(HANDLE processHandle, const DWORD_PTR address, LPCVOID data, const SIZE_T size);

		HMODULE TryGetMonoModule(HANDLE processHandle);
		IMAGE_EXPORT_DIRECTORY TryGetModuleExportTable(HANDLE processHandle, const DWORD_PTR moduleBaseAddress);
		DWORD_PTR TryFindFunctionExportAddress(HANDLE processHandle, const DWORD_PTR moduleBaseAddress, const IMAGE_EXPORT_DIRECTORY& exportDirectory, LPCSTR name);

		void TryExecuteInNewThread(HANDLE processHandle, const DWORD_PTR address);
	};
}
