#ifndef __XBITSET_H__
#define __XBITSET_H__


#include <stdio.h>
#include <iostream>
#include <bitset>
#include <string>

#define NUM_BITS 16

using namespace std;

class xbitset
{

  public:
  xbitset() { }
  xbitset(string input_str);

  xbitset (bitset<NUM_BITS> x, bitset<NUM_BITS> val)
  {
    bs_x   = x;
    bs_val = val;
  }
  
  xbitset (int input)
  {
      bs_val = bitset<NUM_BITS> (input);
  }

  string to_string();
  
  void temp();
    
  bitset<NUM_BITS> get_bs_x()   { return bs_x;   }
  bitset<NUM_BITS> get_bs_val() { return bs_val; }

  private:
  bitset<NUM_BITS> bs_x;
  bitset<NUM_BITS> bs_val;

};


#endif 
