# Monopoly
 A library for injecting assemblies into mono-embedded 64-bit processes.
 
![It works!](https://user-images.githubusercontent.com/95653800/210285103-a2c94c09-e39f-4df5-82ca-e260ca94abfb.PNG)

## Usage
Monopoly provides an `Injector` class to facilitate assembly injection, with a constructor that has four overloads:
```cs
Injector(Process process);
Injector(IntPtr processHandle);
Injector(int processId);
Injector(string processName);
```
Any of these parameters should pertain to the target process to inject the assembly into.

The `Injector` class contains the method `Inject`, with two overloads:
```cs
Inject(byte[] assemblyBytes, string @namespace, string @class, string method);
Inject(string assemblyPath, string @namespace, string @class, string method);
```
With `assemblyPath` being the path of the assembly to inject, or ``assemblyBytes`` being a byte array with the assembly's file contents.

The assembly should have a static, zero-parameter method with return type ``void`` defined, like so:
```cs
namespace TheNamespace
{
    class TheClass
    {
        static void TheMethod() {}
    }
}
```
The ``@namespace``, ``@class``, and ``method`` parameters provided to the ``Inject`` method should pertain to the method defined in the assembly. This method will be invoked upon injection.

## Examples
Examples can be found in the ``Monopoly.Test.Loader`` and ``Monopoly.Test.ModAssembly`` projects.
