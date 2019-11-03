using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.IO;
using System.Collections.Concurrent;
using System.Threading;
using System.ComponentModel;
using System.Collections.ObjectModel;
using Prism.Commands;

namespace PhotoFinish.Views
{
    public partial class Upload : UserControl, INotifyPropertyChanged
    {
        private string name;

        public Upload(string name, DateTime date)
        {
            ProgressHandler = new progresshandler(updateprogress);

            this.date = date;
            this.name = name;
            CancelCommand = new DelegateCommand(Cancel);
            UploadCommand = new DelegateCommand(UploadFilesNow);
            Files = new ObservableCollection<VideoFile>();
            UploadedFiles = new ObservableCollection<string>();

            BindingOperations.EnableCollectionSynchronization(Files, this);

            foreach (var drive in DriveInfo.GetDrives())
            {
                string StreamFolder = drive.RootDirectory + @"PRIVATE\AVCHD\BDMV\STREAM\";
                if (drive.IsReady && drive.DriveType == DriveType.Removable && System.IO.Directory.Exists(StreamFolder))
                {
                    if (this.name == "Start" && drive.TotalSize < 20000000000)
                    {
                        this.driveInfo = drive;
                        this.streamFolder = StreamFolder;
                    }
                    if (this.name == "Finish" && drive.TotalSize > 20000000000)
                    {
                        this.driveInfo = drive;
                        this.streamFolder = StreamFolder;
                    }
                }
            }

            InitializeComponent();

            if (driveInfo != null)
            {
                this.Title.Content = name + "\t(" + driveInfo.Name + ")";

                UpdateFileList();
                this.Loaded += UploadVideos_Loaded;
            }
        }

        private DriveInfo driveInfo;
        public DateTime date;
        private string streamFolder;
        private List<string> previously_uploaded = new List<string>();

        private bool weird = true;

        public ObservableCollection<VideoFile> Files { get; set; }
        public ObservableCollection<string> UploadedFiles { get; set; }

        public ICommand CancelCommand { get; set; }
        public ICommand UploadCommand { get; set; }

        private string status;
        public string Status
        {
            get
            {
                return status;
            }
            set
            {
                status = value;
                RaisePropertyChanged("Status");
            }
        }


        private string remaining0;
        public string Remaining0
        {
            get
            {
                return remaining0;
            }
            set
            {
                remaining0 = value;
                RaisePropertyChanged("Remaining0");
            }
        }
        private string remaining1;
        public string Remaining1
        {
            get
            {
                return remaining1;
            }
            set
            {
                remaining1 = value;
                RaisePropertyChanged("Remaining1");
            }
        }
        private string remaining2;
        public string Remaining2
        {
            get
            {
                return remaining2;
            }
            set
            {
                remaining2 = value;
                RaisePropertyChanged("Remaining2");
            }
        }

        private string remaining3;
        public string Remaining3
        {
            get
            {
                return remaining3;
            }
            set
            {
                remaining3 = value;
                RaisePropertyChanged("Remaining3");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void RaisePropertyChanged(string property)
        {
            if (this.PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(property));
        }

        private void UploadVideos_Loaded(object sender, RoutedEventArgs e)
        {
            if (!weird)
                UploadFilesNow();
        }

        public string directory()
        {
            return @"C:\PhotoFinish\Meets2\" + date.ToString("yyyy-MM-dd") + @"\";
        }

        public static List<string> ParsePlayList(string Drive)
        {
            var list = new List<string>();
            if (!File.Exists(Drive + @"PRIVATE\AVCHD\BDMV\PLAYLIST\00000.MPL"))
                return list;

            var buffer = new byte[1024];
            var bytes = File.ReadAllBytes(Drive + @"PRIVATE\AVCHD\BDMV\PLAYLIST\00000.MPL");
            int count = (int)bytes[65];
            for (int i = 0; i < count; i++)
            {
                int start = 68 + 98 * i;
                string name = Encoding.ASCII.GetString(bytes, start + 2, 9);
                name = name.Replace("M2TS", ".MTS");
                var type = bytes[start + 12];
                if (type != 6) // continuation
                    list.Add(name);
            }
            return list;
        }

        private string Record(FileInfo info)
        {
            return info.Name + " " + info.CreationTime.ToString();
        }

        public static void Sort<T>(ObservableCollection<T> collection)
        {
            var sortableList = new List<T>(collection);
            sortableList.Sort();

            for (int i = 0; i < sortableList.Count; i++)
                collection.Move(collection.IndexOf(sortableList[i]), i);
        }

        static object uploadedFileLock = new object();

        private void UpdateFileList()
        {
            weird = false;

            UploadedFiles.Clear();

            foreach (var file in Directory.EnumerateFiles(directory(), "*.MTS"))
                UploadedFiles.Add(Path.GetFileName(file));

            Sort(UploadedFiles);

            previously_uploaded.Clear();

            lock (uploadedFileLock)
            {
                string file0 = directory() + "uploaded.txt";
                if (File.Exists(file0))
                    using (StreamReader reader = new StreamReader(file0))
                    {
                        while (!reader.EndOfStream)
                            previously_uploaded.Add(reader.ReadLine());
                    }
            }

            Files.Clear();

            var MTSFiles = new List<FileInfo>();
            if (Directory.Exists(streamFolder))
            {

                foreach (var file in Directory.GetFiles(streamFolder, "*.MTS"))
                {
                    FileInfo original = new FileInfo(file);
                    MTSFiles.Add(original);
                }

                weird = MTSFiles.Count == 0;

                var playlist = ParsePlayList(driveInfo.RootDirectory.FullName);

                if (playlist.Count != 1)
                    weird = true;

                var sortedNewVideos = from file in MTSFiles orderby file.CreationTime ascending select file;

                int part = 1;
                foreach (var file in sortedNewVideos)
                {
                    if (playlist.Contains(file.Name))
                        part = 1;
                    else
                        part++;
                    var uploaded = previously_uploaded.Contains(Record(file));
                    var correct_date = true; // file.CreationTime.Year == date.Year && file.CreationTime.Month == date.Month && file.CreationTime.Day == date.Day;
                    var select = !uploaded && correct_date && !weird;
                    var status = uploaded ? "Uploaded" : !correct_date ? "Wrong Date" : "";
                    if (status != "")
                        weird = true;

                    Files.Add(new VideoFile { Status = status, Upload = select, Part = part, Filename = file.Name, Size = file.Length, Date = file.CreationTime.ToString(), info = file });
                }
            }
        }

        private void Refresh(object sender, RoutedEventArgs e)
        {
            UpdateFileList();
        }

        void UpdateFile(FileInfo info, string status)
        {
            lock (uploadedFileLock)
            {
                if (status == "Uploaded")
                    using (var uploadedFiles = new StreamWriter(directory() + "uploaded.txt", true))
                        uploadedFiles.WriteLine(Record(info));
            }

            foreach (var f in Files)
                if (f.info == info)
                {
                    f.Upload = false;
                    f.Status = status;
                }
        }

        async private void Cancel()
        {
            Status = "Cancel in progress ..";
            Status = "Operation Cancelled.";
        }

        private progresshandler ProgressHandler;

        void updateprogress(int thread, long completed, long total, string units)
        {
                Dispatcher.Invoke(() =>
                {
                    if (thread == 0)
                    {
                        Remaining0 = completed + " of " + total + " " + units;
                        ProgressBar0.Maximum = total;
                        ProgressBar0.Value = completed;
                    }
                    else if (thread == 1)
                    {
                        Remaining1 = completed + " of " + total + " " + units;
                        ProgressBar1.Maximum = total;
                        ProgressBar1.Value = completed;
                    }
                    else if (thread == 2)
                    {
                        Remaining2 = completed + " of " + total + " " + units;
                        ProgressBar2.Maximum = total;
                        ProgressBar2.Value = completed;
                    }
                    else if (thread == 3)
                    {
                        Remaining3 = completed + " of " + total + " " + units;
                        ProgressBar3.Maximum = total;
                        ProgressBar3.Value = completed;
                    }
                });
        }

        void UploadFilesNow()
        {
            Status = "Upload in progress ...";

            Task.Run(() =>
            {
                var files = new List<string>();

                string filename = null;

                foreach (var file in Files)
                    if (file.Upload)
                    {
                        if (filename == null)
                            filename = @"Track1-" + name + "-" + file.info.CreationTime.ToString("HH-mm-ss") + ".video";
                        files.Add(file.info.FullName);
                    }

                UploadedFiles.Add(filename);
                Sort(UploadedFiles);

                var which = (name == "Start") ? 0 : 1;

                NativeVideo.OpenCardVideo(directory() + filename, directory(), which, files.ToArray(), files.Count, (1UL << 31), ProgressHandler);
            });
        }
    }

    public class VideoFile : INotifyPropertyChanged
    {
        private string status;
        private bool upload;

        public string Status
        {
            get { return status; }
            set { status = value; OnPropertyChanged("Status"); }
        }
        public bool Upload
        {
            get { return upload; }
            set { upload = value; OnPropertyChanged("Upload"); }
        }
        public int Part { get; set; }
        public string Filename { get; set; }
        public long Size { get; set; }
        public string Date { get; set; }
        public FileInfo info { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(string property)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(property));
        }
    }
}
