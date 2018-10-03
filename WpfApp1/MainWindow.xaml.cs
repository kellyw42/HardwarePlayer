using System.Windows;
using System.Data.OleDb;

namespace WpfApp1
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            var conn = new OleDbConnection(@"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=C:\PhotoFinish\MemberReport.xlsx;Extended Properties='Excel 12.0;IMEX=1;'");
            conn.Open();

            InitializeComponent();
        }
    }
}
