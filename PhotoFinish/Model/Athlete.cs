using System.Collections.Generic;

namespace PhotoFinish
{
    public class Athlete
    {
        public string ageGroup { get; set; }
        public int number { get; set; }
        public string firstname { get; set; }
        public string surname { get; set; }
        public Dictionary<string, double> PBs { get; set; }

        public string Summary
        {
            get
            {
                return "#" + number + " " + firstname + " " + surname + "\t(" + ageGroup + ")";
            }
        }
    }
}
