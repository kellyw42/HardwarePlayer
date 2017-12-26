using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace PhotoFinish
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        eventhandler e1;
        timehandler t1;

        public MainWindow()
        {
            e1 = new eventhandler(event_handler);
            t1 = new timehandler(time_handler);
            NativeVideo.Init();
            InitializeComponent();
        }

        private void OpenFinish(object sender, RoutedEventArgs e)
        { 
            IntPtr player = NativeVideo.CreateVideoPlayer(e1, t1, 2);
            NativeVideo.OpenVideo(player, @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS");
        }

        private void OpenStart(object sender, RoutedEventArgs e)
        {
            IntPtr player = NativeVideo.CreateVideoPlayer(e1, t1, 1);
            NativeVideo.OpenVideo(player, @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Start-17-40-55.MTS");
        }

        private void time_handler(ulong start, ulong now)
        {
            time.Dispatcher.Invoke(new Action(() => { time.Text = start + " - " + now; }));
        }

        private void event_handler(int event_type)
        {
        }
    }
}
