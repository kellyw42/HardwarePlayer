using System.Collections.ObjectModel;

namespace PhotoFinish
{
    public class Lane
    {
        public int LaneNumber { get; set; }
        public ObservableCollection<Athlete> athletes { get; set; }
        public ObservableCollection<TimeStamp> finishTimes { get; set; }
    }
}
