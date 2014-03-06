#include <iostream>
#include "Subnet.h"

using namespace std;

namespace POWEROPT {
  
  void Subnet::print()
  {
    cout<<"===== SUBNET "<<id<<" ====="<<endl;
    cout<<"Name "<<name<<endl;
    if (iPad)
      cout<<"Input Pad: "<<inputPad->getName()<<endl;
    else
      cout<<"Input Terminal: "<<inputTerm->getName()<<endl;
    
    if (oPad)
      cout<<"Output Pad: "<<outputPad->getName()<<endl;
    else
      cout<<"Output Terminal: "<<outputTerm->getName()<<endl;
    
    cout<<"delay: "<<delay<<endl;
    cout<<"=================="<<endl;
  }

}
