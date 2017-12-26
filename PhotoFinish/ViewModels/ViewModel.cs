using System.ComponentModel;
using System.Collections.ObjectModel;

namespace PhotoFinish
{
    public class Meet : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public string date;
        public ObservableCollection<Heat> Heats { get; private set; }
        public ObservableCollection<Race> Races { get; private set; }

        public Meet()
        {
            var START  = @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Start-17-40-55.MTS";
            var FINISH = @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS";

            Heats = new ObservableCollection<Heat>();
            Races = new ObservableCollection<Race>();

            var race = new Race();
            race.SetStartTime(new TimeStamp { filename = START, start = 93600, pts = 93600 + 1800 * 100 });
            race.AddFinishTime(0, new TimeStamp { filename = FINISH, start = 93600, pts = 93600 + 1800 * 900 });
            race.AddFinishTime(0, new TimeStamp { filename = FINISH, start = 93600, pts = 93600 + 1800 * 800 });
            race.AddFinishTime(7, new TimeStamp { filename = FINISH, start = 93600, pts = 93600 + 1800 * 700 });

            AddRace(race);

            AddHeat(new Heat("100m", this, 0));
            AddHeat(new Heat("1500m Walk", this, 1));
            AddHeat(new Heat("200m Hurdles", this, 2));

            var sarah = new Athlete { ageGroup = "17G", firstname = "Sarah", surname = "Kelly", number = 42, PBs = new System.Collections.Generic.Dictionary<string, double>() };
            var rachel = new Athlete { ageGroup = "17G", firstname = "Rachel", surname = "Kelly", number = 21, PBs = new System.Collections.Generic.Dictionary<string, double>() };
            var wayne = new Athlete { ageGroup = "17B", firstname = "Wayne", surname = "Kelly", number = 1, PBs = new System.Collections.Generic.Dictionary<string, double>() };

            Heats[0].AddAthlete(0, sarah);
            Heats[0].AddAthlete(0, rachel);
            Heats[0].AddAthlete(7, wayne);
        }

        public void AddHeat(Heat heat)
        {
            Heats.Add(heat);
        }

        public void AddRace(Race race)
        {
            Races.Add(race);
        }
    }
}
