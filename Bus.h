
class Bus {
  public:
    Bus () { }
    Bus (string bus_name, int n)
    {
      name = bus_name;
      prev_value = string(n, 'x');
    }
  string name;
  string prev_value;
} ;
