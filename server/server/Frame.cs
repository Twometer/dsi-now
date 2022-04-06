using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    internal class Frame
    {
        private const int width = 256;
        private const int height = 120;

        public byte[] Data
        {
            get
            {
                Buffer.BlockCopy(drawBuffer, 0, sendBuffer, 0, drawBuffer.Length * 2);
                return sendBuffer;
            }
        }

        private byte[] sendBuffer = new byte[width * height * 2];

        private ushort[] drawBuffer = new ushort[width * height];

        private ushort EncodeRgb15(int r, int g, int b)
        {
            return (ushort)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) | (1 << 15));
        }

        public void PutPixel(int x, int y, int r, int g, int b)
        {
            ushort rgb15 = EncodeRgb15(r, g, b);
            drawBuffer[y * width + x] = rgb15;
        }

    }
}
