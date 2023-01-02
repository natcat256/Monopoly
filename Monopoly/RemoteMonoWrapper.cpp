#include "pch.h"
#include "RemoteMonoWrapper.h"
#include "RemoteAssembler.h"
#include "MonopolyException.h"
#include "MemoryTools.h"

using namespace System::Runtime::InteropServices;
using namespace Monopoly;

RemoteMonoWrapper::RemoteMonoWrapper(HANDLE processHandle, HMODULE monoModule)
    : m_processHandle(processHandle)
{
    const DWORD_PTR moduleBaseAddress = reinterpret_cast<DWORD_PTR>(monoModule);

    const IMAGE_EXPORT_DIRECTORY exportDirectory = MemoryTools::TryGetModuleExportTable(processHandle, moduleBaseAddress);

    m_getRootDomain = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_get_root_domain");
    m_threadAttach = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_thread_attach");
    m_imageOpenFromData = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_image_open_from_data");
    m_assemblyLoadFrom = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_assembly_load_from");
    m_assemblyGetImage = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_assembly_get_image");
    m_classFromName = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_class_from_name");
    m_classGetMethodFromName = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_class_get_method_from_name");
    m_runtimeInvoke = MemoryTools::TryFindFunctionExportAddress(processHandle, moduleBaseAddress, exportDirectory, "mono_runtime_invoke");
}

#define TRAMPOLINE_ASM_PROLOGUE() \
    assembler.SubRspImmediate32(40);               /* sub rsp, 40                          ; align the stack             */ \
    assembler.MovRaxImmediate64(m_getRootDomain);  /* mov rax, mono_get_root_domain        ; get the MonoDomain*         */ \
    assembler.CallRaxNear();                       /* call rax                                                           */ \
    assembler.MovRcxRax();                         /* mov rcx, rax                         ; pass the MonoDomain* as arg */ \
    assembler.MovRaxImmediate64(m_threadAttach);   /* mov rax, mono_thread_attach          ; attach the thread           */ \
    assembler.CallRaxNear();                       /* call rax                                                           */

#define TRAMPOLINE_ASM_EPILOGUE() \
    assembler.MovRaxImmediate64(EXIT_SUCCESS);     /* mov rax, EXIT_SUCCESS                ; set the thread exit code    */ \
    assembler.AddRspImmediate32(40);               /* add rsp, 40                          ; align the stack             */ \
    assembler.RetNear();                           /* ret                                                                */

constexpr SIZE_T TRAMPOLINE_SIZE = 128;

DWORD_PTR RemoteMonoWrapper::TryOpenImageFromData(array<byte>^ data)
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    pin_ptr<const void> dataNative = &data[0];
    const DWORD_PTR dataAddress = MemoryTools::TryAllocateMemory(m_processHandle, data->Length, PAGE_READWRITE);
    MemoryTools::TryWriteMemory(m_processHandle, dataAddress, dataNative, data->Length);

    int status = 0;
    const DWORD_PTR statusAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(int), PAGE_READWRITE);

    DWORD_PTR returnValue = 0;
    const DWORD_PTR returnValueAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(DWORD_PTR), PAGE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(dataAddress);          // mov rcx, dataAddress                 ; provide char* data
    assembler.MovRdxImmediate64(data->Length);         // mov rdx, dataLength                  ; provide guint32 data_len
    assembler.MovR8Immediate64(0);                     // mov r8, 0                            ; provide gboolean need_copy
    assembler.MovR9Immediate64(statusAddress);         // mov r9, statusAddress                ; provide MonoImageOpenStatus* status
    assembler.MovRaxImmediate64(m_imageOpenFromData);  // mov rax, mono_image_open_from_data   ; call the function
    assembler.CallRaxNear();                           // call rax
    assembler.MovOffsetRax(returnValueAddress);        // mov [returnValueAddress], rax        ; store the MonoImage* where we can grab it
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryReadMemory(m_processHandle, statusAddress, &status, sizeof(int));
    MemoryTools::TryReadMemory(m_processHandle, returnValueAddress, &returnValue, sizeof(DWORD_PTR));

    // the space at dataAddress does not need to be freed for it will remain in use, as determined by need_copy
    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
    MemoryTools::TryFreeMemory(m_processHandle, statusAddress);
    MemoryTools::TryFreeMemory(m_processHandle, returnValueAddress);

    if (!returnValue || status)
        throw gcnew MonopolyException("mono_image_open_from_data failed");

    return returnValue;
}

DWORD_PTR RemoteMonoWrapper::TryLoadAssemblyFromImage(DWORD_PTR monoImage)
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    const DWORD_PTR nameAddress = MemoryTools::TryAllocateMemory(m_processHandle, 1, PAGE_READWRITE); // this is apparently required

    int status = 0;
    const DWORD_PTR statusAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(int), PAGE_READWRITE);

    DWORD_PTR returnValue = 0;
    const DWORD_PTR returnValueAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(DWORD_PTR), PAGE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(monoImage);            // mov rcx, monoImage                ; provide MonoImage* image
    assembler.MovRdxImmediate64(nameAddress);          // mov rdx, nameAddress              ; provide const char* fname
    assembler.MovR8Immediate64(statusAddress);         // mov r8, statusAddress             ; provide MonoImageOpenStatus* status
    assembler.MovRaxImmediate64(m_assemblyLoadFrom);   // mov rax, mono_assembly_load_from  ; call the function
    assembler.CallRaxNear();                           // call rax
    assembler.MovOffsetRax(returnValueAddress);        // mov [returnValueAddress], rax     ; store the MonoAssembly* where we can grab it
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryReadMemory(m_processHandle, statusAddress, &status, sizeof(int));
    MemoryTools::TryReadMemory(m_processHandle, returnValueAddress, &returnValue, sizeof(DWORD_PTR));

    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
    MemoryTools::TryFreeMemory(m_processHandle, nameAddress);
    MemoryTools::TryFreeMemory(m_processHandle, statusAddress);
    MemoryTools::TryFreeMemory(m_processHandle, returnValueAddress);

    if (!returnValue || status)
        throw gcnew MonopolyException("mono_assembly_load_from failed");

    return returnValue;
}

DWORD_PTR RemoteMonoWrapper::TryGetImageFromAssembly(DWORD_PTR monoAssembly)
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    DWORD_PTR returnValue = 0;
    const DWORD_PTR returnValueAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(DWORD_PTR), PAGE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(monoAssembly);         // mov rcx, monoImage                ; provide MonoAssembly* assembly
    assembler.MovRaxImmediate64(m_assemblyGetImage);   // mov rax, mono_assembly_get_image  ; call the function
    assembler.CallRaxNear();                           // call rax
    assembler.MovOffsetRax(returnValueAddress);        // mov [returnValueAddress], rax     ; store the MonoImage* where we can grab it
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryReadMemory(m_processHandle, returnValueAddress, &returnValue, sizeof(DWORD_PTR));

    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
    MemoryTools::TryFreeMemory(m_processHandle, returnValueAddress);

    if (!returnValue)
        throw gcnew MonopolyException("mono_assembly_get_image failed");

    return returnValue;
}

DWORD_PTR RemoteMonoWrapper::TryGetClassFromName(DWORD_PTR monoImage, String^ __identifier(namespace), String^ __identifier(class))
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    LPCSTR namespaceNative = reinterpret_cast<LPCSTR>(Marshal::StringToHGlobalAnsi(__identifier(namespace)).ToPointer());
    const DWORD_PTR namespaceAddress = MemoryTools::TryAllocateMemory(m_processHandle, __identifier(namespace)->Length + 1, PAGE_READWRITE);
    MemoryTools::TryWriteMemory(m_processHandle, namespaceAddress, namespaceNative, __identifier(namespace)->Length + 1);

    LPCSTR classNative = reinterpret_cast<LPCSTR>(Marshal::StringToHGlobalAnsi(__identifier(class)).ToPointer());
    const DWORD_PTR classAddress = MemoryTools::TryAllocateMemory(m_processHandle, __identifier(class)->Length + 1, PAGE_READWRITE);
    MemoryTools::TryWriteMemory(m_processHandle, classAddress, classNative, __identifier(class)->Length + 1);

    DWORD_PTR returnValue = 0;
    const DWORD_PTR returnValueAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(DWORD_PTR), PAGE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(monoImage);            // mov rcx, monoImage               ; provide MonoImage* image
    assembler.MovRdxImmediate64(namespaceAddress);     // mov rdx, namespaceAddress        ; provide const char* name_space
    assembler.MovR8Immediate64(classAddress);          // mov r8, classAddress             ; provide const char* name
    assembler.MovRaxImmediate64(m_classFromName);      // mov rax, mono_class_from_name    ; call the function
    assembler.CallRaxNear();                           // call rax
    assembler.MovOffsetRax(returnValueAddress);        // mov [returnValueAddress], rax    ; store the MonoClass* where we can grab it
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryReadMemory(m_processHandle, returnValueAddress, &returnValue, sizeof(DWORD_PTR));

    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
    MemoryTools::TryFreeMemory(m_processHandle, namespaceAddress);
    MemoryTools::TryFreeMemory(m_processHandle, classAddress);
    MemoryTools::TryFreeMemory(m_processHandle, returnValueAddress);

    if (!returnValue)
        throw gcnew MonopolyException("mono_class_from_name failed");

    return returnValue;
}

DWORD_PTR RemoteMonoWrapper::TryGetNoParamClassMethodFromName(DWORD_PTR monoClass, String^ method)
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    LPCSTR methodNative = reinterpret_cast<LPCSTR>(Marshal::StringToHGlobalAnsi(method).ToPointer());
    const DWORD_PTR methodAddress = MemoryTools::TryAllocateMemory(m_processHandle, method->Length + 1, PAGE_READWRITE);
    MemoryTools::TryWriteMemory(m_processHandle, methodAddress, methodNative, method->Length + 1);

    DWORD_PTR returnValue = 0;
    const DWORD_PTR returnValueAddress = MemoryTools::TryAllocateMemory(m_processHandle, sizeof(DWORD_PTR), PAGE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(monoClass);                // mov rcx, monoClass                       ; provide MonoClass* klass
    assembler.MovRdxImmediate64(methodAddress);            // mov rdx, methodAddress                   ; provide const char* name
    assembler.MovR8Immediate64(0);                         // mov r8, 0                                ; provide int param_count
    assembler.MovRaxImmediate64(m_classGetMethodFromName); // mov rax, mono_class_get_method_from_name ; call the function
    assembler.CallRaxNear();                               // call rax
    assembler.MovOffsetRax(returnValueAddress);            // mov [returnValueAddress], rax            ; store the MonoMethod* where we can grab it
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryReadMemory(m_processHandle, returnValueAddress, &returnValue, sizeof(DWORD_PTR));

    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
    MemoryTools::TryFreeMemory(m_processHandle, methodAddress);
    MemoryTools::TryFreeMemory(m_processHandle, returnValueAddress);

    if (!returnValue)
        throw gcnew MonopolyException("mono_class_get_method_from_name failed");

    return returnValue;
}

void RemoteMonoWrapper::TryInvokeNoParamStaticMethod(DWORD_PTR monoMethod)
{
    const DWORD_PTR trampolineAddress = MemoryTools::TryAllocateMemory(m_processHandle, TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE);

    RemoteAssembler assembler(m_processHandle, trampolineAddress);
    TRAMPOLINE_ASM_PROLOGUE();
    assembler.MovRcxImmediate64(monoMethod);       // mov rcx, monoMethod          ; provide MonoMethod* method
    assembler.MovRdxImmediate64(0);                // mov rdx, 0                   ; provide void* obj
    assembler.MovR8Immediate64(0);                 // mov r8, 0                    ; provide void** params
    assembler.MovR9Immediate64(0);                 // mov r9, 0                    ; provide MonoObject** exc
    assembler.MovRaxImmediate64(m_runtimeInvoke);  // mov rax, mono_runtime_invoke ; call the function
    assembler.CallRaxNear();                       // call rax
    TRAMPOLINE_ASM_EPILOGUE();

    MemoryTools::TryExecuteInNewThread(m_processHandle, trampolineAddress);

    MemoryTools::TryFreeMemory(m_processHandle, trampolineAddress);
}
