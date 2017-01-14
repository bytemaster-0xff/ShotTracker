using LagoVista.Core.Commanding;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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
        public const byte EOT = 0x04;

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
                var client = new Services.ImagingServicesClient();
                await client.ConnectAsync("127.0.0.1", 27015);
                await client.SendAsync(SOH);

                //4 Bytes
                await client.SendAsync(sizeBuffer);
                await client.SendAsync(STX);

                await client.SendAsync(buffer);

                for (var idx = 0; idx < buffer.Length; ++idx)
                {
                    checkSum += buffer[idx];
                }

                await client.SendAsync(ETX);
                await client.SendAsync(BitConverter.GetBytes(checkSum));
                await client.SendAsync(EOT);

                var result = await client.Receive();
                var contents = System.Text.ASCIIEncoding.ASCII.GetString(result);
                client.Close();
                Debug.WriteLine(contents);
            });
        }

        public RelayCommand RefreshTargetCommand { get; private set; }
    }
}
