#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>

#include "Terminal.h"

using namespace std;


namespace POWEROPT {

std::ofstream term_debug_file ("PowerOpt/term_debug_file", std::ofstream::out);

  void Terminal::print()
  {
    cout<<"-----Terminal "<<id<<"-----"<<endl;
    cout<<"Name "<<name<<endl;
    cout<<"-------------------"<<endl;
  }

  void Terminal::printNets(ostream& debug_file = std::cout)
  {
    for (int i = 0; i < nets.size(); i++)
    {
       debug_file << nets[i]->getName() << ", ";
    }
  }

  void Terminal::setToggled (bool val, string to)
  {
    toggled = val;
    prev_val = new_val;
    new_val = to;
    if ( new_val == "0")
    {
      if (prev_val == "1") toggle_type = FALL;
      else if(prev_val == "x") toggle_type = FALL;
      else if (prev_val == "0") toggled = false; // sometimes vcds print the same net twice (meaning effectively no toggle)
      else 
      { 
        cout << gatePtr->getName() << "/" << name <<  " : prev_val : " << prev_val << " new_val : " << new_val << endl;
        assert(0); 
      }
    }
    else if (new_val == "1")
    {
      if (prev_val == "0") toggle_type = RISE;
      else if(prev_val == "x")  toggle_type = RISE;
      else if(prev_val == "1")  toggled = false; // sometimes vcds print the same net twice (meaning effectively no toggle)
      else 
      { 
        cout << gatePtr->getName() << "/" << name << " : prev_val : " << prev_val << " new_val : " << new_val << endl;
        assert(0);
      }
    }
    else if (new_val == "x") 
    {
      toggle_type = UNKN;
    }
  }
   
  void Terminal::inferToggleFromNet()
  {
    assert(nets.size() == 1);
    Net* net = nets[0];
    toggled = net->getToggled();
    toggle_type = net->getToggleType();
    prev_val = net->getPrevVal();
    new_val = net->getToggledTo();
    //cout << getFullName() << " : " << net->getName()  << " : " << toggled << " : " << toggle_type << endl;
  } 

  
//  string Terminal::getFullName()
//  {
//    return gatePtr->getName()+"/"+name;
//  }

  void Terminal::computeFullName()
  { 
    fullName = gatePtr->getName()+"/"+name;
  }

  bool Terminal::isMuxOutput()
  {
    if( (gatePtr->getIsMux()) && (pin.getType() != OUTPUT)) return true; 
    return false;
  }

  string Terminal::getExpr() 
  {
    if (expr.length() != 0) return expr;
    return nets[0]->getExpr();
//    Net* net = nets[0];
//    Gate* gate = net->getDriverGate();
//    exp = gate->getFanoutTerminal(0)->getExpr();
//    return exp;
  }

  void Terminal::setExpr(string Expr)
  {
      expr = Expr; 
      nets[0]->setExpr(Expr); 
      cout << nets[0]->getName() << " --> " << Expr << endl;
  }
   
  string Terminal::getNetName() 
  {
    if (nets.size() != 0)
      return nets[0]->getName();
    else return "NULL";

  }

  void Terminal::setSimValue(string value,  priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
  {
    if (nets.size() == 0) return; // if floating/constant terminal
    Net* net = nets[0];
    bool toggled = net->setSimValue(value);
    if (toggled) 
    {
      int fo_num = net->getFanoutGateNum();
      //term_debug_file << net->getName() << " : " ;
      for (int i = 0; i < fo_num; i++)
      {
        Gate* g = net->getFanoutGate(i);
        if (!g->getFFFlag())
        {
          GNode* node = g->getGNode();
          sim_wf.push(node);
           //term_debug_file << node->getName() << ", " ;
        }
      }
      //term_debug_file << endl;
    }
  }                                         

  void Terminal::setSimValue(string value)
  {
    if (nets.size() == 0) return; // if floating/constant terminal
    nets[0]->setSimValue(value);
  }                                         
                                            
  string Terminal::getSimValue()            
  {
    return nets[0]->getSimValue();          
  }                                         

  ToggleType Terminal::getSimToggType()
  {
    return nets[0]->getSimToggType();
  }                                         
                                            
}                                           
                                            
