using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;

namespace PhotoFinish
{
    public class Race : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        public Heat heat;
        public double video_c0, video_c1;
        public Meet meet;
        public string FinishFile;
        public bool IsSync = false;
        public TimeStamp StartTime { set; get; }
        public ObservableCollection<TimeStamp>[] finishTimes { get; private set; }
        public string TimeCount
        {
            get
            {
                var count = finishTimes.Sum(lane => lane.Count);
                return count + " times";
            }
        }

        public Race(Meet meet, long start_time, string start_file, string finish_file, bool sync, double c0, double c1)
        {
            finishTimes = new ObservableCollection<TimeStamp>[8];
            for (int lane = 0; lane < 8; lane++)
            {
                finishTimes[lane] = new ObservableCollection<TimeStamp>();
                finishTimes[lane].CollectionChanged += Race_CollectionChanged;
            }
            StartTime = new TimeStamp(meet, this, start_time, start_file);
            FinishFile = finish_file;
            IsSync = sync;
            video_c0 = c0;
            video_c1 = c1;
        }

        private void Race_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnPropertyRaised("finishTimes");
            OnPropertyRaised("TimeCount");
        }

        public Race(Meet meet, string line)
        {
            this.meet = meet;

            var parts = line.Split(',');
            var StartFile = meet.Directory + parts[1];
            var StartPts = long.Parse(parts[2]);
            StartTime = new TimeStamp(meet, this, StartPts, StartFile);

            var Distance = parts[3];
            IsSync = (Distance == "Sync");

            FinishFile = meet.Directory + parts[4];

            video_c0 = double.Parse(parts[5]);
            video_c1 = double.Parse(parts[6]);

            finishTimes = new ObservableCollection<TimeStamp>[8];
            for (int lane = 0; lane < 8; lane++)
            {
                finishTimes[lane] = new ObservableCollection<TimeStamp>();
                finishTimes[lane].CollectionChanged += Race_CollectionChanged;
                var list = parts[7 + lane];
                if (list.Length > 0)
                    foreach (var time in list.Split('.'))
                        finishTimes[lane].Add(new TimeStamp(meet, this, long.Parse(time), FinishFile));
            }
        }

        private long NearestFrame(long pts)
        {
            return (long)System.Math.Round((double)(pts / TimeStamp.PTS_PER_FRAME)) * TimeStamp.PTS_PER_FRAME;
        }

        public long CorrespondingFinishTime(long startPts)
        {
            return 93600 + (long)((startPts - 93600) * (1+video_c1) + video_c0);
        }

        public long CorrespondingStartTime(long finishPts)
        {
            return 93600 + (long)((finishPts - 93600 - video_c0) / (1 + video_c1));
        }

        private string Base(string fullpath)
        {
            return new FileInfo(fullpath).Name;
        }

        public void Save(StreamWriter writer)
        {
            var evnt = IsSync ? "Sync" : (heat != null ? heat.Distance : "");
            writer.Write("{6},{0},{1},{2},{3},{4},{5}",  
                Base(StartTime.filename), StartTime.pts, evnt, Base(FinishFile), video_c0, video_c1, 
                CorrespondingFinishTime(StartTime.pts)-StartTime.pts);

            foreach (var lane in finishTimes)
                writer.Write(",{0}", string.Join(".", lane.Select(timestamp => timestamp.pts)));

            writer.WriteLine();
        }

        public void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }
    }
}
