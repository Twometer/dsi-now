using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    internal class ToggleBoolean
    {

        private bool lastValue = false;

        public bool SetValue(bool t)
        {
            if (lastValue != t)
            {
                lastValue = t;
                return true;
            }

            return false;
        }

        public bool GetValue()
        {
            return lastValue;
        }

    }
}
