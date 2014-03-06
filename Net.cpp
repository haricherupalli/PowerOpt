#include <iostream>
#include "Net.h"

using namespace std;

namespace POWEROPT {
  
  Terminal * Net::getInputTerminal()
  {
    assert(0 < terms.size());
    if (terms[0]->getPinType() == OUTPUT)
      return terms[0];
    else
      return NULL;
  }
  
  void Net::makeFalseInputTerminal()
  {
    assert(0 < terms.size());
    terms[0]->setPinType(OUTPUT);
  }
  
  void Net::print()
  {
    cout<<"=====NET "<<id<<"====="<<endl;
    cout<<"Name "<<name<<endl;
    cout<<"=================="<<endl;
  }

}
