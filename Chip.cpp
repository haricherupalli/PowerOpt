#include <iostream>
#include <fstream>
#include <ctime>

#include "Chip.h"

using namespace std;

namespace POWEROPT {
  void Chip::print()
  {
    cout<<"Chip "<<id<<": ["<<lx<<","<<by<<"]->["<<rx<<","<<ty<<"]"<<endl;
    cout<<"RowNum: "<<siteRowNum<<" ColNum: "<<siteColNum<<" rowW: "<<rowW<<" rowH: "<<rowH<<endl;
//    for (int i = 0; i < gates.size(); i ++)
//      {
//      	Gate *g = gates[i];
//        cout<<"Gate "<<g->getId()<<":";
//        cout<<" Name: "<<g->getName()<<" ["<<g->getLX()<<","<<g->getBY()<<"]->["<<g->getRX()<<","<<g->getTY()<<"]"<<endl;
//      }
  }

  void Chip::removeGate(int gId)
  {
  	bool found = false;
    for (int i = 0; i < gates.size(); i ++)
      {
      	if (gates[i]->getId() == gId)
      	{
      		gates.erase(gates.begin()+i);
      		found = true;
      		break;
      	}
      }
      assert(found);
  }

}
