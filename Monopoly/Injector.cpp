#include "pch.h"
#include "Injector.h"
#include "MonopolyException.h"
#include "MemoryTools.h"

using namespace System::IO;
using namespace Monopoly;

#define CHECK_ARGUMENT_NULL(x) if (__identifier(x) == nullptr) throw gcnew ArgumentNullException(#x);

Injector::Injector(Process^ process)
{
    CHECK_ARGUMENT_NULL(process);

    Initialize(process->Handle);
}

Injector::Injector(IntPtr processHandle)
{
    Initialize(processHandle);
}

Injector::Injector(int processId)
{
    Process^ process = Process::GetProcessById(processId);
    if (process == nullptr)
        throw gcnew MonopolyException("Process does not exist");

    Initialize(process->Handle);
}

Injector::Injector(String^ processName)
{
    CHECK_ARGUMENT_NULL(processName);

    array<Process^>^ processes = Process::GetProcessesByName(processName);
    if (processes->Length < 1)
        throw gcnew MonopolyException("Process does not exist");

    Initialize(processes[0]->Handle);
}

void Injector::Initialize(IntPtr processHandle)
{
    if (processHandle == IntPtr::Zero)
        throw gcnew ArgumentNullException("processHandle");

    HANDLE ptr = processHandle.ToPointer();
    HMODULE monoModule = MemoryTools::TryGetMonoModule(ptr);

    m_mono = gcnew RemoteMonoWrapper(ptr, monoModule);
}

void Injector::Inject(String^ assemblyPath, String^ __identifier(namespace), String^ __identifier(class), String^ method)
{
    array<byte>^ assemblyBytes = File::ReadAllBytes(assemblyPath);
    Inject(assemblyBytes, __identifier(namespace), __identifier(class), method);
}

void Injector::Inject(array<byte>^ assemblyBytes, String^ __identifier(namespace), String^ __identifier(class), String^ method)
{
    CHECK_ARGUMENT_NULL(assemblyBytes);
    CHECK_ARGUMENT_NULL(namespace);
    CHECK_ARGUMENT_NULL(class);
    CHECK_ARGUMENT_NULL(method);

    DWORD_PTR image = m_mono->TryOpenImageFromData(assemblyBytes);
    DWORD_PTR assembly = m_mono->TryLoadAssemblyFromImage(image);
    DWORD_PTR assemblyImage = m_mono->TryGetImageFromAssembly(assembly);
    DWORD_PTR monoClass = m_mono->TryGetClassFromName(assemblyImage, __identifier(namespace), __identifier(class));
    DWORD_PTR monoMethod = m_mono->TryGetNoParamClassMethodFromName(monoClass, method);
    m_mono->TryInvokeNoParamStaticMethod(monoMethod);
}
