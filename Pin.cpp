#include <iostream>
#include <cstdio>
#include <algorithm>
#include "assert.h"

#include "Pin.h"

using namespace std;

namespace POWEROPT {
  
  void Pin::print()
  {
    cout<<"-----PIN "<<id<<"-----"<<endl;
    cout<<"Name "<<name<<endl;
    cout<<"type "<<type<<endl;
    printf("  RECT %i %i %i %i\n",
           bbox.left(),
           bbox.bottom(),
           bbox.right(),
           bbox.top());
    cout<<"center ( "<<centerX<<" , "<<centerY<<" ) "<<endl;
    cout<<"-------------------"<<endl;
  }

  
}
