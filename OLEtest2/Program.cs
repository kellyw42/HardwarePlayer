
using System.Data.OleDb;

namespace OLEtest2
{
    class Program
    {
        static void Main(string[] args)
        {
            var conn = new OleDbConnection(@"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=C:\PhotoFinish\MemberReport.xlsx;Extended Properties='Excel 12.0;IMEX=1;'");
            conn.Open();
        }
    }
}
