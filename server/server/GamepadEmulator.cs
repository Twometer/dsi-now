using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static server.Win32;

namespace server
{
    internal class GamepadEmulator
    {
      
        private ToggleBoolean wDown = new ToggleBoolean();
        private ToggleBoolean aDown = new ToggleBoolean();
        private ToggleBoolean sDown = new ToggleBoolean();
        private ToggleBoolean dDown = new ToggleBoolean();

        private ToggleBoolean spaceDown = new ToggleBoolean();

        private ToggleBoolean leftClick = new ToggleBoolean();
        private ToggleBoolean rightClick = new ToggleBoolean();


        public void Update(NdsKeys keys)
        {
            var inputs = new List<INPUT>();

            // WASD mapping
            if (wDown.SetValue(keys.HasFlag(NdsKeys.UP)))
            {
                inputs.Add(CreateKey(ScanCodeShort.KEY_W, wDown.GetValue()));
            }
            if (sDown.SetValue(keys.HasFlag(NdsKeys.DOWN)))
            {
                inputs.Add(CreateKey(ScanCodeShort.KEY_S, sDown.GetValue()));
            }
            if (aDown.SetValue(keys.HasFlag(NdsKeys.LEFT)))
            {
                inputs.Add(CreateKey(ScanCodeShort.KEY_A, aDown.GetValue()));
            }
            if (dDown.SetValue(keys.HasFlag(NdsKeys.RIGHT)))
            {
                inputs.Add(CreateKey(ScanCodeShort.KEY_D, dDown.GetValue()));
            }
            if (spaceDown.SetValue(keys.HasFlag(NdsKeys.TOUCH)))
            {
                inputs.Add(CreateKey(ScanCodeShort.SPACE, spaceDown.GetValue()));
            }


            // Left and right click
            if (leftClick.SetValue(keys.HasFlag(NdsKeys.L)))
            {
                inputs.Add(CreateButton(true, !leftClick.GetValue()));
            }
            if (rightClick.SetValue(keys.HasFlag(NdsKeys.R)))
            {
                inputs.Add(CreateButton(false, !rightClick.GetValue()));
            }

            // Mouse movement
            int dx = 0, dy = 0;
            if (keys.HasFlag(NdsKeys.X))
            {
                dy -= 15; 
            }
            if (keys.HasFlag(NdsKeys.Y))
            {
                dx -= 15;
            }
            if (keys.HasFlag(NdsKeys.A))
            {
                dx += 15;
            }
            if (keys.HasFlag(NdsKeys.B))
            {
                dy += 15;
            }

            inputs.Add(CreateMove(dx, dy));
            SendInput((uint)inputs.Count, inputs.ToArray(), INPUT.Size);
        }

    }
}
