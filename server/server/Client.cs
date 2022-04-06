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
        private StreamReader reader;

        public bool LoggedIn = false;
        public event EventHandler<EventArgs> ConnectionLost;
        public event EventHandler<string> PacketReceived;

        public Client(int id, TcpClient tcp)
        {
            this.id = id;
            this.tcp = tcp;
            this.stream = tcp.GetStream();
            this.reader = new StreamReader(stream);
        }

        private void Receive()
        {
            var builder = new StringBuilder();
            while (true)
            {
                try
                {
                    var line = reader.ReadLine();
                    if (line == null) { throw new IOException("Connection closed: EOF"); }
                    if (line.Length == 0)
                    {
                        var message = builder.ToString();
                        if (message.StartsWith("LOGIN DVTP/0.1"))
                        {
                            Console.WriteLine($"Client #{id} logged in successfully");
                            LoggedIn = true;
                        }
                        PacketReceived(this, message);
                        builder.Clear();
                    }
                    else
                    {
                        builder.AppendLine(line);
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
