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

        public MainViewModel()
        {
            RefreshTargetCommand = new RelayCommand(RefreshTarget);

        }

        public async override Task InitAsync()
        {
            await base.InitAsync();

         }

        void RefreshTarget()
        {
            Task.Run(async () =>
            {
                short checkSum = 0;

                var buffer = System.IO.File.ReadAllBytes("Content/UWPTarget01.jpg");
                var sizeBuffer = BitConverter.GetBytes(Convert.ToInt32(buffer.Length));
                var _client = new Services.ImagingServicesClient();
                await _client.ConnectAsync("127.0.0.1", 27015);
                await _client.SendAsync(SOH);

                //4 Bytes
              /*  await _client.SendAsync(sizeBuffer);
                await _client.SendAsync(STX);

                await _client.SendAsync(buffer);

                for(var idx = 0; idx < buffer.Length; ++idx)
                {
                    checkSum += buffer[idx];
                }

                await _client.SendAsync(ETX);
                await _client.SendAsync(BitConverter.GetBytes(checkSum));
                await _client.SendAsync(EOT);*/


                var result = _client.Receive();
            });
        }

        public RelayCommand RefreshTargetCommand { get; private set; }
    }
}
