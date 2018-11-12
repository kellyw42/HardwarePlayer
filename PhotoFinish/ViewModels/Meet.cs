using Prism.Commands;
using System.Collections.ObjectModel;
using System.Windows.Input;
using System.Windows;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System;
using System.ComponentModel;
using System.Windows.Data;
using Microsoft.Win32;

namespace PhotoFinish
{
    public class Meet : INotifyPropertyChanged
    {
        const string MeetsDirectory = @"C:\PhotoFinish\Meets2\";

        public string Directory
        {
            get
            {
                return MeetsDirectory + CurrentMeet + @"\";
            }
        }

        private Entry currentEntry;
        public Entry CurrentEntry
        {
            get { return currentEntry; }
            set
            {
                currentEntry = value;
                OnPropertyRaised("CurrentEntry");
                RaceChanged();
            }
        }

        //private eventhandler audio1, audio2;
        private timehandler StartTimeHandler, FinishTimeHandler;
        private eventhandler StartEventHandler, FinishEventHandler;

        private string startTime;
        public string StartTime
        {
            get { return startTime; }
            set { startTime = value; OnPropertyRaised("StartTime"); }
        }

        private string finishTime;
        public string FinishTime
        {
            get { return finishTime; }
            set { finishTime = value; OnPropertyRaised("FinishTime"); }
        }

        public ObservableCollection<Entry> Entries { get; private set; }
        public ObservableCollection<Heat> Heats { get; private set; }
        public ObservableCollection<Race> Races { get; private set; }
        public ObservableCollection<string> Dates { get; private set; }

        public ObservableCollection<long> StartBangs { get; private set; }
        public ObservableCollection<long> FinishBangs { get; private set; }

        public string CurrentMeet { get; set; }

        public ICommand ViewStartCommand { get; set; }
        public ICommand ViewFinishCommand { get; set; }
        public ICommand UploadVideoCommand { get; set; }
        public ICommand UploadHeatsCommand { get; set; }
        public ICommand GotoFinishTimeCommand { get; set; }
        public ICommand MeetChangedCommand { get; set; }
        public ICommand FinishLaneCommand { get; set; }
        public ICommand StartCommand { get; set; }
        public ICommand FindSyncCommand { get; set; }
        public ICommand AddSyncCommand { get; set; }
        public ICommand FastForwardCommand { get; set; }
        public ICommand RewindCommand { get; set; }
        public ICommand PlayPauseCommand { get; set; }
        public ICommand PrevFrameCommand { get; set; }
        public ICommand NextFrameCommand { get; set; }
        public ICommand VisualSearchCommand { get; set; }
        public ICommand SaveCommand { get; set; }
        public ICommand ExitCommand { get; set; }
        public ICommand RemoveRaceCommand { get; set; }

        public Meet()
        {
            StartTimeHandler = new timehandler(start_time_handler);
            FinishTimeHandler = new timehandler(finish_time_handler);
            StartEventHandler = new eventhandler(start_event_handler);
            FinishEventHandler = new eventhandler(finish_event_handler);

            //audio1 = new eventhandler(StartAudioHandler);
            //audio2 = new eventhandler(FinishAudioHandler);

            NativeVideo.Init();

            StartTime = "00:00:00.00";

            ViewStartCommand = new DelegateCommand(ViewStart);
            ViewFinishCommand = new DelegateCommand(ViewFinish);
            UploadVideoCommand = new DelegateCommand(UploadVideo);
            UploadHeatsCommand = new DelegateCommand(UploadHeats);
            FinishLaneCommand = new DelegateCommand<int?>(FinishLane);
            SaveCommand = new DelegateCommand(SaveResults);
            ExitCommand = new DelegateCommand(Exit);
            MeetChangedCommand = new DelegateCommand(LoadMeet);
            StartCommand = new DelegateCommand(AddRace);
            RemoveRaceCommand = new DelegateCommand<Entry>(RemoveRace);
            FastForwardCommand = new DelegateCommand(FastForward);
            RewindCommand = new DelegateCommand(Rewind);
            PlayPauseCommand = new DelegateCommand(PlayPause);
            NextFrameCommand = new DelegateCommand(NextFrame);
            PrevFrameCommand = new DelegateCommand(PrevFrame);
            VisualSearchCommand = new DelegateCommand(VisualSearch);
            FindSyncCommand = new DelegateCommand(FindSync);
            AddSyncCommand = new DelegateCommand(AddSync);
            GotoFinishTimeCommand = new DelegateCommand<TimeStamp>(GotoFinishTime);

            Entries = new ObservableCollection<Entry>();
            Heats = new ObservableCollection<Heat>();
            Races = new ObservableCollection<Race>();
            Dates = new ObservableCollection<string>();
            StartBangs = new ObservableCollection<long>();
            FinishBangs = new ObservableCollection<long>();

            BindingOperations.EnableCollectionSynchronization(StartBangs, this);
            BindingOperations.EnableCollectionSynchronization(FinishBangs, this);

            foreach (var meet in System.IO.Directory.EnumerateDirectories(MeetsDirectory))
                Dates.Add(Path.GetFileName(meet));

            var today = DateTime.Now.ToString("yyyy-MM-dd");
            if (!Dates.Contains(today))
            {
                System.IO.Directory.CreateDirectory(Directory + today);
                Dates.Add(today);
            }

            CurrentMeet = today;

            Athlete.LoadAthletePBs(this);
            Athlete.LoadRecords();

            StartTimeStamp = new TimeStamp(this);
            FinishTimeStamp = new TimeStamp(this);

            LoadMeet();
        }

        private void UploadHeats()
        {
            if (NativeVideo.UploadHeats(Directory + "heats.txt"))
                LoadHeats();
        }

        private void ViewStart()
        {
            if (startPlayer != IntPtr.Zero)
                NativeVideo.GotoTime(startPlayer, StartTimeStamp.pts);
        }

        private void ViewFinish()
        {
            if (finishPlayer != IntPtr.Zero)
                NativeVideo.GotoTime(finishPlayer, FinishTimeStamp.pts);
        }

        private void UploadVideo()
        {
            var date = DateTime.Parse(CurrentMeet);

            foreach (var drive in System.IO.DriveInfo.GetDrives())
            {
                string StreamFolder = drive.RootDirectory + @"PRIVATE\AVCHD\BDMV\STREAM\";
                if (drive.IsReady && drive.DriveType == DriveType.Removable && System.IO.Directory.Exists(StreamFolder))
                {
                    var window = new Views.UploadVideos(drive, StreamFolder, date);
                    window.Show();
                }
            }
        }

        private void start_event_handler(long event_type)
        {
            event_handler(event_type);
        }

        private void finish_event_handler(long event_type)
        {
            event_handler(event_type);
        }

        private void event_handler(long event_type)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                switch (event_type)
                {
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                        int lane = (int)event_type - '0';
                        FinishLaneCommand.Execute(lane);
                        return;
                    case ' ':
                        StartCommand.Execute(null);
                        return;
                    case 'F':
                        FastForwardCommand.Execute(null);
                        return;
                    case 'R':
                        RewindCommand.Execute(null);
                        return;
                    case 'P':
                        PlayPauseCommand.Execute(null);
                        return;
                    case 0x25: // Left
                        PrevFrameCommand.Execute(null);
                        return;
                    case 0x27: // Right
                        NextFrameCommand.Execute(null);
                        return;
                    case 'S':
                        this.VisualSearchCommand.Execute(null);
                        return;
                }
            });
        }

        //long time1 = -1, time2 = -1;
        //bool startDone, finishDone;

        //List<TimeStamp> startBangs = new List<TimeStamp>();
        //List<TimeStamp> finishBangs = new List<TimeStamp>();
/*
        private void StartAudioHandler(long sample)
        {
            if (sample < 0)
                startDone = true;
            else
            {
                var pts = StartTimeStamp.start + sample * TimeStamp.FRAMES_PER_SECOND * TimeStamp.PTS_PER_FRAME / 48000;
                startBangs.Add(new TimeStamp(this, null, StartTimeStamp.start, pts, StartTimeStamp.filename));
            }
        }

        private void FinishAudioHandler(long sample)
        {
            if (sample < 0)
                finishDone = true;
            else
            {
                var pts = FinishTimeStamp.start + sample * TimeStamp.FRAMES_PER_SECOND * TimeStamp.PTS_PER_FRAME / 48000;
                finishBangs.Add(new TimeStamp(this, null, FinishTimeStamp.start, pts, FinishTimeStamp.filename));
            }
        }
 */
        //TimeStamp startSync, finishSync;

        String FindVideo(String which)
        {
            var files = System.IO.Directory.EnumerateFiles(Directory, "Track1-" + which + "*.MTS");

            if (files.Count() == 1)
                return files.First();
            else
            {
                var dialog = new OpenFileDialog();
                dialog.InitialDirectory = Directory;
                dialog.Filter = "Video Files|*.MTS";
                if (dialog.ShowDialog() == true)
                    return dialog.FileName;
                else
                    return null;
            }
        }

        private void FindSync()
        {
            String startFile = FindVideo("Start");

            String finishFile = FindVideo("Finish");

            if (startFile == null || finishFile == null)
                return;

            StartTimeStamp.filename = startFile;
            startPlayer = NativeVideo.OpenVideo(startFile, StartEventHandler, StartTimeHandler);

            FinishTimeStamp.filename = finishFile;
            finishPlayer = NativeVideo.OpenVideo(finishFile, FinishEventHandler, FinishTimeHandler);


            //var view = new Views.BangView();
            //view.DataContext = this;
            //view.Show();
            /*
                        NativeVideo.AudioSearch(Directory + StartTimeStamp.filename + ".audio", audio1);
                        NativeVideo.AudioSearch(Directory + FinishTimeStamp.filename + ".audio", audio2);

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        while (!finishDone)
                            System.Threading.Thread.Sleep(100);

                        foreach (var t in finishBangs)
                            if (FinishTimeStamp.ToTime() <= t.ToTime() && t.ToTime() <= FinishTimeStamp.ToTime().Add(TimeSpan.FromSeconds(0.1)))
                            {
                                finishSync = t;
                                break;
                            }

                        ///////////////////////////////////////////////////////////////////////////////////////////////////////////

                        while (!startDone)
                            System.Threading.Thread.Sleep(100);

                        foreach (var t in startBangs)
                            if (StartTimeStamp.ToTime() <= t.ToTime() && t.ToTime() <= StartTimeStamp.ToTime().Add(TimeSpan.FromSeconds(0.1)))
                            {
                                startSync = t;
                                break;
                            }

                        StreamWriter writer = new StreamWriter("points.csv");
                        foreach (var t1 in startBangs)
                        {
                            var offset1 = t1.pts - startSync.pts;
                            foreach (var t2 in finishBangs)
                            {
                                var offset2 = t2.pts - finishSync.pts;
                                if (offset2 - 5 * 1800 <= offset1 && offset1 <= offset2 + 5 * 1800)
                                {
                                    writer.WriteLine("{0},{1}", offset1, offset2);
                                    break;
                                }
                            }
                        }
                        writer.Close();
                    */
        }

        private void StudySync()
        {
        }

        TimeStamp startSync, finishSync;

        private void AddSync()
        {
            var race = new Race(this);
            race.StartTime = new TimeStamp(StartTimeStamp);
            race.FinishFile = FinishTimeStamp.filename;
            race.IsSync = true;
            race.sync = (FinishTimeStamp.pts - FinishTimeStamp.start) - (StartTimeStamp.pts - StartTimeStamp.start);
            Races.Add(race);

            AdjustEntries(race);
            SaveRaces();


            startSync = new TimeStamp(StartTimeStamp); 
            finishSync = new TimeStamp(FinishTimeStamp);

            var thread = new System.Threading.Thread(StudySync);
            thread.IsBackground = true;
            thread.Start();
        }

        private void VisualSearch()
        {
            NativeVideo.VisualSearch(startPlayer, finishPlayer);
        }

        private void Rewind()
        {
            NativeVideo.Rewind();
        }

        private void FastForward()
        {
            NativeVideo.FastForward();
        }

        private void PlayPause()
        {
            NativeVideo.PlayPause();
        }

        private void NextFrame()
        {
            NativeVideo.NextFrame();
        }

        private void PrevFrame()
        {
            NativeVideo.PrevFrame();
        }

        private void AddRace()
        {
            var race = new Race(this);
            race.StartTime = new TimeStamp(StartTimeStamp);
            race.FinishFile = FinishTimeStamp.filename;

            int i = 0;
            while (i < Races.Count && Races[i].StartTime.ToTime() < race.StartTime.ToTime())
                i++;

            if (i == Races.Count || Races[i].StartTime.ToTime() != race.StartTime.ToTime())
            {
                Races.Insert(i, race);
                CurrentEntry = AdjustEntries(race);
            }
            else
                race = Races[i];

            long best = 0;
            if (race.heat != null)
                best = TimeStamp.Pts(race.heat.BestTime);

            long skip = (long)(0.9 * best);
            skip = (skip / TimeStamp.PTS_PER_FRAME) * TimeStamp.PTS_PER_FRAME;

            SaveRaces();

            NativeVideo.GotoTime(finishPlayer, race.StartTime.pts + race.sync + skip);
            NativeVideo.PlayPause();
        }

        private TimeStamp StartTimeStamp, FinishTimeStamp;
        IntPtr startPlayer = IntPtr.Zero, finishPlayer = IntPtr.Zero;

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }

        private void start_time_handler(long start, long pts)
        {
            StartTimeStamp.start = start;
            StartTimeStamp.pts = pts;

            StartTime = StartTimeStamp.ToString();
        }

        private void finish_time_handler(long start, long pts)
        {
            FinishTimeStamp.start = start;
            FinishTimeStamp.pts = pts;

            var race = CurrentEntry != null ? CurrentEntry.Race : null;

            if (race != null)
                FinishTime = FinishTimeStamp.Elapsed;
            else
                FinishTime = "";
        }

        public void RaceChanged()
        {
            if (CurrentEntry == null || CurrentEntry.Race == null)
                return;

            FinishTimeStamp.race = CurrentEntry.Race;
            if (FinishTimeStamp.filename != CurrentEntry.Race.FinishFile)
            {
                FinishTimeStamp.filename = CurrentEntry.Race.FinishFile;
                finishPlayer = NativeVideo.OpenVideo(Directory + CurrentEntry.Race.FinishFile, FinishEventHandler, FinishTimeHandler);
            }

            StartTimeStamp.race = CurrentEntry.Race;
            if (StartTimeStamp.filename != CurrentEntry.Race.StartTime.filename)
            {
                StartTimeStamp.filename = CurrentEntry.Race.StartTime.filename;
                startPlayer = NativeVideo.OpenVideo(Directory + CurrentEntry.Race.StartTime.filename, StartEventHandler, StartTimeHandler);
            }

            NativeVideo.GotoTime(startPlayer, CurrentEntry.Race.StartTime.pts);
        }

        private void GotoFinishTime(TimeStamp timestamp)
        {
            NativeVideo.GotoTime(finishPlayer, timestamp.pts);
        }

        private Entry AdjustEntries(Race target)
        {
            Entry targetEntry = null;

            int entryNum = 0, raceNum = 0, heatNum = 0;
            while (raceNum < Races.Count || heatNum < Heats.Count)
            {
                if (entryNum >= Entries.Count)
                    Entries.Add(new Entry());

                var entry = Entries[entryNum++];

                Race race = null;
                if (raceNum < Races.Count)
                    race = Races[raceNum++];

                if (race == target)
                    targetEntry = entry;

                Heat heat = null;
                if (heatNum < Heats.Count && (race == null || !race.IsSync))
                {
                    heat = Heats[heatNum++];
                    heat.Number = heatNum;
                }
                if (entry.Heat != heat)
                    entry.Heat = heat;
                if (entry.Race != race)
                    entry.Race = race;
                if (race != null)
                    race.heat = heat;
            }

            while (Entries.Count > entryNum)
                Entries.RemoveAt(Entries.Count-1);

            return targetEntry;
        }

        private void RemoveRace(Entry entry)
        {
            if (entry.Race != null)
                Races.Remove(entry.Race);
            AdjustEntries(null);
        }

        private void LoadHeats()
        {
            Heats.Clear();

            var heatsFile = Directory + "heats.txt";

            if (File.Exists(heatsFile))
            {
                var file = new StreamReader(heatsFile);
                var lastEvent = "???";

                while (!file.EndOfStream)
                {
                    var evnt = file.ReadLine().Substring(7);
                    if (evnt == "Same")
                        evnt = lastEvent;
                    else
                        lastEvent = evnt;
                    var lanes = file.ReadLine().Split(',');

                    Heat heat = new Heat(evnt);

                    for (int lane = 0; lane < lanes.Length; lane++)
                        if (lanes[lane].Length > 0)
                            foreach (var athlete in lanes[lane].Split('.'))
                                heat.athletes[lane].Add(Athlete.Lookup(int.Parse(athlete), this));

                    if (heat.AthleteCount > 0)
                        Heats.Add(heat);
                }
            }
        }

        public StreamWriter Backup(string filename)
        {
            var backup = filename + ".backup";
            if (File.Exists(backup))
                File.Delete(backup);
            if (File.Exists(filename))
                File.Move(filename, backup);
            return new StreamWriter(filename);
        }

        public void SaveRaces()
        {
            StreamWriter writer = Backup(Directory + @"\races.txt");
            for (int i = 0; i < Races.Count; i++)
                Races.ElementAt(i).Save(writer);
            writer.Close();
        }

        private void LoadRaces()
        {
            Races.Clear();

            var racesFile = Directory + "races.txt";

            if (File.Exists(racesFile))
            {
                var file = new StreamReader(racesFile);

                while (!file.EndOfStream)
                {
                    var race = new Race(this);

                    var parts = file.ReadLine().Split(',');
                    var StartFile = parts[0];
                    var StartStart = long.Parse(parts[1]);
                    var StartPts = long.Parse(parts[2]);

                    // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    race.StartTime = new TimeStamp(this, race, StartStart, StartPts, StartFile);

                    var Sync1 = long.Parse(parts[3]); // bang
                    var Distance = parts[4];

                    race.IsSync = (Distance == "Sync");

                    race.FinishFile = parts[5];

                    race.sync = long.Parse(parts[6]); // Visual Sync
                    var Sync3 = long.Parse(parts[7]); // Audio Sync

                    for (int lane=0; lane<8; lane++)
                        if (parts[8+lane].Length > 0)
                            foreach (var time in parts[8+lane].Split('.'))
                            {
                                var subparts = time.Split('|');
                                var FinishStart = long.Parse(subparts[0]);
                                var FinishPts = long.Parse(subparts[1]);
                                race.finishTimes[lane].Add(new TimeStamp(this, race, FinishStart, FinishPts, race.FinishFile));
                            }

                    Races.Add(race);
                }

                AdjustEntries(null);
            }
        }

        private void LoadMeet()
        {
            LoadHeats();
            LoadRaces();
            AdjustEntries(null);

            if (Entries.Count > 0)
                CurrentEntry = Entries[0];
        }

        private void MeetChanged()
        {
            LoadMeet();
        }

        private void SaveResults()
        {
            var emailWriter = new StreamWriter(Directory + @"\email-test.csv");
            var resultshqWriter = new StreamWriter(Directory + @"\resultshq-test.csv");

            resultshqWriter.WriteLine("id,firstname,surname,age,gender,eventname,date,result,heattype,pb,status");

            foreach (var entry in Entries)
                if (entry.Heat != null)
                {
                    var distance = entry.Heat.Distance;

                    for (int lane = 0; lane < entry.Race.finishTimes.Length; lane++)
                    {
                        var times = entry.Race.finishTimes[lane];
                        var athletes = entry.Heat.athletes[lane];
                        System.Diagnostics.Debug.Assert(times.Count == athletes.Count);
                        for (int i = 0; i < times.Count; i++)
                        {
                            var athlete = athletes[i];
                            var time = times[i].ElapsedSinceRaceStart;

                            System.Diagnostics.Debug.Assert(athlete.AgeGroup != "XXX");
                            if (athlete.number != 0)
                            {
                                emailWriter.WriteLine("{0},{1},{2},{3},{4},{5},{6},{7}", distance, athlete.number, athlete.AgeGroup.Substring(0, 2), athlete.AgeGroup.Substring(2, 1), time.TotalSeconds.ToString("f2"), athlete.Email, athlete.Firstname, athlete.Surname);

                                var age = int.Parse(athlete.AgeGroup.Substring(0, 2));
                                var gender = athlete.AgeGroup.Substring(2, 1);
                                if (gender == "B")
                                    gender = "M";
                                else
                                    gender = "F";
                                var date = DateTime.Parse(CurrentMeet).ToString("dd MMM yyyy");
                                resultshqWriter.WriteLine("{0},{1},{2},{3},{4},{5},{6},\"{7}\",{8},{9},{10}", athlete.number, athlete.Firstname, athlete.Surname, age, gender, distance, date, time.ToString(@"m\:ss\.ff"), "Heat", "NO", "OK");
                            }
                        }
                    }
                }

            emailWriter.Close();
            resultshqWriter.Close();
        }

        private void Exit()
        {
            Application.Current.Shutdown();
        }

        private void FinishLane(int? lane)
        {
            CurrentEntry.Race.finishTimes[lane.Value - 1].Add(new TimeStamp(FinishTimeStamp));
        }
    }
}   
