using System;
using System.Collections.ObjectModel;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace Client
{
    public partial class MainWindow : Window
    {
        private ObservableCollection<TodoItem> Items { get; } = new ObservableCollection<TodoItem>();
        private TcpClient _tcp;
        private NetworkStream _stream;
        private const string SERVER_IP = "127.0.0.1"; // change if server is remote
        private const int SERVER_PORT = 5000;

        public MainWindow()
        {
            InitializeComponent();
            ItemsListView.ItemsSource = Items;
            Loaded += MainWindow_Loaded;
        }

        private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await ConnectToServerAsync();
        }

        private async Task ConnectToServerAsync()
        {
            try
            {
                _tcp = new TcpClient();
                await _tcp.ConnectAsync(SERVER_IP, SERVER_PORT);
                _stream = _tcp.GetStream();
                // request initial list
                await SendStringAsync("GET\n");
                _ = Task.Run(() => ListenLoop());
            }
            catch (Exception ex)
            {
                MessageBox.Show("Could not connect to server: " + ex.Message);
            }
        }

        private async Task SendStringAsync(string s)
        {
            if (_stream == null) return;
            byte[] b = Encoding.UTF8.GetBytes(s);
            try
            {
                await _stream.WriteAsync(b, 0, b.Length);
            }
            catch
            {
                // ignore
            }
        }

        private async void AddButton_Click(object sender, RoutedEventArgs e)
        {
            string txt = NewItemTextBox.Text.Trim();
            if (string.IsNullOrEmpty(txt)) return;
            await SendStringAsync("ADD:" + txt + "\n");
            NewItemTextBox.Text = "";
        }

        private async void CheckBox_Click(object sender, RoutedEventArgs e)
        {
            if (sender is CheckBox cb && cb.DataContext is TodoItem item)
            {
                
                await SendStringAsync("TOGGLE:" + item.Id + "\n");
            }
        }

        private async Task ListenLoop()
        {
            var buffer = new byte[1024];
            var sb = new StringBuilder();
            try
            {
                while (true)
                {
                    int read = await _stream.ReadAsync(buffer, 0, buffer.Length);
                    if (read <= 0) break;
                    sb.Append(Encoding.UTF8.GetString(buffer, 0, read));
                    string s = sb.ToString();
                    int idx;
                    while ((idx = s.IndexOf('\n')) >= 0)
                    {
                        string line = s.Substring(0, idx).Trim();
                        if (!string.IsNullOrEmpty(line))
                        {
                            ProcessServerLine(line);
                        }
                        s = s.Substring(idx + 1);
                    }
                    sb.Clear();
                    sb.Append(s);
                }
            }
            catch
            {
               
            }
        }

        private void ProcessServerLine(string line)
        {
            try
            {
                using (JsonDocument doc = JsonDocument.Parse(line))
                {
                    var root = doc.RootElement;
                    if (root.TryGetProperty("type", out var t) && t.GetString() == "FULL_LIST")
                    {
                        var itemsElem = root.GetProperty("items");
                        Dispatcher.Invoke(() =>
                        {
                            Items.Clear();
                            foreach (var el in itemsElem.EnumerateArray())
                            {
                                var todo = new TodoItem
                                {
                                    Id = el.GetProperty("id").GetInt32(),
                                    Description = el.GetProperty("description").GetString() ?? "",
                                    Completed = el.GetProperty("completed").GetBoolean()
                                };
                                Items.Add(todo);
                            }
                        });
                    }
                }
            }
            catch (Exception ex)
            {
                
                Console.WriteLine("Parse error: " + ex.Message);
            }
        }
    }
}