using Windows.UI.Xaml.Controls;
using System.Threading;
using System;
using Windows.UI.Xaml.Shapes;
using Windows.UI;
using Windows.UI.Xaml.Media.Imaging;
using System.Threading.Tasks;
using Windows.Graphics.Display;
using Windows.Storage.Streams;
using Windows.Graphics.Imaging;
using System.Runtime.InteropServices.WindowsRuntime;
using System.IO;
using Windows.UI.Xaml;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace ShotTracker.App
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        DispatcherTimer _refreshTimer;

        public MainPage()
        {
            this.InitializeComponent();
            Loaded += MainPage_Loaded;


        }

        private async void MainPage_Loaded(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            ViewModel.ResponseAvailable += Vm_ResponseAvailable;
            await ViewModel.InitAsync();

            _refreshTimer = new DispatcherTimer();
            _refreshTimer.Interval = TimeSpan.FromSeconds(1.5);
            _refreshTimer.Tick += _refreshTimer_Tick;
            _refreshTimer.Start();
        }

        private void _refreshTimer_Tick(object sender, object e)
        {
            Refresh();
        }

        private ViewModels.MainViewModel ViewModel
        {
            get { return DataContext as ViewModels.MainViewModel; }
        }

        async void Refresh()
        {
            var ms = new InMemoryRandomAccessStream();

            var renderTargetBitmap = new RenderTargetBitmap();
            await renderTargetBitmap.RenderAsync(OriginalImageContents);

            var pixels = await renderTargetBitmap.GetPixelsAsync();

            var logicalDpi = DisplayInformation.GetForCurrentView().LogicalDpi;
            var encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.JpegEncoderId, ms);
            encoder.SetPixelData(
            BitmapPixelFormat.Bgra8,
            BitmapAlphaMode.Ignore,
            (uint)renderTargetBitmap.PixelWidth,
            (uint)renderTargetBitmap.PixelHeight,
            logicalDpi,
            logicalDpi,
            pixels.ToArray());

            await encoder.FlushAsync();

            ms.Seek(0);

            var bytes = new Windows.Storage.Streams.Buffer((uint)ms.Size);

            await ms.ReadAsync(bytes, (uint)ms.Size, InputStreamOptions.None);
            var byteArray = bytes.ToArray();

            ViewModel.RefreshTarget(byteArray);
        }

        private async void Vm_ResponseAvailable(object sender, Models.Response e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                var brush = new Windows.UI.Xaml.Media.SolidColorBrush(Colors.Red);
                RenderTime.Text = "Processed in: " + e.ProcessingTime.TotalMilliseconds + " ms";
                ShotImage.Children.Clear();
                foreach (var line in e.Lines)
                {
                    var renderedLine = new Line()
                    {
                        X1 = line.Start.X,
                        Y1 = line.Start.Y,
                        X2 = line.End.X,
                        Y2 = line.End.Y,
                        Stroke = brush,
                        StrokeThickness = 2
                    };

                    ShotImage.Children.Add(renderedLine);
                }
            });
        }

        private void AppBarButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
           
        }

        private void OriginalImageContents_Tapped(object sender, Windows.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
            var location = new Windows.UI.Xaml.Thickness() { Left = e.GetPosition(sender as UIElement).X - 12.5, Top = e.GetPosition(sender as UIElement).Y - 12.5 };

            string url = "ms-appx:///Content/Shot.png";

            var img = new Image()
            {
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
                Width = 25,
                Height = 25,
                Margin = location,
            };
            img.Source = new BitmapImage(new Uri(url));

            OriginalImageContents.Children.Add(img);
        }
    }
}
