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
using System.Windows.Shapes;
using System.IO;

namespace PhotoFinish.Views
{
    public partial class UploadVideos : Window
    {
        private ReportSync handler;
        public double c0, c1;
        public List<long> start_times = new List<long>();

        void DrawSync(int pos, long start_time, double c0, double c1)
        {
            if (pos < start_times.Count)
                start_times[pos] = start_time;
            else
                start_times.Add(start_time);

            this.c0 = c0;
            this.c1 = c1;

            var times = String.Join(", ", start_times.Select(time => (time/48000.0).ToString("0.00")));

            Dispatcher.Invoke(() => 
                this.Title = "start at " + times + " seconds, sync = " + (c1*1000000).ToString("F2") + " x + " + c0.ToString("F0"));

        }

        public UploadVideos(DateTime date)
        {
            this.handler = new ReportSync(DrawSync);

            InitializeComponent();

            NativeVideo.SyncAudio(handler);

            var start = new Upload("Start", date);
            var finish = new Upload("Finish", date);

            this.MyGrid.Children.Add(start);
            this.MyGrid.Children.Add(finish);

            Grid.SetColumn(start, 0);
            Grid.SetColumn(finish, 1);
        }
    }
}
