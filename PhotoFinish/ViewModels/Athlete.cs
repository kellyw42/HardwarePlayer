using System.Collections.Generic;
using System.Data.OleDb;
using System;
using System.IO;
using System.Globalization;
using System.ComponentModel;
using ExcelDataReader;

namespace PhotoFinish
{
    public class Athlete : INotifyPropertyChanged
    {
        private Meet meet;
        public string AgeGroup { get; set; }
        public int number { get; set; }
        public string Firstname { get; set; }
        public string Surname { get; set; }

        public string Email { get; set; }
        
        public Dictionary<string, TimeSpan> PBs { get; set; }

        public static Dictionary<int, Athlete> athletes = new Dictionary<int, Athlete>();
        public static Dictionary<string, Dictionary<string, TimeSpan>> records = new Dictionary<string, Dictionary<string, TimeSpan>>();

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public System.Windows.Media.Brush Improvement
        {
            get
            {
                if (FinishTime.Contains("-"))
                    return System.Windows.Media.Brushes.Red;
                else
                    return System.Windows.Media.Brushes.Black;
            }
        }

        public System.Windows.Media.Brush Matched
        {
            get
            {
                if (Time != TimeSpan.Zero)
                    return System.Windows.Media.Brushes.LightBlue;
                else
                    return System.Windows.Media.Brushes.Pink;
            }
        }

        public string Number
        {
            get
            {
                return "#" + number;
            }
        }

        public void Update()
        {
            OnPropertyChanged("FinishTime");
            OnPropertyChanged("Improvement");
            OnPropertyChanged("Matched");
        }

        private TimeSpan Time
        {
            get
            {
                var athletes = meet.CurrentEntry.Heat.athletes;
                var times = meet.CurrentEntry.Race.finishTimes;
                for (int lane = 0; lane < athletes.Length; lane++)
                {
                    var pos = athletes[lane].IndexOf(this);
                    if (pos >= 0 && pos < times[lane].Count)
                        return times[lane][pos].ElapsedSinceRaceStart;
                }
                return TimeSpan.Zero;
            }
        }

        public string FinishTime
        {
            get
            {
                if (PBs.ContainsKey(meet.CurrentEntry.Distance))
                {
                    var PB = PBs[meet.CurrentEntry.Distance];

                    if (Time != TimeSpan.Zero)
                    {
                        var diff = Time - PB;
                        return ((diff < TimeSpan.Zero) ? "-" : "+") + diff.ToString(@"s\.ff");
                    }

                    return PB.ToString(@"m\:ss\.ff");
                }

                return "";
            }
        }

        public string Name
        {
            get
            {
                return Firstname + " " + Surname;
            }
        }

        public static void LoadRecords()
        {
            records.Clear();

            var inFile = File.Open(@"C:\PhotoFinish\CentreRecords.xlsx", FileMode.Open, FileAccess.Read);
            var reader2 = ExcelReaderFactory.CreateReader(inFile, new ExcelReaderConfiguration());
            reader2.Read();
            reader2.Read();
            //var conn = new OleDbConnection(@"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=C:\PhotoFinish\CentreRecords.xlsx;Extended Properties='Excel 12.0;IMEX=1;'");
            //conn.Open();
            //var cmd = new OleDbCommand("select * from [Centre Records$A3:N]", conn);
            //var reader2 = cmd.ExecuteReader();
            reader2.Read();
            while (reader2.Read())
            {
                var evnt = reader2.GetString(6);

                if (char.IsDigit(evnt[0]))
                {
                    var time = reader2.GetString(1);
                    TimeSpan result;
                    if (!TimeSpan.TryParseExact(time, "m\\:ss\\.ff", CultureInfo.InvariantCulture, out result))
                        if (!TimeSpan.TryParseExact(time, "s\\.ff", CultureInfo.InvariantCulture, out result))
                            continue;

                    var gender = reader2.GetString(9);
                    var age = int.Parse(reader2.GetString(10));
                    var group = age.ToString("D2") + (gender == "Male" ? "B" : "G");

                    if (!records.ContainsKey(evnt))
                        records.Add(evnt, new Dictionary<string, TimeSpan>());

                    records[evnt][group] = result;
                }
            }
        }

        public static void LoadAthletePBs(Meet meet)
        {
            athletes.Clear();

            var inFile = File.Open(@"C:\PhotoFinish\MemberReport.xlsx", FileMode.Open, FileAccess.Read);
            var reader = ExcelReaderFactory.CreateReader(inFile, new ExcelReaderConfiguration());
            reader.Read();
            //var conn = new OleDbConnection(@"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=C:\PhotoFinish\MemberReport.xlsx;Extended Properties='Excel 12.0;IMEX=1;'");
            //conn.Open();
            //var cmd = new OleDbCommand("select * from [Member Report$]", conn);
            //var reader2 = cmd.ExecuteReader();
            while (reader.Read())
            {
                var num = int.Parse(reader.GetString(0));
                var fname = reader.GetString(1);
                var surname = reader.GetString(2);
                var dob = DateTime.Parse(reader.GetString(3));
                var age = reader.GetString(4);
                var gender = reader.GetString(6);
                var email = reader.GetString(16);

                if (age.Length < 2)
                    age = "0" + age;

                var group = age + (gender == "M" ? "B" : "G");

                var athlete = new Athlete { meet = meet, AgeGroup = group, Firstname = fname, number = num, Surname = surname, Email = email, PBs = new Dictionary<string, TimeSpan>() };
                athletes[num] = athlete;
            }
            reader.Close();
            inFile.Close();
            //conn.Close();

            try
            {
                inFile = File.Open(@"C:\PhotoFinish\SeasonReport_Season Best.xlsx", FileMode.Open, FileAccess.Read);
                reader = ExcelReaderFactory.CreateReader(inFile, new ExcelReaderConfiguration());
                reader.Read();

                //var conn = new OleDbConnection("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=C:\\PhotoFinish\\SeasonReport_Season Best.xlsx;Extended Properties='Excel 12.0;IMEX=1;'");
                //conn.Open();
                //var cmd = new OleDbCommand("select * from [Season Best$A2:K]", conn);
                //var reader2 = cmd.ExecuteReader();

                while (reader.Read())
                {
                    var num = int.Parse(reader.GetString(0));
                    if (athletes.ContainsKey(num))
                    {
                        var evnt = reader.GetString(6);
                        var time = reader.GetString(10);
                        TimeSpan best;
                        if (time.Contains(":"))
                            best = TimeSpan.ParseExact(time, "m':'ss'.'ff", null);
                        else
                            best = TimeSpan.FromSeconds(double.Parse(time));
                        var a = athletes[num];
                        a.PBs[evnt] = best;
                    }
                }
                reader.Close();
                inFile.Close();
            }
            catch (Exception)
            {
                // too bad, probably file format error
            }
        }

        public static Athlete Lookup(int number, Meet meet)
        {
            if (number == 0)
                return new Athlete { meet = meet, AgeGroup = "TTT", Firstname = "Trialist", Surname = "", number = 0, PBs = new Dictionary<string, TimeSpan>() };
            else if (athletes.ContainsKey(number))
                return athletes[number];
            else
                return new Athlete { meet = meet, AgeGroup = "XXX", Firstname = "Unregistered", Surname = "#" + number, number = number, PBs = new Dictionary<string, TimeSpan>() };
        }
    }
}
