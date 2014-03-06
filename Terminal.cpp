#include <iostream>
#include <cstdio>
#include <algorithm>

#include "Terminal.h"

using namespace std;

namespace POWEROPT {

  void Terminal::print()
  {
    cout<<"-----Terminal "<<id<<"-----"<<endl;
    cout<<"Name "<<name<<endl;
    cout<<"-------------------"<<endl;
  }
  
}
