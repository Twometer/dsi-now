using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    [Flags]
    public enum NdsKeys
    {
        A = 1 << 0,
        B = 1 << 1,
        SELECT = 1 << 2,
        START = 1 << 3,
        RIGHT = 1 << 4,
        LEFT = 1 << 5,
        UP = 1 << 6,
        DOWN = 1 << 7,
        R = 1 << 8,
        L = 1 << 9,
        X = 1 << 10,
        Y = 1 << 11,
        TOUCH = 1 << 12,
        LID = 1 << 13,
    }
}
