using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    internal interface IServer
    {
        event EventHandler<string> PacketReceived;

        void Start();

        void Broadcast(byte[] frame);

    }
}
