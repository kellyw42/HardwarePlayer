using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Shapes;
using Prism.Commands;

namespace PhotoFinish.Views
{
    public partial class UploadVideos : Window
    {
        private ReportSync handler;
        private BangHandler bang_handler;
        public double video_c0, video_c1;
        public long start_bang;
        public Upload start, finish;
        private List<long> start_bangs = new List<long>();
        private List<long> finish_bangs = new List<long>();

        void DrawSync(int done, long start_bang, double audio_c0, double audio_c1)
        {
            this.video_c0 = (audio_c0 / 48000.0) * TimeStamp.FRAMES_PER_SECOND * TimeStamp.PTS_PER_FRAME;
            this.video_c1 = audio_c1;
            this.start_bang = start_bang;

            Dispatcher.Invoke(() =>
            {
                this.Title = "Done = " + done + ", start_bang = " + start_bang/48000.0 + ", sync = " + (video_c1 * 1000000).ToString("F2") + " x + " + video_c0.ToString("F0");
                if (done == 2)
                {
                    DialogResult = true;
                }
            });
        }

        long init_diff;

        void Plot(int last)
        {
            var last_start = start_bangs[last];
            var last_finish = finish_bangs[last];

            if (last == 0)
                init_diff = last_finish - last_start;

            var diff = ((last_finish - last_start) - init_diff) / 10000.0;
            var x = plot.ActualWidth * (last_start - 93600) / 486000000.0;
            var y = plot.ActualHeight * (1.0 - diff);
            Dispatcher.Invoke(() =>
            {
                var dot = new Ellipse();
                dot.Fill = Brushes.Blue;
                dot.Width = dot.Height = 1;
                plot.Children.Add(dot);
                Canvas.SetLeft(dot, x);
                Canvas.SetTop(dot, y);
                UpdateLayout();
            });
        }

        void BangHandler(int which, long bang_time)
        {
            if (which == 0)
            {
                start_bangs.Add(bang_time);
                if (start_bangs.Count <= finish_bangs.Count)
                    Plot(start_bangs.Count - 1);
            }
            else
            {
                finish_bangs.Add(bang_time);
                if (finish_bangs.Count <= start_bangs.Count)
                    Plot(finish_bangs.Count - 1);
            }
        }

        public ICommand UploadCommand { get; set; }

        void UploadFilesNow()
        {
            this.Title = "Uploading ...";
            start.UploadCommand.Execute(null);
            finish.UploadCommand.Execute(null);
        }

        bool autoStart = false;

        public UploadVideos(DateTime date, bool autoStart)
        {
            this.autoStart = autoStart;

            this.UploadCommand = new DelegateCommand(UploadFilesNow);
            this.handler = new ReportSync(DrawSync);
            this.bang_handler = new BangHandler(BangHandler);

            InitializeComponent();

            NativeVideo.SyncAudio(handler, bang_handler);

            start = new Upload("Start", date);
            finish = new Upload("Finish", date);

            this.MyGrid.Children.Add(start);
            this.MyGrid.Children.Add(finish);

            Grid.SetRow(start, 1);
            Grid.SetColumn(start, 0);
            Grid.SetRow(finish, 1);
            Grid.SetColumn(finish, 1);

            if (autoStart)
                this.Loaded += UploadVideos_Loaded;
        }

        private void UploadVideos_Loaded(object sender, RoutedEventArgs e)
        {
            UploadFilesNow();
        }
    }
}
