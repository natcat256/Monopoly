using UnityEngine;

namespace Monopoly.Test.ModAssembly
{
    public class InjectionEntry
    {
        public static void Initialize()
        {
            GameObject gameObject = new GameObject();
            gameObject.AddComponent<TestBehavior>();
            Object.DontDestroyOnLoad(gameObject);
        }
    }
}
