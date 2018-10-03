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
                OnPropertyChanged("Race");
                OnPropertyChanged("Lanes");
                OnPropertyChanged("Distance");
            }
        }

        private Heat heat;

        public Heat Heat
        {
            get { return heat; }
            set
            {
                heat = value;
                OnPropertyChanged("Heat");
                OnPropertyChanged("Lanes");
                OnPropertyChanged("Distance");
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
                if (race.IsSync)
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
