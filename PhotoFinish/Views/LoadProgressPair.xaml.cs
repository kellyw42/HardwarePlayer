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

namespace PhotoFinish.Views
{
    /// <summary>
    /// Interaction logic for LoadProgressPair.xaml
    /// </summary>
    public partial class LoadProgressPair : Window
    {
        bool startDone = false, finishDone = false;
        public void StartProgressHandler(int thread, long completed, long total, string units)
        {
            start.ProgressHandler(thread, completed, total, units);
            if (units == "Done!")
            {
                startDone = true;
                if (finishDone)
                    Close();
            }
        }

        public void FinishProgressHandler(int thread, long completed, long total, string units)
        {
            finish.ProgressHandler(thread, completed, total, units);
            if (units == "Done!")
            {
                finishDone = true;
                if (startDone)
                    Close();
            }
        }

        public LoadProgressPair()
        {
            InitializeComponent();
        }
    }
}
