#include <iostream>
#include <fstream>
#include "Net.h"

using namespace std;

namespace POWEROPT {
  
ofstream Net::net_debug_file;

bool check_toggles = false;



  void Net::openFiles(string outDir)
  {
    string net_debug_file_name = outDir+"/PowerOpt/net_debug_file";
    net_debug_file.open(net_debug_file_name.c_str(), std::ofstream::out);
  }
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

  void Net::updateToggled()
  {
    if ( new_val == "0")
    {
      if (prev_val == "1") { toggle_type = FALL; toggled = true; }
      else if(prev_val == "x") { toggle_type = FALL; toggled = true; }
      else if (prev_val == "0") toggled = false; // sometimes vcds print the same net multiple times ending in the same value (meaning effectively no toggle)
      else 
      { 
        cout << driver_gate->getName() << "/" << name <<  " : prev_val : " << prev_val << " new_val : " << new_val << endl;
        assert(0); 
      }
    }
    else if (new_val == "1")
    {
      if (prev_val == "0") { toggle_type = RISE; toggled = true; }
      else if(prev_val == "x")  { toggle_type = RISE; toggled = true; }
      else if(prev_val == "1")  toggled = false; // sometimes vcds print the same net multiplie times ending in the same value (meaning effectively no toggle)
      else 
      { 
        cout << driver_gate->getName() << "/" << name << " : prev_val : " << prev_val << " new_val : " << new_val << endl;
        assert(0);
      }
    }
    else if (new_val == "x") 
    {
      if (prev_val == "x"){ toggled = false; }
      else { toggle_type = UNKN; toggled = true; }
    }

  }

  string Net::getOldSimVal(int toggle_offset)
  {
    if (toggle_offset = 0)
        return sim_val_old_0;
    else if (toggle_offset = 1)
        return sim_val_old_1;
    else assert(0);
  }

  bool Net::setSimValue(string value) // return value is whether the net toggled or not.
  {
     //if (value.empty()) assert(0);
     //assert(!value.empty()); // H3 added for debu
     if (sim_val != value)
     {
       if ((sim_ignore_once == true) && (sim_val == "X" || sim_val == "Xb") )
       {
         sim_ignore_once = false; 
       }
       else 
       {
         if (check_toggles == true){
            toggled = true; // whether the toggle needs to be registered or not.
         }
       }
      // handle Xbs here
       if ( sim_val == "X" || value == "X") sim_toggle_type = UNKN;
       else if (sim_val == "0" && value == "1") sim_toggle_type = FALL;
       else if (sim_val == "1" && value == "0") sim_toggle_type = RISE;
       //else assert(0); // empty strings need to be debugged.
//        if (sim_val == "X" || sim_val == "x") sim_val = value;
//        else if (sim_val == "0") sim_val = "1";
//        else if (sim_val == "1") sim_val = "0";
       sim_val = value; 
       sim_val_old_1 = sim_val_old_0;
       sim_val_old_0 = sim_val;
       return true;
     }
    sim_toggle_type = CONSTANT;
    if (value == "X") return true; // This should only be used when simulation uses the wavefront
     //cout << " NOT " << endl;
    return false;
  }

  void Net::flip_check_toggles() { check_toggles = true; } 

  Terminal* Net::getOutputTerminal()
  {
    if (out_terms.size() == 1) return out_terms[0];
    else if (out_terms.size() == 0) return NULL;
    else { assert(0); }
  }

  void Net::addFanoutGate(Gate* g)
  {
  	for (int i = 0; i < fanout_gates.size(); i ++)
  	{
  		if (fanout_gates[i]->getId() == g->getId())
  		{
  			//cout<<"fanout gate "<<g->getId()<<" Name: "<<g->getName()<<" already added for gate "<<name<<endl;
		  	return;
		}
  	}
  	fanout_gates.push_back(g);
  }

}
