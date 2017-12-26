using System.Collections.ObjectModel;

namespace PhotoFinish
{
    public class Race
    {
        TimeStamp startTime;
        public ObservableCollection<ObservableCollection<TimeStamp>> finishTimes { get; private set; }

        public Race()
        {
            finishTimes = new ObservableCollection<ObservableCollection<TimeStamp>>();
            for (int i = 0; i < 8; i++)
                finishTimes.Add(new ObservableCollection<TimeStamp>());
        }

        private string TimeCount
        {
            get
            {
                var count = 0;
                foreach (var lane in finishTimes)
                    count += lane.Count;

                return count + " times";
            }
        }

        //17:57:19.44, 6 times
        public string Summary
        {
            get
            {
                return startTime.ToTime() + ", " + TimeCount;
            }
        }

        public void SetStartTime(TimeStamp startTime)
        {
            this.startTime = startTime;
        }
  
        public void AddFinishTime(int lane, TimeStamp finishTime)
        {
            finishTimes[lane].Add(finishTime);
        }
    }
}
