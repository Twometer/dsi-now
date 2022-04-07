using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    internal class ServerUDP : IServer
    {
        private readonly UdpClient client = new UdpClient();

        private readonly List<IPEndPoint> endpoints = new List<IPEndPoint>();

        public event EventHandler<string> PacketReceived;

        public void Broadcast(byte[] frame)
        {
            lock (endpoints)
            {
                foreach (var ep in endpoints)
                {
                    client.Send(frame, frame.Length, new IPEndPoint(ep.Address, 34221));
                    client.Send(frame, frame.Length, ep);
                }
            }
        }

        public void Start()
        {
            client.Client.Bind(new IPEndPoint(IPAddress.Any, 34221));
            client.Client.IOControl(-1744830452, new byte[] { 0 }, new byte[] { 0 }); // Ehm, no connection reset...
            BeginReceive();
        }

        private void BeginReceive()
        {
            client.BeginReceive(new AsyncCallback(ReceiveCallback), null);
        }

        private void ReceiveCallback(IAsyncResult result)
        {
            try
            {
                IPEndPoint sender = null;
                var data = client.EndReceive(result, ref sender);

                var dataStr = Encoding.UTF8.GetString(data);
                if (dataStr.StartsWith("LOGIN"))
                {
                    Console.WriteLine($"Login from {sender}");
                    lock (endpoints)
                    {
                        endpoints.Add(sender);
                    }
                }
                PacketReceived(sender, dataStr);

                BeginReceive();
            }
            catch (ObjectDisposedException e)
            {
            }
        }
    }
}
