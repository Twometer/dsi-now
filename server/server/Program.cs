using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace server
{
    internal class Program
    {
        private const int Framerate = 10;
        private static GamepadEmulator emu = new GamepadEmulator();

        private static NdsKeys currentKeys = 0;

        private static ImageCodecInfo GetEncoder(ImageFormat format)
        {
            ImageCodecInfo[] codecs = ImageCodecInfo.GetImageEncoders();
            foreach (ImageCodecInfo codec in codecs)
            {
                if (codec.FormatID == format.Guid)
                {
                    return codec;
                }
            }
            return null;
        }


        static void Main(string[] args)
        {
            Console.WriteLine("** DSi Now! Video Server **");

            var frame = new Frame();
            var server = new ServerTCP();
            server.PacketReceived += (sender, e) =>
            {
                currentKeys = (NdsKeys)e;
            };
            server.Start();

            var frameTime = (int)(1000.0 / Framerate);
            Console.WriteLine($"Starting frame broadcast at {Framerate}Hz ({frameTime}ms)");


            new Thread(UpdateThread).Start();

            ImageCodecInfo jpgEncoder = GetEncoder(ImageFormat.Jpeg);
            EncoderParameters encoderParams = new EncoderParameters(1);
            encoderParams.Param[0] = new EncoderParameter(Encoder.Quality, 50L);

            var random = new Random();
            var screen = Screen.PrimaryScreen;
            var bigBitmap = new Bitmap(screen.Bounds.Width, screen.Bounds.Height);
            var bitmap = new Bitmap(256, 144);
            var bigGraphics = Graphics.FromImage(bigBitmap);
            var graphics = Graphics.FromImage(bitmap);
            while (true)
            {
                var start = DateTime.Now;

                // Create screen buffer
                bigGraphics.CopyFromScreen(screen.Bounds.Top, screen.Bounds.Left, 0, 0, screen.Bounds.Size, CopyPixelOperation.SourceCopy);
                graphics.DrawImage(bigBitmap, new Rectangle(0, 0, 256, 144));

                // Encode to JPEG
                var jpegStr = new MemoryStream();
                bitmap.Save(jpegStr, jpgEncoder, encoderParams);

                // Build packet
                var packetStream = new MemoryStream();
                var packetBuf = new BinaryWriter(packetStream);
                packetBuf.Write(BitConverter.GetBytes((int)jpegStr.Length));
                packetBuf.Write(jpegStr.GetBuffer());

                // Send
                server.Broadcast(packetStream.GetBuffer());

                // Wait
                var end = DateTime.Now;
                var duration = (int)(end - start).TotalMilliseconds;
                Thread.Sleep(Math.Max(0, frameTime - duration));
            }
        }

        private static void UpdateThread()
        {
            Console.WriteLine("Input Thread Running");
            while (true) {
                emu.Update(currentKeys);
                Thread.Sleep(6);
            }
        }
    }
}
