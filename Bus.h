
class Bus {
  public:
    Bus () { }
    Bus (string bus_name, int msb, int lsb) // NOTE: lsb need not be zero!
    {
      name = bus_name;
      int length = msb - lsb + 1;
      this->lsb = lsb;
      this->msb = msb;
      prev_value = string(length, 'x');
    }
  string name;
  string prev_value;
  int lsb;
  int msb;
} ;
