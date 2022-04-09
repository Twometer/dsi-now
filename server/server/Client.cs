using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace server
{
    internal class Client
    {
        private int id;
        private TcpClient tcp;
        private NetworkStream stream;

        public bool LoggedIn = false;
        public event EventHandler<EventArgs> ConnectionLost;
        public event EventHandler<uint> PacketReceived;

        public Client(int id, TcpClient tcp)
        {
            this.id = id;
            this.tcp = tcp;
            this.stream = tcp.GetStream();
        }

        private void Receive()
        {
            byte[] buffer = new byte[4];
            while (true)
            {
                try
                {
                    stream.Read(buffer, 0, buffer.Length);
                    var bufferStr = Encoding.UTF8.GetString(buffer);

                    if (!LoggedIn &&  bufferStr == "DVTP")
                    {
                        LoggedIn = true;
                    }
                    else
                    {
                        PacketReceived(this, BitConverter.ToUInt32(buffer, 0));
                    }

                }
                catch (Exception ex)
                {
                    Console.WriteLine($"error: client #{id}: receive failed: {ex.Message}");
                    ConnectionLost(this, null);
                    LoggedIn = false;
                    return;
                }
            }
        }

        public void BeginReceive()
        {
            new Thread(Receive).Start();
        }

        public void Send(byte[] data)
        {
            try
            {
                stream.Write(data, 0, data.Length);
                stream.Flush();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"error: client #{id}: send failed: {ex.Message}");
                ConnectionLost(this, null);
                LoggedIn = false;
            }
        }
    }
}
