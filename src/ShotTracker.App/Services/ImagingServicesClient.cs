using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Net.Sockets;
using System.Threading;
using System.Net;

namespace ShotTracker.App.Services
{
    public class ImagingServicesClient
    {
        Socket _socket = null;

        TaskCompletionSource<int> _sendTCS;
        TaskCompletionSource<byte[]> _recvTCS;

        //Memory is cheap, right?
        const int RECEIVE_BUFFER_SIZE = 2048 * 1024; /* 2MB */


        public Task<int> ConnectAsync(string hostName, int portNumber)
        {
            var tcs = new TaskCompletionSource<int>();
            Task.Run(() =>
            {
                var hostEntry = new DnsEndPoint(hostName, portNumber);

                _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

                var socketEventArg = new SocketAsyncEventArgs();
                socketEventArg.RemoteEndPoint = hostEntry;

                socketEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(delegate (object s, SocketAsyncEventArgs e)
                {
                    tcs.SetResult(-1);
                });

                _socket.ConnectAsync(socketEventArg);
            });


            return tcs.Task;
        }

        public Task<int> SendAsync(byte data)
        {
            _sendTCS = new TaskCompletionSource<int>();
            Task.Run(() =>
            {
                var socketEventArg = new SocketAsyncEventArgs();

                socketEventArg.RemoteEndPoint = _socket.RemoteEndPoint;
                socketEventArg.UserToken = null;

                socketEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(delegate (object s, SocketAsyncEventArgs e)
                {
                    lock (_socket)
                    {
                        if (_sendTCS != null)
                        {
                            var tcs = _sendTCS;
                            _sendTCS = null;
                            tcs.SetResult(0);
                        }
                    }
                });

                socketEventArg.SetBuffer(new byte[] { data }, 0, 1);
                _socket.SendAsync(socketEventArg);
            });

            return _sendTCS.Task;
        }

        public Task<int> SendAsync(byte[] data)
        {
            _sendTCS = new TaskCompletionSource<int>();
            Task.Run(() =>
            {
                var socketEventArg = new SocketAsyncEventArgs();

                socketEventArg.RemoteEndPoint = _socket.RemoteEndPoint;
                socketEventArg.UserToken = null;

                socketEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(delegate (object s, SocketAsyncEventArgs e)
                {
                    lock (_socket)
                    {
                        if (_sendTCS != null)
                        {
                            var tcs = _sendTCS;
                            _sendTCS = null;
                            tcs.SetResult(0);
                        }
                    }
                });

                socketEventArg.SetBuffer(data, 0, data.Length);
                _socket.SendAsync(socketEventArg);
            });

            return _sendTCS.Task;
        }


        public Task<byte[]> Receive()
        {
            _recvTCS = new TaskCompletionSource<byte[]>();
            Task.Run(() =>
            {
                var socketEventArg = new SocketAsyncEventArgs();
                socketEventArg.RemoteEndPoint = _socket.RemoteEndPoint;

                socketEventArg.SetBuffer(new Byte[RECEIVE_BUFFER_SIZE], 0, RECEIVE_BUFFER_SIZE);

                socketEventArg.Completed += new EventHandler<SocketAsyncEventArgs>(delegate (object s, SocketAsyncEventArgs e)
                {
                    lock (_socket)
                    {
                        if (_recvTCS != null)
                        {
                            if (e.SocketError == SocketError.Success)
                            {
                                _recvTCS.SetResult(e.Buffer);
                                _recvTCS = null;
                            }
                            else
                            {
                                _recvTCS.SetResult(null);
                            }
                        }
                    }
                });

                _socket.ReceiveAsync(socketEventArg);
            });

            return _recvTCS.Task;

        }

        /// <summary>
        /// Closes the Socket connection and releases all associated resources
        /// </summary>
        public void Close()
        {
            if (_socket != null)
            {
                lock (_socket)
                {
                    if (_recvTCS != null)
                    {
                        var txCancellationToken = new CancellationToken();
                        _recvTCS.TrySetCanceled(txCancellationToken);
                        _recvTCS = null;
                    }

                    if (_sendTCS != null)
                    {
                        var rxCancellationToken = new CancellationToken();
                        _sendTCS.TrySetCanceled(rxCancellationToken);
                        _sendTCS = null;
                    }
                }

                _socket.Dispose();
                _socket = null;
            }
        }
    }
}
