using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShotTracker.App.Models
{
    public class Line
    {
        private int _byteCount;
        private int _readIndex;
        private byte[] _tmpBuffer = new byte[4];

        public Line()
        {
            Start = new Point();
            End = new Point();
        }

        public Point Start { get; set; }
        public Point End { get; set; }

        public bool HandleByte(byte ch)
        {
            _byteCount++;

            if(_byteCount > 0 && _byteCount % 4 == 0)
            {
                switch(_byteCount / 4)
                {
                    case 1: Start.X = BitConverter.ToInt32(_tmpBuffer, 0); _readIndex = 0; break;
                    case 2: Start.Y = BitConverter.ToInt32(_tmpBuffer, 0); _readIndex = 0; break;
                    case 3: End.X = BitConverter.ToInt32(_tmpBuffer, 0); _readIndex = 0; break;
                    case 4: End.Y = BitConverter.ToInt32(_tmpBuffer, 0); _readIndex = 0; break;
                }
            }
            else
            {
                _tmpBuffer[_readIndex++] = ch;
            }

            return (_byteCount == 16);
        }
    }
}
