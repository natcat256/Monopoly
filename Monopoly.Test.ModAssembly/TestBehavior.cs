using UnityEngine;

namespace Monopoly.Test.ModAssembly
{
    internal class TestBehavior : MonoBehaviour
    {
        void OnGUI()
        {
            GUIStyle style = new GUIStyle();
            style.fontSize = 24;
            style.normal.textColor = Color.white;

            GUI.Label(new Rect(0, 0, 100, 100), "nat was here", style);
        }
    }
}
