using LagoVista.Core.Commanding;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShotTracker.App.ViewModels
{
    public class MainViewModel : LagoVista.Core.ViewModels.ViewModelBase
    {
        
        public const byte SOH = 0x01;
        public const byte STX = 0x02;
        public const byte ETX = 0x03;
        public const byte EOT = 0x03;
        

        Services.ImagingServicesClient _client;

        public MainViewModel()
        {
            RefreshTargetCommand = new RelayCommand(RefreshTarget);

            _client = new Services.ImagingServicesClient();
        }

        public async override Task InitAsync()
        {
            await base.InitAsync();

            await _client.ConnectAsync("127.0.0.1", 27015);
        }

        void RefreshTarget()
        {
            Task.Run(() =>
            {
                short checkSum = 0;

                var buffer = System.IO.File.ReadAllBytes("Content/UWPTarget01.jpg");
                var sizeBuffer = BitConverter.GetBytes(Convert.ToInt32(buffer.Length));

                _client.SendAsync(SOH);

                //4 Bytes
                _client.SendAsync(sizeBuffer);
                _client.SendAsync(STX);

                _client.SendAsync(buffer);

                for(var idx = 0; idx < buffer.Length; ++idx)
                {
                    checkSum += buffer[idx];
                }

                _client.SendAsync(ETX);
                _client.SendAsync(BitConverter.GetBytes(checkSum));
                _client.SendAsync(EOT);


                var result = _client.Receive();
            });
        }

        public RelayCommand RefreshTargetCommand { get; private set; }
    }
}
