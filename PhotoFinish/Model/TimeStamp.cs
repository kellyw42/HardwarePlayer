using System;
using System.Text.RegularExpressions;

namespace PhotoFinish
{
    public class TimeStamp
    {
        // Fixme: get from Video metadata !!!
        const int PTS_PER_FRAME = 1800;
        const int FRAMES_PER_SECOND = 50;
        const long NANOSECONDS_PER_FRAME = 1000000000L / FRAMES_PER_SECOND;


        public ulong start { get; set; }
        public ulong pts { get; set; }
        public string filename { get; set; }
        //          1         2         3         4         5         6
        //0123456789012345678901234567890123456789012345678901234567890
        //C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS, 17:57:19.44
        public string ToTime()
        {
            Match m = Regex.Match(filename, @"C:\\PhotoFinish\\Meets2\\(\d+)-(\d+)-(\d+)\\Track(\d)-(Start|Finish)-(\d+)-(\d+)-(\d+).MTS");
            if (m.Success)
            {
                var year = int.Parse(m.Groups[1].Value);
                var month = int.Parse(m.Groups[2].Value);
                var day = int.Parse(m.Groups[3].Value);

                var track = int.Parse(m.Groups[4].Value);

                var start_finish = m.Groups[5].Value;

                var hours = int.Parse(m.Groups[6].Value);
                var minutes = int.Parse(m.Groups[7].Value);
                var seconds = int.Parse(m.Groups[8].Value);

                var time = new System.DateTime(year, month, day, hours, minutes, seconds);

                var frames = (long)(pts - start) / PTS_PER_FRAME;

                time.AddTicks(frames * NANOSECONDS_PER_FRAME);

                return time.ToString("HH:mm:ss.ff");
            }
            else
                throw new Exception("Invalid video file path " + filename);


            //    var dirstart = filename.IndexOf("Meets2");
            //var filestart = filename.IndexOf("Track");
            //filestart = filename.IndexOf('-', filestart+1);
            //filestart = filename.IndexOf('-', filestart+1);

            //if (dirstart < 0 || filestart < 0)
            //    throw new Exception("Invalid video file path");

            //var year = int.Parse(filename.Substring(dirstart + 7, 4));
            //var month = int.Parse(filename.Substring(dirstart + 12, 2));
            //var day = int.Parse(filename.Substring(dirstart + 15, 2));

            //var hours = int.Parse(filename.Substring(filestart + 1, 2));
            //var minutes = int.Parse(filename.Substring(filestart + 4, 2));
            //var seconds = int.Parse(filename.Substring(filestart + 7, 2));

        }
    }
}
