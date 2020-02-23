using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Prism.Commands;

namespace PhotoFinish.Views
{
    public partial class UploadVideos : Window
    {
        private ReportSync handler;
        public double video_c0, video_c1;
        public long start_bang;
        public Upload start, finish;

        void DrawSync(int done, long start_bang, double audio_c0, double audio_c1)
        {
            this.video_c0 = (audio_c0 / 48000.0) * TimeStamp.FRAMES_PER_SECOND * TimeStamp.PTS_PER_FRAME;
            this.video_c1 = audio_c1;
            this.start_bang = start_bang;

            Dispatcher.Invoke(() =>
            {
                this.Title = "Done = " + done + ", start_bang = " + start_bang + ", sync = " + (video_c1 * 1000000).ToString("F2") + " x + " + video_c0.ToString("F0");
                if (done == 2)
                    DialogResult = true;
            });
        }

        public ICommand UploadCommand { get; set; }

        void UploadFilesNow()
        {
            start.UploadCommand.Execute(null);
            finish.UploadCommand.Execute(null);
        }

        public UploadVideos(DateTime date)
        {
            this.UploadCommand = new DelegateCommand(UploadFilesNow);
            this.handler = new ReportSync(DrawSync);

            InitializeComponent();

            NativeVideo.SyncAudio(handler);

            start = new Upload("Start", date);
            finish = new Upload("Finish", date);

            this.MyGrid.Children.Add(start);
            this.MyGrid.Children.Add(finish);

            Grid.SetRow(start, 1);
            Grid.SetColumn(start, 0);
            Grid.SetRow(finish, 1);
            Grid.SetColumn(finish, 1);
        }
    }
}
