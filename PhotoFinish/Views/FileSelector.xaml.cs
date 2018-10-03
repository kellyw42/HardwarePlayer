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
using Microsoft.Win32;
using System.IO;

namespace PhotoFinish.Views
{
    public partial class FileSelector : UserControl
    {
        public FileSelector()
        {
            InitializeComponent();
        }

        public string Folder { get; set; }

        private string pattern;

        public Action selectionChanged;

        public string Pattern
        {
            set
            {
                pattern = value;
                var files = Directory.EnumerateFiles(Folder, value);
                if (files.Any())
                {
                    filename.Text = files.First();
                    selectionChanged?.Invoke();
                }
            }
        }

        private void filePickerButton_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.InitialDirectory = Folder;
            dialog.FileName = filename.Text;
            dialog.Filter = "Video Files|" + pattern;
            if (dialog.ShowDialog() == true)
            {
                filename.Text = dialog.FileName;
                selectionChanged?.Invoke();
            }
        }
    }
}