using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShotTracker.App.Models
{
    public class Response
    {
        Line _currentLine;
        int _arraySize;
        int _readByteIndex;
        byte[] _tmpBuffer = new byte[4];
        public List<Line> Lines { get; private set; }
        public TimeSpan ProcessingTime { get; private set; }

        DateTime _start;

        public Response()
        {
            Lines = new List<Line>();

            _start = DateTime.Now;
        }

        enum ParsingResponseStates
        {
            ReadingSize,
            ReadingVector,
         }

        ParsingResponseStates _state = ParsingResponseStates.ReadingSize;

        public bool Parse(byte[] buffer)
        {
            for (var idx = 0; idx < buffer.Length; ++idx)
            {
                byte ch = buffer[idx];

                if (_state == ParsingResponseStates.ReadingSize)
                {
                    _tmpBuffer[_readByteIndex++] = ch;
                    if(_readByteIndex == 4)
                    {
                        _arraySize = BitConverter.ToInt32(_tmpBuffer,0);
                        _state = ParsingResponseStates.ReadingVector;
                    }
                }
                else
                {
                    if(_currentLine == null)
                    {
                        _currentLine = new Line();
                    }

                   if( _currentLine.HandleByte(ch))
                    {
                        Lines.Add(_currentLine);
                        _currentLine = null;
                        if(Lines.Count == _arraySize)
                        {
                            ProcessingTime = DateTime.Now - _start;
                            return true;
                        }
                    }
                }
            }

            return false;
        }
    }
}
