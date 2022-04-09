using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace server
{
    internal class ServerTCP
    {
        private readonly TcpListener listener = new TcpListener(IPAddress.Any, 34221);
        private readonly IDictionary<int, Client> clients = new ConcurrentDictionary<int, Client>();
        public event EventHandler<uint> PacketReceived;

        private void AcceptClients()
        {
            Console.WriteLine("Accepting clients on port 34221");
            int idCounter = 0;
            while (true)
            {
                var tcpClient = listener.AcceptTcpClient();

                var clientId = ++idCounter;
                var client = new Client(clientId, tcpClient);
                Console.WriteLine($"New connection from {tcpClient.Client.RemoteEndPoint} as client #{clientId}");

                client.BeginReceive();
                client.ConnectionLost += (sender, e) =>
                {
                    Console.WriteLine($"Client #{clientId} lost connection");
                    clients.Remove(clientId);
                };
                client.PacketReceived += PacketReceived;
                clients[clientId] = client;
            }
        }

        public void Start()
        {
            Console.WriteLine("Starting TCP listener.");
            listener.Start();
            new Thread(AcceptClients).Start();
        }

        public void Broadcast(byte[] data)
        {
            foreach (var c in clients)
            {
                if (!c.Value.LoggedIn) continue;
                c.Value.Send(data);
            }
        }

    }
}
