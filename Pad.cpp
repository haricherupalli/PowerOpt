#include <iostream>
#include <cstdio>
#include <algorithm>

#include "Pad.h"

using namespace std;

namespace POWEROPT {

  void Pad::print()
  {
    cout<<"-----Pad "<<id<<"-----"<<endl;
    cout<<"Name "<<name<<endl;
    cout<<"-------------------"<<endl;
  }

  void Pad::setExpr()
  {
      if (nets.size() != 0)
      nets[0]->setExpr(name); 
  }

  void Pad::setSimValue(string value, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
  {
    if (nets.size() == 0)
    {
      sim_val = value;
      return; // a pad need not be connected to anything
    }
    Net* net = nets[0];
    bool toggled = net->setSimValue(value);
    if (toggled) 
    {
      int fo_num = net->getFanoutGateNum();
      for (int i = 0; i < fo_num; i++)
      {
        Gate* g = net->getFanoutGate(i);
        if (!g->getFFFlag())
        {
          GNode* node = g->getGNode();
          sim_wf.push(node);
        }
      }
    }
  }

  void Pad::setSimValue(string value)
  {
    if (nets.size() == 0) return; // a pad need not be connected to anything
    Net* net = nets[0];
    net->setSimValue(value);
  }

  string Pad::getSimValue()
  {
    if (nets.size() == 0) return  sim_val; // ONLY works for per_addrs
    return nets[0]->getSimValue();
  }

  ToggleType Pad::getSimToggType()
  {
    if (nets.size() == 0) return  UNKN;
    return nets[0]->getSimToggType();
  }
  
}
