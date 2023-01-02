#pragma once

using namespace System;

namespace Monopoly
{
	private ref class RemoteMonoWrapper
	{
	private:
		HANDLE m_processHandle;

		DWORD_PTR m_getRootDomain;
		DWORD_PTR m_threadAttach;
		DWORD_PTR m_assemblyLoadFrom;
		DWORD_PTR m_assemblyGetImage;
		DWORD_PTR m_imageOpenFromData;
		DWORD_PTR m_classFromName;
		DWORD_PTR m_classGetMethodFromName;
		DWORD_PTR m_runtimeInvoke;

	public:
		RemoteMonoWrapper(HANDLE processHandle, HMODULE monoModule);

		DWORD_PTR TryOpenImageFromData(array<byte>^ data);
		DWORD_PTR TryLoadAssemblyFromImage(DWORD_PTR monoImage);
		DWORD_PTR TryGetImageFromAssembly(DWORD_PTR monoAssembly);
		DWORD_PTR TryGetClassFromName(DWORD_PTR monoImage, String^ __identifier(namespace), String^ __identifier(class));
		DWORD_PTR TryGetNoParamClassMethodFromName(DWORD_PTR monoClass, String^ method);
		void TryInvokeNoParamStaticMethod(DWORD_PTR monoMethod);
	};
}

