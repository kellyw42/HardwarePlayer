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
    /// Interaction logic for LoadProgress.xaml
    /// </summary>
    public partial class LoadProgress : UserControl
    {
        public void ProgressHandler(int thread, long completed, long total, string units)
        {
            Dispatcher.Invoke(() =>
            {
                status.Content = (thread==0 ? "loading " : "initializing ") + completed + " of " + total + " (" + units +")";
                progressBar.Maximum = total;
                progressBar.Value = completed;
            });
        }

        public LoadProgress()
        {
            InitializeComponent();
        }
    }
}
