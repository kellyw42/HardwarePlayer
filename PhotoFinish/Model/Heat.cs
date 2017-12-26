using System.Collections.ObjectModel;
using System.Collections.Generic;
using System.Text;

namespace PhotoFinish
{
    public class Heat
    {
        private int number;
        private Meet meet;

        public string Distance { get; private set; }
        public ObservableCollection<ObservableCollection<Athlete>> athletes { get; private set; }

        public ObservableCollection<Lane> lanes
        {
            get
            {
                ObservableCollection<Lane> results = new ObservableCollection<Lane>();
                for (int lane = 0; lane < athletes.Count; lane++)
                    results.Add(new Lane { LaneNumber = lane + 1, athletes = this.athletes[lane], finishTimes = (race == null) ? new ObservableCollection<TimeStamp>() : race.finishTimes[lane] });
                return results;
            }
            set
            {
                System.Console.WriteLine(value);
            }
        }

        private string AthleteCount
        { 
            get
            {
                int count = 0;
                foreach (var lane in athletes)
                    count += lane.Count;
                return count + " athletes";
            }
        }

        private string Ages
        {
            get
            {
                var ages = new SortedSet<string>();
                foreach (var lane in athletes)
                    foreach (var athlete in lane)
                        ages.Add(athlete.ageGroup);
                return string.Join(", ", ages);
            }
        }

        public override string ToString()
        {
            return Summary;
        }

        //13: 74,0.082 17:57:19.44, 6 times,  70m, 6 athletes, 17G, 14G, 15G
        public string Summary
        {
            get
            {
                return (number+1) + ": " + RaceSummary + Distance + ", " + AthleteCount + ", " + Ages;
            }
        }

        private string RaceSummary
        {
            get
            {
                if (race != null)
                    return race.Summary + ", ";
                else
                    return "";
            }
        }
        
        public Race race
        {
            get
            {
                if (meet.Races.Count > number)
                    return meet.Races[number];
                else
                    return null;
            }
        }

        public Heat(string Distance, Meet meet, int number)
        {
            this.meet = meet;
            this.Distance = Distance;
            this.number = number;
            athletes = new ObservableCollection<ObservableCollection<Athlete>>();
            for (int i = 0; i < 8; i++)
                athletes.Add(new ObservableCollection<Athlete>());
        }

        public void AddAthlete(int lane, Athlete athlete)
        {
            athletes[lane].Add(athlete);
        }
    }
}
