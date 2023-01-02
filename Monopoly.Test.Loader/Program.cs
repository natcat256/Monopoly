
namespace Monopoly.Test.Loader
{
    internal class Program
    {
        static void Main()
        {
            Injector injector = new Injector("Gang Beasts");
            injector.Inject("Monopoly.Test.ModAssembly.dll", "Monopoly.Test.ModAssembly", "InjectionEntry", "Initialize");
        }
    }
}
