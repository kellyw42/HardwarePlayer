using Prism.Commands;
using System.Collections.ObjectModel;
using System.Windows.Input;
using System.Windows;
using System.IO;
using System.Threading.Tasks;
using System.Linq;
using System;
using System.ComponentModel;
using System.Windows.Data;

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
        public ICommand FastForwardCommand { get; set; }
        public ICommand RewindCommand { get; set; }
        public ICommand PlayCommand { get; set; }
        public ICommand PauseCommand { get; set; }
        public ICommand PlayPauseCommand { get; set; }
        public ICommand UpCommand { get; set; }
        public ICommand DownCommand { get; set; }
        public ICommand PrevFrameCommand { get; set; }
        public ICommand NextFrameCommand { get; set; }
        public ICommand VisualSearchCommand { get; set; }
        public ICommand SaveCommand { get; set; }
        public ICommand ExitCommand { get; set; }
        public ICommand RemoveRaceCommand { get; set; }

        private Race CurrentRace
        {
            get
            {
                return CurrentEntry != null ? CurrentEntry.Race : null;
            }
        }

        //StreamWriter histogram = new StreamWriter("histogram55.csv");
        public Meet()
        {
            StartTimeHandler = new timehandler(start_time_handler);
            FinishTimeHandler = new timehandler(finish_time_handler);
            StartEventHandler = new eventhandler(start_event_handler);
            FinishEventHandler = new eventhandler(finish_event_handler);

            //audio1 = new eventhandler(StartAudioHandler);
            //audio2 = new eventhandler(FinishAudioHandler);

            try
            {
                NativeVideo.Init();
            }
            catch (System.DllNotFoundException e)
            {
                MessageBox.Show(e.Message, "dll not found exception");
            }
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
            PlayCommand = new DelegateCommand(Play);
            PauseCommand = new DelegateCommand(Pause);
            UpCommand = new DelegateCommand(Up);
            DownCommand = new DelegateCommand(Down);
            NextFrameCommand = new DelegateCommand(NextFrame);
            PrevFrameCommand = new DelegateCommand(PrevFrame);
            VisualSearchCommand = new DelegateCommand(VisualSearch);
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

            //CurrentMeet = "2020-02-28";

            Athlete.LoadAthletePBs(this);
            Athlete.LoadRecords();

            StartTimeStamp = new TimeStamp(this);
            FinishTimeStamp = new TimeStamp(this);

            LoadMeet();
        }

        private void UploadHeats()
        {
            if (NativeVideo.UploadHeats(Directory + "heats.txt"))
            {
                LoadHeats();
                AdjustEntries(null);
            }
        }

        private void ViewStart()
        {
            if (startPlayer != IntPtr.Zero)
                NativeVideo.GotoTime(startPlayer, StartTimeStamp.pts);
        }

        private void ViewFinish()
        {
            if (finishPlayer != IntPtr.Zero)
                NativeVideo.GotoTime(finishPlayer, NearestFrame(CurrentRace.CorrespondingFinishTime(StartTimeStamp.pts)));
        }

        private void UploadVideo()
        {
            if (CurrentMeet == null)
                return;

            var date = DateTime.Parse(CurrentMeet);
            var window = new Views.UploadVideos(date);
            if (window.ShowDialog().Value)
            {
                // add sync ...
                var race = new Race(this, 93600, window.start.dstFilename, window.finish.dstFilename, 
                    true, window.video_c0, window.video_c1);
                race.PropertyChanged += Race_PropertyChanged;
                Races.Add(race);
                AdjustEntries(race);
                SaveRaces();
            }
        }

        private void start_event_handler(long event_type, int wm_keydown)
        {
            event_handler(event_type, wm_keydown);
        }

        private void finish_event_handler(long event_type, int wm_keydown)
        {
            event_handler(event_type, wm_keydown);
        }

        private bool down = false;
        private System.DateTime down_start;

        public void AutoRepeatRight()
        {
            if (down)
            {
                var now = System.DateTime.Now;
                var down_time = now - down_start;
                if (down_time.TotalSeconds > 0.25)
                    PlayCommand.Execute(null);
                else
                    NextFrameCommand.Execute(null);
            }
            else
            {
                down = true;
                down_start = System.DateTime.Now;
                NextFrameCommand.Execute(null);
            }
        }

        public void StopRepeatRight()
        {
            down = false;
            PauseCommand.Execute(null);
        }

        private void event_handler(long event_type, int wm_keydown)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                if (wm_keydown != 0)
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
                        case 0x26: // Up
                            UpCommand.Execute(null);
                            return;
                        case 0x27: // Right
                            AutoRepeatRight();
                            return;
                        case 0x28: // Down
                            DownCommand.Execute(null);
                            return;

                        case 'S':
                            this.VisualSearchCommand.Execute(null);
                            return;
                    }
                else
                {
                    if (event_type == 0x27) // Right
                        StopRepeatRight();
                }
            });
        }

        private void VisualSearch()
        {
            NativeVideo.VisualSearch(startPlayer, NearestFrame(CurrentRace.CorrespondingStartTime(FinishTimeStamp.pts)));
        }

        private void Rewind()
        {
            NativeVideo.Rewind();
        }

        private void FastForward()
        {
            NativeVideo.FastForward();
        }

        private void Pause()
        {
            NativeVideo.Pause();
        }

        private void Play()
        {
            //recording = true;
            NativeVideo.Play();
        }

        private void PlayPause()
        {
            NativeVideo.PlayPause();
        }
        private void Up()
        {
            NativeVideo.Up();
        }

        private void Down()
        {
            NativeVideo.Down();
        }

        private void NextFrame()
        {
            NativeVideo.NextFrame();
        }

        private void PrevFrame()
        {
            NativeVideo.PrevFrame();
        }

        private long NearestFrame(long pts)
        {
            return (long)Math.Round((double)(pts / TimeStamp.PTS_PER_FRAME)) * TimeStamp.PTS_PER_FRAME;
        }

        private void AddRace()
        {
            Race race;

            // find where this race belongs in list based on start time ...
            int i = 0;
            while (i < Races.Count && Races[i].StartTime.ToTime() < StartTimeStamp.ToTime())
                i++;

            // insert or replace new race ...
            if (i == Races.Count || Races[i].StartTime.ToTime() != StartTimeStamp.ToTime())
            {
                race = new Race(this, StartTimeStamp.pts, StartTimeStamp.filename, FinishTimeStamp.filename, false, 0, 0);
                race.PropertyChanged += Race_PropertyChanged;
                Races.Insert(i, race);
            }
            else
                race = Races[i];

            CurrentEntry = AdjustEntries(race);

            // inherit synchronization from previous race
            if (i > 0 && !race.IsSync)
            {
                race.video_c0 = Races[i - 1].video_c0;
                race.video_c1 = Races[i - 1].video_c1;
            }

            long best = 0;
            if (race.heat != null)
                best = TimeStamp.Pts(race.heat.BestTime);

            var skip = (long)(0.9 * best);

            SaveRaces();

            NativeVideo.GotoTime(finishPlayer, NearestFrame(race.CorrespondingFinishTime(race.StartTime.pts) + skip));
            //NativeVideo.PlayPause();
        }

        private void Race_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            SaveRaces();
        }

        private TimeStamp StartTimeStamp, FinishTimeStamp;
        IntPtr startPlayer = IntPtr.Zero, finishPlayer = IntPtr.Zero;

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyRaised(string propertyname)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyname));
        }

        private void start_time_handler(long pts)
        {
            StartTimeStamp.pts = pts;
            StartTime = StartTimeStamp.ToString();
        }

        //private System.Drawing.Bitmap image = new System.Drawing.Bitmap(1920, 1080);
        //private int xpos = 1900;
        //private int icount = 0;
        private void finish_time_handler(long pts)
        {
            FinishTimeStamp.pts = pts;

            //if (recording)
            //{
            //    var x = 960;

            //    var size = 4 * 1920 * 1080 / 2;
            //    byte[] dest = new byte[size];
            //    System.Runtime.InteropServices.Marshal.Copy(host, dest, 0, size);

            //    var top = 1039;
            //    var bottom = 985;

            //    for (int yy = 0; yy < 1080; yy++)
            //    {
            //        for (int i = 0; i < 20; i+=2)
            //        {
            //            var dy = ((float)yy) / 1080;
            //            int copyx = (int)(dy * bottom + (1 - dy) * top);

            //            var offset0 = ((yy / 2) * 1920 + copyx + (i/2) - 10) * 4;
            //            var blue0 = dest[offset0];
            //            var green0 = dest[offset0 + 1];
            //            var red0 = dest[offset0 + 2];

            //            image.SetPixel(xpos + i, yy, System.Drawing.Color.FromArgb(red0, green0, blue0));
            //            image.SetPixel(xpos + i, yy + 1, System.Drawing.Color.FromArgb(red0, green0, blue0));
            //            image.SetPixel(xpos + i + 1, yy, System.Drawing.Color.FromArgb(red0, green0, blue0));
            //            image.SetPixel(xpos + i + 1, yy + 1, System.Drawing.Color.FromArgb(red0, green0, blue0));
            //        }
            //        yy++;
            //    }
            //    xpos -= 20;
            //    if (xpos < 0)
            //    {
            //        image.Save("photofinish" + icount.ToString("D3") + ".tif", System.Drawing.Imaging.ImageFormat.Tiff);
            //        xpos = 1900;
            //        icount++;
            //    }


            //    var y = 540;
            //    var offset = ((y / 2) * 1920 + x) * 4;
            //    var blue = dest[offset];
            //    var green = dest[offset + 1];
            //    var red = dest[offset + 2];

            //    histogram.WriteLine("{0},{1},{2},{3}", FinishTimeStamp.ToString(), red, green, blue);
            //    histogram.Flush();
            //}
            if (CurrentRace != null)
                FinishTime = FinishTimeStamp.Elapsed;
            else
                FinishTime = "";
        }

        public void RaceChanged()
        {
            if (CurrentRace == null)
                return;

            StartTimeStamp.race = CurrentRace;
            FinishTimeStamp.race = CurrentRace;

            if (StartTimeStamp.filename != CurrentRace.StartTime.filename || FinishTimeStamp.filename != CurrentRace.FinishFile)
            {
                StartTimeStamp.filename = CurrentRace.StartTime.filename;
                FinishTimeStamp.filename = CurrentRace.FinishFile;

                if (startPlayer != IntPtr.Zero)
                    NativeVideo.Close(startPlayer);

                if (finishPlayer != IntPtr.Zero)
                    NativeVideo.Close(finishPlayer);

                var progress = new PhotoFinish.Views.LoadProgressPair();

                var start = CurrentRace.StartTime.filename;
                var finish = CurrentRace.FinishFile;

                //Task.Run(() =>
                //{
                    var startSource = NativeVideo.LoadVideo(start, progress.StartProgressHandler);
                    startPlayer = NativeVideo.OpenVideo(startSource, StartEventHandler, StartTimeHandler);
                //});

                //Task.Run(() =>
                //{
                    var finishSource = NativeVideo.LoadVideo(finish, progress.FinishProgressHandler);
                    finishPlayer = NativeVideo.OpenVideo(finishSource, FinishEventHandler, FinishTimeHandler);
                //});

                //progress.ShowDialog();
            }

            System.Diagnostics.Debug.Assert(startPlayer != null);
            NativeVideo.GotoTime(startPlayer, CurrentRace.StartTime.pts);
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
            SaveRaces();
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
                    {
                        Heats.Add(heat);
                        heat.PropertyChanged += Heat_PropertyChanged;
                    }
                }
            }
        }

        private void SaveHeats()
        {
            StreamWriter writer = Backup(Directory + @"\heats.txt");
            for (int i = 0; i < Heats.Count; i++)
                Heats.ElementAt(i).Save(writer);
            writer.Close();
        }

        private void Heat_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            this.SaveHeats();
        }

        public StreamWriter Backup(string filename)
        {
            var backup = filename + ".backup2";
            if (File.Exists(backup))
                File.Delete(backup);
            if (File.Exists(filename))
                File.Move(filename, backup);
            return new StreamWriter(filename);
        }

        public void SaveRaces()
        {
            StreamWriter writer = Backup(Directory + @"\races2.txt");
            for (int i = 0; i < Races.Count; i++)
                Races.ElementAt(i).Save(writer);
            writer.Close();
        }

        private void LoadRaces()
        {
            Races.Clear();

            var racesFile = Directory + "races2.txt";

            if (File.Exists(racesFile))
            {
                var file = new StreamReader(racesFile);
                while (!file.EndOfStream)
                {
                    var race = new Race(this, file.ReadLine());
                    race.PropertyChanged += Race_PropertyChanged;
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
            var emailWriter = new StreamWriter(Directory + @"\email.csv");
            var resultshqWriter = new StreamWriter(Directory + @"\resultshq.csv");

            resultshqWriter.WriteLine("id,firstname,surname,age,gender,eventname,date,result,heattype,pb,status");

            foreach (var entry in Entries)
                if (entry.Heat != null && entry.Race != null)
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
            int i = 0;
            while (i < CurrentRace.finishTimes[lane.Value - 1].Count && CurrentRace.finishTimes[lane.Value - 1][i].pts < FinishTimeStamp.pts)
                i++;
            CurrentRace.finishTimes[lane.Value - 1].Insert(i, new TimeStamp(FinishTimeStamp));
            SaveRaces();
        }
    }
}   
