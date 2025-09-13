using System.ComponentModel;

namespace Client
{
    public class TodoItem : INotifyPropertyChanged
    {
        public int Id { get; set; }

        private string _description;
        public string Description { get => _description; set { _description = value; OnPropertyChanged(nameof(Description)); } }

        private bool _completed;
        public bool Completed { get => _completed; set { _completed = value; OnPropertyChanged(nameof(Completed)); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
