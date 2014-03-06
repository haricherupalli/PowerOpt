#include <iostream>
#include <fstream>
#include <ctime>

#include "Grid.h"

using namespace std;

namespace POWEROPT {
  void Grid::print()
  {
    cout<<"Grid "<<id<<": ["<<lx<<","<<by<<"]->["<<rx<<","<<ty<<"]"<<endl;
    for (int i = 0; i < gates.size(); i ++)
      {
        cout<<"Gate "<<gates[i]->getId()<<":";
        cout<<" channel length: "<<gates[i]->getChannelLength()<<endl;
      }
  }

  void Grid::removeGate(int gId)
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
