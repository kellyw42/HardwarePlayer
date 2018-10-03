using System.Collections.ObjectModel;
using System.Windows.Input;
using Prism.Commands;

namespace PhotoFinish
{
    public class Lane
    {
        public int LaneNumber { get; set; }

        private ObservableCollection<Athlete> _athletes;
        public ObservableCollection<Athlete> athletes
        {
            get { return _athletes; }
            set
            {
                _athletes = value;
                _athletes.CollectionChanged += update;
            }
        }

        private void update(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            foreach (var athlete in athletes)
                athlete.Update();
            foreach (var finishTime in finishTimes)
                finishTime.Update();
        }

        private ObservableCollection<TimeStamp> _finishTimes;
        public ObservableCollection<TimeStamp> finishTimes
        {
            get
            {
                return _finishTimes;
            }
            set
            {
                _finishTimes = value;
                _finishTimes.CollectionChanged += update;
            }
        }

        public ICommand RemoveAthleteCommand { get; set; }
        public ICommand RemoveFinishTimeCommand { get; set; }

        public Lane()
        {
            RemoveAthleteCommand = new DelegateCommand<Athlete>(RemoveAthlete);
            RemoveFinishTimeCommand = new DelegateCommand<TimeStamp>(RemoveFinishTime);
        }

        private void RemoveAthlete(Athlete athlete)
        {
            athletes.Remove(athlete);
        }

        private void RemoveFinishTime(TimeStamp timeStamp)
        {
            finishTimes.Remove(timeStamp);
        }
    }
}
