
#include <string>
#include <cassert>
#include "xbitset.h"

using namespace std;
////class xbitset;

/*xbitset operator & (xbitset a, xbitset b)
{
  bitset<NUM_BITS> bs_x = (a.get_bs_x() & b.get_bs_val()) | (a.get_bs_val() & b.get_bs_x()) | (a.get_bs_x() & b.get_bs_x());
  bitset<NUM_BITS> bs_val = a.get_bs_val() & b.get_bs_val();
  return xbitset(bs_x, bs_val);
}

xbitset operator | (xbitset a, xbitset b)
{
  bitset<NUM_BITS> bs_x = (a.get_bs_x() | b.get_bs_x()) & (~a.get_bs_val()) & (~b.get_bs_val()) ;
  bitset<NUM_BITS> bs_val = a.get_bs_val() | b.get_bs_val();
  return xbitset(bs_x, bs_val);
}*/


xbitset::xbitset(string input_str)
{
  if (input_str.size() != NUM_BITS) assert(0);
  for (int i = 0; i < NUM_BITS; i++)
  {
    char c = input_str[i];
    int j = NUM_BITS - 1 - i;
    switch (c)
    {
      case 'X':
        bs_x[j] = 1;
        break;
      case '1':
        bs_x[j] = 0;
        bs_val[j] = 1;
        break;
      case '0':
        bs_x[j] = 0;
        bs_val[j] = 0;
        break;
      default :
        break;
    }
  }
}


void xbitset::temp()
{
  return;
}

string xbitset::to_string()  
{
  string output_str(NUM_BITS, '0') ;
  for (int i = 0; i < NUM_BITS; i++)
  {
    int j = NUM_BITS - 1 - i;
    if (bs_x[j] == 1) { output_str[i] = 'X'; }
    else if (bs_val[j] == 1) { output_str[i] = '1'; }
    else if (bs_val[j] == 0) { output_str[i] = '0'; }
    else assert(0);
  }
  return output_str;
}
