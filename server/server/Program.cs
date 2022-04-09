using Ionic.Zlib;
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using ZstdNet;

namespace server
{
    internal class Program
    {
        private const int Framerate = 12;

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
                Console.WriteLine($"Incoming packet:\n----------\n{e}\n----------");
            };
            server.Start();

            var frameTime = (int)(1000.0 / Framerate);
            Console.WriteLine($"Starting frame broadcast at {Framerate}Hz ({frameTime}ms)");

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

                /*
                // == ZLIB ==
                var str = new MemoryStream();
                var buf = new BinaryWriter(str);
                var compressed = ZlibStream.CompressBuffer(frame.Data);
                buf.Write(BitConverter.GetBytes(compressed.Length));
                buf.Write(compressed);*/

                // == JPEG ==
                var jpegStr = new MemoryStream();
                bitmap.Save(jpegStr, jpgEncoder, encoderParams);
                //bitmap.Save("test.jpeg", jpgEncoder, encoderParams);
                var str = new MemoryStream();
                var buf = new BinaryWriter(str);
                buf.Write(BitConverter.GetBytes((int)jpegStr.Length));
                buf.Write(jpegStr.GetBuffer());



                server.Broadcast(str.GetBuffer());

                var end = DateTime.Now;
                var duration = (int)(end - start).TotalMilliseconds;

                // Wait
                Thread.Sleep(Math.Max(0, frameTime - duration));
            }
        }
    }
}
