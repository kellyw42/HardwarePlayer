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
        public MainWindow()
        {
            Console.WriteLine("InitializeComponent() ...");
            InitializeComponent();
            Console.WriteLine("NativeVideo.Init() ...");
            NativeVideo.Init();
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            Console.WriteLine("NativeVideo.CreateVideoWindow() ...");
            NativeVideo.CreateVideoWindow(event_handler, time_handler, @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS", 2);

            Console.WriteLine("NativeVideo.EventLoop() ...");
            NativeVideo.EventLoop();
        }

        private void time_handler(int start, int now)
        {
        }

        private void event_handler(int event_type)
        {
        }
    }
}
