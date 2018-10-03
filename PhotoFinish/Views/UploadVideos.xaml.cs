using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows;
using System.IO;
using System.ComponentModel;
using System.Windows.Input;
using System.Windows.Data;
using System.Collections.ObjectModel;
using Prism.Commands;

namespace PhotoFinish.Views
{
    public partial class UploadVideos : Window, INotifyPropertyChanged
    {
        const int SIZE = 1 << 25;

        private DriveInfo driveInfo;
        private DateTime date;
        private string streamFolder;
        private List<string> previously_uploaded = new List<string>();

        private long total_size, uploaded_size;
        private BlockingCollection<Block> free_buffer, write_buffer;
        private DateTime startTime;
        private CancellationTokenSource cancel;
        private Task reader = null, writer = null;
        private bool weird = true, autoClose = false;

        public ObservableCollection<VideoFile> Files { get; set; }
        public ObservableCollection<string> UploadedFiles { get; set; }

        public ICommand CancelCommand { get; set; }
        public ICommand UploadCommand { get; set; }

        private string rate;
        public string Rate
        {
            get
            {
                return rate;
            }
            set
            {
                rate = value;
                RaisePropertyChanged("Rate");
            }
        }

        private string remaining;
        public string Remaining
        {
            get
            {
                return remaining;
            }
            set
            {
                remaining = value;
                RaisePropertyChanged("Remaining");
            }
        }

        private string StartFinish
        {
            get
            {
                if (driveInfo.TotalSize > 20000000000)
                    return "Finish-";
                else
                    return "Start-";
            }
        }

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

        private int progress;
        public int Progress
        {
            get
            {
                return progress;
            }
            set
            {
                progress = value;
                RaisePropertyChanged("Progress");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void RaisePropertyChanged(string property)
        {
            if (this.PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(property));
        }

        public UploadVideos(DriveInfo driveInfo, string streamFolder, DateTime date)
        {
            this.driveInfo = driveInfo;
            this.date = date;
            this.streamFolder = streamFolder;

            CancelCommand = new DelegateCommand(Cancel);
            UploadCommand = new DelegateCommand<string>(UploadFilesNow);

            Files = new ObservableCollection<VideoFile>();
            UploadedFiles = new ObservableCollection<string>();

            BindingOperations.EnableCollectionSynchronization(Files, this);

            this.Title = driveInfo.ToString() + "\t" + StartFinish + "\t" + date.ToShortDateString();

            InitializeComponent();

            UpdateFileList();
            this.Loaded += UploadVideos_Loaded;
        }

        private void UploadVideos_Loaded(object sender, RoutedEventArgs e)
        {
            if (!weird)
            {
                // Auto start upload
                autoClose = true;
                UploadFilesNow(StartFinish);
            }
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

        private void UpdateFileList()
        {
            weird = false;

            UploadedFiles.Clear();

            foreach (var file in Directory.EnumerateFiles(directory(), "*.MTS"))
                UploadedFiles.Add(Path.GetFileName(file));

            Sort(UploadedFiles);

            previously_uploaded.Clear();

            string file0 = directory() + "uploaded.txt";
            if (File.Exists(file0))
                using (StreamReader reader = new StreamReader(file0))
                {
                    while (!reader.EndOfStream)
                        previously_uploaded.Add(reader.ReadLine());
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
                    var correct_date = file.CreationTime.Year == date.Year && file.CreationTime.Month == date.Month && file.CreationTime.Day == date.Day;
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

        void UpdateProgress()
        {
            var sofar = DateTime.Now - startTime;

            var rate = uploaded_size / sofar.TotalSeconds;
            Rate = string.Format("{0} MB of {1} MB, {2:f2} MB/sec", uploaded_size / 1000000, total_size / 1000000, rate / 1000000.0);

            var remaining_time = (int)((total_size - uploaded_size) / rate);
            Remaining = string.Format("{0} min {1} sec remaining", remaining_time / 60, remaining_time % 60);

            Progress = (int)(100 * uploaded_size / total_size);
        }

        void AppendToBuffer(FileStream input)
        {
            while (!cancel.IsCancellationRequested)
            {
                var block = free_buffer.Take();

                block.size = input.Read(block.buffer, 0, SIZE);

                UpdateProgress();

                if (block.size == 0)
                {
                    free_buffer.Add(block);
                    break;
                }

                write_buffer.Add(block);
            }
        }

        void WriteWorker(FileStream output)
        {
            while (!cancel.IsCancellationRequested && uploaded_size < total_size)
            {
                try
                {
                    foreach (var block in write_buffer.GetConsumingEnumerable(cancel.Token))
                    {
                        output.Write(block.buffer, 0, block.size);
                        uploaded_size += block.size;
                        free_buffer.Add(block);
                        UpdateProgress();

                        if (uploaded_size == total_size)
                            break;
                    }
                }
                catch (OperationCanceledException)
                {
                }
            }
        }

        void ReadWorker(string filename, List<FileInfo> files)
        {
            startTime = DateTime.Now;

            int i = 1;
            foreach (var file in files)
            {
                if (cancel.IsCancellationRequested)
                    break;

                using (FileStream input = File.Open(file.FullName, FileMode.Open))
                {
                    UpdateFile(file, "Uploading ...");
                    Status = String.Format("Uploading file {0} of {1} ...", i++, files.Count);

                    AppendToBuffer(input);

                    UpdateFile(file, "Uploaded");
                }
            }
        }

        void UpdateFile(FileInfo info, string status)
        {
            if (status == "Uploaded")
                using (var uploadedFiles = new StreamWriter(directory() + "uploaded.txt", true))
                    uploadedFiles.WriteLine(Record(info));

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
            if (cancel != null)
                cancel.Cancel();

            if (reader != null)
                await reader;

            if (writer != null)
                await writer;

            Status = "Operation Cancelled.";
        }

        async void UploadFilesNow(string which)
        {
            Status = "Upload in progress ...";
            Progress = 0;

            cancel = new CancellationTokenSource();

            uploaded_size = total_size = 0;

            var files = new List<FileInfo>();
            foreach (var file in Files)
                if (file.Upload)
                {
                    files.Add(file.info);
                    total_size += file.info.Length;
                }

            string filename = @"Track1-" + which + files[0].CreationTime.ToString("HH-mm-ss") + ".MTS";

            free_buffer = new BlockingCollection<Block>();
            write_buffer = new BlockingCollection<Block>();

            for (int i = 0; i < 4; i++)
                free_buffer.Add(new Block(SIZE));

            string new_filename = directory() + filename;

            using (var output = File.Open(new_filename, FileMode.Create))
            {
                UploadedFiles.Add(filename);
                Sort(UploadedFiles);

                reader = Task.Run(() => ReadWorker(filename, files));
                writer = Task.Run(() => WriteWorker(output));

                await reader;
                await writer;
            }

            Status = "Extracting audio ...";
            //NativeVideoInterface.ExtractAudio(new_filename);

            Status = "Upload complete";

            if (autoClose)
                Close();
        }
    }

    public class VideoFile: INotifyPropertyChanged
    {
        private string status;
        private bool upload;

        public string Status {
            get { return status; }
            set { status = value; OnPropertyChanged("Status"); }
        }
        public bool Upload {
            get { return upload;  }
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

    public struct Block
    {
        public byte[] buffer;
        public int size;

        public Block(int SIZE)
        {
            buffer = new byte[SIZE];
            size = 0;
        }
    };
}
