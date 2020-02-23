using System.Collections.ObjectModel;
using System.ComponentModel;

namespace PhotoFinish
{
    public class Entry : INotifyPropertyChanged
    { 
        private Race race;

        public Race Race
        {
            get { return race; }
            set
            {
                race = value;
                if (race != null)
                    race.PropertyChanged += Race_PropertyChanged;
                OnPropertyChanged("Race");
                OnPropertyChanged("Lanes");
                OnPropertyChanged("Distance");
                OnPropertyChanged("Brush");
            }
        }

        private void Race_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnPropertyChanged("Race");
            OnPropertyChanged("Lanes");
            OnPropertyChanged("Distance");
            OnPropertyChanged("Brush");
        }

        private Heat heat;

        public Heat Heat
        {
            get { return heat; }
            set
            {
                heat = value;
                if (heat != null)
                    heat.PropertyChanged += Heat_PropertyChanged;
                OnPropertyChanged("Heat");
                OnPropertyChanged("Lanes");
                OnPropertyChanged("Distance");
                OnPropertyChanged("Brush");
            }
        }

        private void Heat_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnPropertyChanged("Race");
            OnPropertyChanged("Lanes");
            OnPropertyChanged("Distance");
            OnPropertyChanged("Brush");
        }

        public System.Windows.Media.Brush Brush
        {
            get
            {
                if (heat != null && race != null)
                {
                    bool good = true;
                    for (int lane = 0; lane < race.finishTimes.Length; lane++)
                    {
                        var times = race.finishTimes[lane];
                        var athletes = heat.athletes[lane];
                        if (times.Count != athletes.Count)
                            good = false;
                    }
                    if (good)
                        return System.Windows.Media.Brushes.LightBlue;
                }

                return System.Windows.Media.Brushes.Transparent;
            }
        }

        public string Distance
        {
            get
            {
                if (heat != null)
                    return heat.Distance;
                else if (race != null && race.IsSync)
                    return "Sync";
                else
                    return "";
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public ObservableCollection<Lane> Lanes
        {
            get
            {
                if (race == null || race.IsSync)
                    return null;

                var inner = heat.athletes[0].Count;
                var outer = 0;
                for (int lane = 1; lane < 8; lane++)
                    outer += heat.athletes[lane].Count;

                int NumberOfLanes = 8;
                if (outer == 0 && inner > 2)
                    NumberOfLanes = 1;

                ObservableCollection<Lane> results = new ObservableCollection<Lane>();
                for (int lane = 0; lane < NumberOfLanes; lane++)
                    results.Add(new Lane { LaneNumber = lane + 1, finishTimes = race.finishTimes[lane], athletes = (heat == null) ? new ObservableCollection<Athlete>() : heat.athletes[lane] });
                return results;
            }
        }
    }
}
