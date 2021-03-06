﻿using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;

namespace PhotoFinish.Views
{
    /// <summary>
    /// Interaction logic for MainView.xaml
    /// </summary>
    public partial class MainView : Window
    {
        public MainView()
        {
            InitializeComponent();
        }

        Meet meet
        {
            get { return (Meet)DataContext;  }
        }

        protected override void OnPreviewKeyUp(KeyEventArgs e)
        {
            if (e.Key == Key.Right)
                meet.StopRepeatRight();
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.D1:
                case Key.D2:
                case Key.D3:
                case Key.D4:
                case Key.D5:
                case Key.D6:
                case Key.D7:
                case Key.D8:
                    meet.FinishLaneCommand.Execute(e.Key - Key.D0);
                    e.Handled = true;
                    return;
                case Key.Up:
                    meet.UpCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.Down:
                    meet.DownCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.Left:
                    meet.PrevFrameCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.Right:
                    meet.AutoRepeatRight();
                    e.Handled = true;
                    return;
                case Key.Space:
                    meet.StartCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.R:
                    meet.RewindCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.T:
                    meet.TestCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.F:
                    meet.FastForwardCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.B:
                    e.Handled = true;
                    meet.FindStartsCommand.Execute(null);
                    return;
                case Key.P:
                    meet.PlayPauseCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.C:
                    meet.ConfirmCommand.Execute(null);
                    e.Handled = true;
                    return;
                case Key.S:
                    meet.VisualSearchCommand.Execute(null);
                    e.Handled = true;
                    return;
                default:
                    base.OnPreviewKeyDown(e);
                    return;
            }
        }

        private void TimeSelected(object sender, MouseButtonEventArgs e)
        {
            var label = (Label)e.Source;
            var time = (TimeStamp)label.Tag;
            meet.GotoFinishTimeCommand.Execute(time);
        }

        private void ListViewItem_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            var item = sender as ListViewItem;
            if (item != null && item.IsSelected)
            {
                meet.RaceChanged();
            }
        }
    }
}
