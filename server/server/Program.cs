using Ionic.Zlib;
using System;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace server
{
    internal class Program
    {
        private const int Framerate = 24;

        static void Main(string[] args)
        {
            Console.WriteLine("** DSi Now! Video Server **");

            var frame = new Frame();
            var server = new Server();
            server.PacketReceived += (sender, e) =>
            {
                Console.WriteLine($"Incoming packet:\n----------\n{e}\n----------");
            };
            server.Start();

            var frameTime = (int)(1000.0 / Framerate);
            Console.WriteLine($"Starting frame broadcast at {Framerate}Hz ({frameTime}ms)");

            var random = new Random();
            var screen = Screen.PrimaryScreen;
            var bigBitmap = new Bitmap(screen.Bounds.Width, screen.Bounds.Height);
            var bitmap = new Bitmap(256, 144);
            var bigGraphics = Graphics.FromImage(bigBitmap);
            var graphics = Graphics.FromImage(bitmap);
            while (true)
            {
                bigGraphics.CopyFromScreen(screen.Bounds.Top, screen.Bounds.Left, 0, 0, screen.Bounds.Size, CopyPixelOperation.SourceCopy);
                graphics.DrawImage(bigBitmap, new Rectangle(0, 0, 256, 144));
                // Generate random color image
                int r = random.Next();
                for (int y = 0; y < 144; y++)
                {
                    for (int x = 0; x < 256; x++)
                    {
                        var color = bitmap.GetPixel(x, y);
                        frame.PutPixel(x, y, color.R, color.G, color.B);
                    }
                }

                var str = new MemoryStream();
                var buf = new BinaryWriter(str);
                var compressed = ZlibStream.CompressBuffer(frame.Data);
                buf.Write(BitConverter.GetBytes(compressed.Length));
                buf.Write(compressed);

                // Send
                server.Broadcast(str.GetBuffer());

                // Wait
                Thread.Sleep(frameTime);
            }
        }
    }
}
