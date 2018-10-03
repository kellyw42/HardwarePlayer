using System.Collections.ObjectModel;
using System.ComponentModel;
using System;

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

        public void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }
    }
}
