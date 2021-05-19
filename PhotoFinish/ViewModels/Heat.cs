using System.Collections.ObjectModel;
using System.Collections.Generic;
using System.ComponentModel;
using System;
using System.IO;
using System.Linq;

namespace PhotoFinish
{
    public class Heat : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }

        public string Distance { get; private set; }
        public ObservableCollection<Athlete>[] athletes { get; private set; }

        public int Number
        {
            get; set;
        }

        public int AthleteCount
        {
            get
            {
                int count = 0;
                foreach (var lane in athletes)
                    count += lane.Count;
                return count;
            }
        }

        public string Ages
        {
            get
            {
                return string.Join(", ", ages);
            }
        }

        private IEnumerable<string> ages
        {
            get
            {
                var ages = new SortedSet<string>();
                foreach (var lane in athletes)
                    foreach (var athlete in lane)
                        ages.Add(athlete.AgeGroup);
                return ages;
            }
        }

        public ObservableCollection<string> Records
        {
            get
            {
                var records = new ObservableCollection<string>();
                foreach (var age in ages)
                    if (Athlete.records.ContainsKey(Distance))
                        if (Athlete.records[Distance].ContainsKey(age))
                            records.Add(age + ": " + Athlete.records[Distance][age].ToString(@"m\:ss\.ff"));
                return records;
            }
        }

        public TimeSpan BestTime
        {
            get
            {
                TimeSpan best = TimeSpan.MaxValue;
                TimeSpan clubbest = TimeSpan.MaxValue;

                foreach (var age in ages)
                {
                    foreach (var athlete in Athlete.athletes.Values)
                        if (athlete.AgeGroup == age && athlete.PBs.ContainsKey(Distance))
                        {
                            var time = athlete.PBs[Distance];
                            if (time < best)
                                best = time;
                        }

                    if (Athlete.records.ContainsKey(Distance) && Athlete.records[Distance].ContainsKey(age))
                    {
                        var time = Athlete.records[Distance][age];
                        if (time < clubbest)
                            clubbest = time;
                    }
                }

                if (best < TimeSpan.MaxValue)
                    return best;
                else if (clubbest < TimeSpan.MaxValue)
                    return clubbest;
                else
                    return TimeSpan.Zero;
            }
        }

        public bool laned;

        public bool Laned(String Distance)
        {
            switch (Distance)
            {
                case "70m":
                case "100m":
                case "200m":
                case "400m":
                case "60m Hurdles":
                case "80m Hurdles":
                case "90m Hurdles":
                case "100m Hurdles":
                case "110m Hurdles":
                case "200m Hurdles":
                case "300m Hurdles":
                    return true;
                case "300m":
                case "500m":
                case "700m":
                case "800m":
                case "1500m":
                case "300m Walk":
                case "700m Walk":
                case "1100m Walk":
                case "1500m Walk":
                    return false;
                default:
                    throw new Exception("");
            }
        }



        public Heat(string Distance)
        {
            this.Distance = Distance;

            this.laned = Laned(Distance);

            athletes = new ObservableCollection<Athlete>[8];
            for (int i = 0; i < 8; i++)
            {
                athletes[i] = new ObservableCollection<Athlete>();
                athletes[i].CollectionChanged += Heat_CollectionChanged;
            }
        }

        private void Heat_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnPropertyRaised("AthleteCount");
            OnPropertyRaised("BestTime");
            OnPropertyRaised("Records");
            OnPropertyRaised("Ages");
        }

        public void Save(StreamWriter writer)
        {
            writer.WriteLine("Event: {0}", this.Distance);
            writer.WriteLine(string.Join(",", athletes.Select(lane => string.Join(".", lane.Select(athlete => athlete.number)))));
        }
    }
}
