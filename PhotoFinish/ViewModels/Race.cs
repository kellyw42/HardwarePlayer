using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;

namespace PhotoFinish
{
    public class Race : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public TimeStamp StartTime { set; get; }
        public ObservableCollection<TimeStamp>[] finishTimes { get; private set; }

        public string TimeCount
        {
            get
            {
                var count = 0;
                foreach (var lane in finishTimes)
                    count += lane.Count;

                return count + " times";
            }
        }

        public Heat heat;
        public long sync;
        public Meet meet;
        public string FinishFile;
        public bool IsSync = false;

        public Race(Meet meet)
        {
            this.meet = meet;
            finishTimes = new ObservableCollection<TimeStamp>[8];
            for (int i = 0; i < 8; i++)
            {
                finishTimes[i] = new ObservableCollection<TimeStamp>();
            }
        }

        private string Basename(string fullpath)
        {
            return new FileInfo(fullpath).Name;
        }

        public void Save(StreamWriter writer)
        {
            var bang = 0;
            var audio_sync = 0;

            writer.Write("{0},{1},{2},{3},{4},{5},{6},{7}", Basename(StartTime.filename), StartTime.start, StartTime.pts, bang, (IsSync ? "Sync" : (heat != null ? heat.Distance : "")), Basename(FinishFile), sync, audio_sync);
            foreach (var lane in finishTimes)
            {
                writer.Write(',');
                bool firstInLane = true;
                foreach (var time in lane)
                {
                    if (firstInLane)
                        firstInLane = false;
                    else
                        writer.Write('.');
                    writer.Write("{0}|{1}", time.start, time.pts);
                }
            }
            writer.WriteLine();
        }

        public void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }
    }
}
