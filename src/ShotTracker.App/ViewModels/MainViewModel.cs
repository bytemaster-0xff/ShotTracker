using LagoVista.Core.Commanding;
using ShotTracker.App.Models;
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

        public event EventHandler<Response> ResponseAvailable;

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
            var buffer = System.IO.File.ReadAllBytes("Content/UWPTarget01.jpg");
            RefreshTarget(buffer);            
        }

        public void RefreshTarget(byte[] buffer)
        {
            Task.Run(async () =>
            {
                var response = new Models.Response();

                short checkSum = 0;

                var client = new Services.ImagingServicesClient();
                await client.ConnectAsync("127.0.0.1", 27015);
                await client.SendAsync(SOH);

                var sizeBuffer = BitConverter.GetBytes(Convert.ToInt32(buffer.Length));

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

                var done = false;
                while(!done)
                {
                    var result = await client.Receive();
                    done = response.Parse(result);
                }

                ResponseAvailable(this, response);
               
                client.Close();
            });
        }       

        public RelayCommand RefreshTargetCommand { get; private set; }
    }
}
