using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;

namespace PhotoFinish.Views
{
    public partial class AudioPanel : UserControl
    {
        const int quiet = 1100;
        const int quiet_period = 60;
        const int loud = 8000;
        const int loud_period = 200;

        private int origin = 0;
        public float[] audioData;

        public AudioPanel()
        { 
            Loaded += AudioPanel_Loaded;         
            InitializeComponent();
        }

        private void AudioPanel_Loaded(object sender, RoutedEventArgs e)
        {
            Refresh();
        }

        public void Refresh()
        {
            HorizontalLine.X1 = ActualWidth / 2;
            HorizontalLine.X2 = ActualWidth / 2;
            HorizontalLine.Y1 = 0;
            HorizontalLine.Y2 = ActualHeight - 1;

            VerticalLine.X1 = 0;
            VerticalLine.X2 = ActualWidth - 1;
            VerticalLine.Y1 = ActualHeight / 2;
            VerticalLine.Y2 = ActualHeight / 2;

            var quiet_height = ActualHeight * quiet / 32768;
            Canvas.SetLeft(Quiet, ActualWidth / 2 - quiet_period);
            Canvas.SetTop(Quiet, ActualHeight / 2 - quiet_height / 2);
            Quiet.Width = quiet_period;
            Quiet.Height = quiet_height;

            var load_height = ActualHeight * loud / 32768;
            Canvas.SetLeft(Loud, ActualWidth / 2);
            Canvas.SetTop(Loud, ActualHeight / 2 - load_height / 2);
            Loud.Width = loud_period;
            Loud.Height = load_height;

            Wave.Points.Clear();

            for (int xx = 0; xx < (int)ActualWidth * 2; xx++)
            {
                int i = origin + xx - (int)ActualWidth;
                if (0 <= i && i < 48000)
                {
                    int x = xx / 2;
                    int y = (int)ActualHeight / 2 + (int)(audioData[i] * ActualHeight * 0.5);
                    Wave.Points.Add(new Point(x, y));
                }
            }
        }

        private void AudioPanel_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Right)
            {
                if (origin + 100 < 48000)
                    origin += 100;
                Refresh();
            }
            else if (e.Key == Key.Left)
            {
                if (origin - 100 >= 0)
                    origin -= 100;
                Refresh();
            }
            else if (e.Key == Key.M)
            {
                if (origin + 1 < 48000)
                    origin += 1;
                Refresh();
            }
            else if (e.Key == Key.N)
            {
                if (origin - 1 >= 0)
                    origin -= 1;
                Refresh();
            }
            e.Handled = true;
        }

        private void UserControl_MouseDown(object sender, MouseButtonEventArgs e)
        {
            this.Focusable = true;
            Focus();
            Keyboard.Focus(this);
            e.Handled = true;
        }
    }
}
