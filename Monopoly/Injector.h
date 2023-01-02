#pragma once

#include "RemoteMonoWrapper.h"

using namespace System;
using namespace System::Diagnostics;

namespace Monopoly
{
	public ref class Injector
	{
	private:
		RemoteMonoWrapper^ m_mono;

		void Initialize(IntPtr processHandle);

	public:
		Injector(Process^ process);
		Injector(IntPtr processHandle);
		Injector(int processId);
		Injector(String^ processName);

		void Inject(array<byte>^ assemblyBytes, String^ __identifier(namespace), String^ __identifier(class), String^ method);
		void Inject(String^ assemblyPath, String^ __identifier(namespace), String^ __identifier(class), String^ method);
	};
}
