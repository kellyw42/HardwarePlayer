using System;
using System.ComponentModel;
using System.Text.RegularExpressions;

namespace PhotoFinish
{
    public class TimeStamp : INotifyPropertyChanged
    {
        // Fixme: get from Video metadata !!!
        public const long PTS_PER_FRAME = 1800;
        public const long FRAMES_PER_SECOND = 50;

        private Meet meet;

        public event PropertyChangedEventHandler PropertyChanged;

        public Race race { get; set; }
        public long start { get; set; }
        public long pts { get; set; }
        public string filename { get; set; }

        private void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public System.Windows.Media.Brush Record
        {
            get
            {
                if (ElapsedSinceRaceStart < PB && ElapsedSinceRaceStart < RecordTime)
                    return System.Windows.Media.Brushes.Red;
                else
                    return System.Windows.Media.Brushes.LightBlue;
            }
        }

        public void Update()
        {
            OnPropertyChanged("Record");
            OnPropertyChanged("PB");
            OnPropertyChanged("RecordTime");
            OnPropertyChanged("ElapsedSinceRaceStart");
            OnPropertyChanged("Elapsed");
        }

        public TimeSpan PB
        {
            get
            {
                var athletes = meet.CurrentEntry.Heat.athletes;
                var times = meet.CurrentEntry.Race.finishTimes;
                for (int lane = 0; lane < times.Length; lane++)
                {
                    var pos = times[lane].IndexOf(this);
                    if (pos >= 0 && pos < athletes[lane].Count)
                    {
                        var athlete = athletes[lane][pos];
                        if (athlete.PBs.ContainsKey(meet.CurrentEntry.Distance))
                            return athlete.PBs[meet.CurrentEntry.Distance];
                    }
                }
                return TimeSpan.MaxValue;
            }
        }

        public TimeSpan RecordTime
        {
            get
            {
                var athletes = meet.CurrentEntry.Heat.athletes;
                var times = meet.CurrentEntry.Race.finishTimes;
                for (int lane = 0; lane < times.Length; lane++)
                {
                    var pos = times[lane].IndexOf(this);
                    if (pos >= 0 && pos < athletes[lane].Count)
                    {
                        var age = athletes[lane][pos].AgeGroup;
                        if (Athlete.records[meet.CurrentEntry.Distance].ContainsKey(age))
                            return Athlete.records[meet.CurrentEntry.Distance][age];
                    }
                }
                return TimeSpan.Zero;
            }
        }

        public TimeSpan ElapsedSinceRaceStart
        {
            get
            {
                return Span(pts - race.StartTime.pts - race.sync);
            }
        }

        public string Elapsed
        {
            get
            {
                var diff = ElapsedSinceRaceStart;
                return ((diff < TimeSpan.Zero) ? "-" : "") + diff.ToString(@"m\:ss\.ff");
            }
        }

        public TimeStamp(TimeStamp old)
        {
            this.meet = old.meet;
            this.pts = old.pts;
            this.race = old.race;
            this.start = old.start;
            this.filename = old.filename;
        }

        public TimeStamp(Meet meet)
        {
            this.meet = meet;
        }

        public TimeStamp(Meet meet, Race race, long start, long pts, string filename)
        {
            this.meet = meet;
            this.race = race;
            this.start = start;
            this.pts = pts;
            this.filename = filename;
        }

        public static TimeSpan Span(long pts)
        {
            var ticks = (long)pts * TimeSpan.TicksPerSecond / FRAMES_PER_SECOND / PTS_PER_FRAME;
            var milliseconds = TimeSpan.FromTicks(ticks).TotalMilliseconds;
            milliseconds = 10 * Math.Ceiling(milliseconds/10);
            return TimeSpan.FromMilliseconds(milliseconds);
        }

        public static long Pts(TimeSpan timeSpan)
        {
            return (long)(FRAMES_PER_SECOND * PTS_PER_FRAME * timeSpan.Ticks / TimeSpan.TicksPerSecond); 
        }

        public override string ToString()
        {
            var diff = ToTime();

            return ((diff < TimeSpan.Zero) ? "-" : "") + ToTime().ToString(@"hh\:mm\:ss\.ff");
        }

        public TimeSpan ToTime()
        { 
            Match m = Regex.Match(filename, @"Track(\d)-(Start|Finish)-(\d+)-(\d+)-(\d+).MTS");
            if (m.Success)
            {
                var track = int.Parse(m.Groups[1].Value);

                var start_finish = m.Groups[2].Value;

                var hours = int.Parse(m.Groups[3].Value);
                var minutes = int.Parse(m.Groups[4].Value);
                var seconds = int.Parse(m.Groups[5].Value);

                var time = new System.TimeSpan(hours, minutes, seconds);

                time = time.Add(Span((long)(pts - start)));

                return time;
            }
            else
                throw new Exception("Invalid video file path " + filename);
        }

        public string Bang
        {
            get
            {
                if (pts > 0)
                    return ToTime().ToString(@"hh\:mm\:ss\.ffff");
                else
                    return "End";
            }
        }
    }
}
