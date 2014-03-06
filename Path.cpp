#include <iostream>
#include <fstream>
#include <ctime>

#include "Path.h"

using namespace std;

namespace POWEROPT {
  void Path::print()
  {
    cout<<"Path "<<id<<": slack: "<<slack<<" treq: "<<treq<<" Gate number:"<<gates.size()<<endl;
    for (int i = 0; i < gates.size(); i ++)
      {
        cout<<"Gate "<<gates[i]->getId()<<":";
        cout<<" channel length: "<<gates[i]->getChannelLength()<<endl;
      }
  }
  
  void Path::fixCells()
  {
    for (int i = 0; i < gates.size(); i ++)
    {
    	gates[i]->setFixed(true);
    }
  }
  
  bool Path::addGate(Gate *g) 
  { 
    GateVectorItr itr;
    for (itr = gates.begin(); itr != gates.end(); itr++)
      {
        if ((*itr)->getId() == g->getId()) {
          return false;
        }
      }
    if (itr == gates.end()) {
      gates.push_back(g);
    } /* if (itr == gates.end()) */

    return true;
  }

  bool Path::addGateInDiffGrid(Gate *g) 
  { 
    GateVectorItr itr;
    for (itr = gates.begin(); itr != gates.end(); itr++)
      {
        if ((*itr)->getId() == g->getId() || (*itr)->getGrid()->getId() == g->getGrid()->getId()) {
          return false;
        }
      }
    if (itr == gates.end()) {
      gates.push_back(g);
    } /* if (itr == gates.end()) */

    return true;
  }

  void Path::calDelay()
  {
    delay = 0;
    for (int i = 0; i < gates.size(); i ++)
      {
        delay += gates[i]->getDelay();
      }
  }

  void Path::calOptDelay()
  {
    optDelay = 0;
    for (int i = 0; i < gates.size(); i ++)
      {
        optDelay += gates[i]->getOptDelay();
      }
  }

}

