#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <limits.h>
#include <bitset>
#include <stdlib.h>

#include "Net.h"
#include "Terminal.h"
#include "Path.h"
#include "Pad.h"
#include "Gate.h"
#include "PowerOpt.h"
#include "system_state.h"
#include "Globals.h"

using namespace std;


namespace POWEROPT {

ofstream system_state::sys_state_debug_file;

static string bin2hex (string binary_str)
{
  bitset<16> set(binary_str);
  stringstream ss;
  ss << hex << uppercase << set.to_ulong();
  return ss.str();
}

void system_state::openFiles(string outDir)
{
    string sys_state_debug_file_name = outDir+"/PowerOpt/sys_state_debug_file";
    sys_state_debug_file.open(sys_state_debug_file_name.c_str(), std::ofstream::out);
}

static xbitset get_conservative_val(xbitset a, xbitset b)
{
   bitset<NUM_BITS>& a_bs_x   = a.get_bs_x();
   bitset<NUM_BITS>& a_bs_val = a.get_bs_val();
   bitset<NUM_BITS>& b_bs_x   = b.get_bs_x();
   bitset<NUM_BITS>& b_bs_val = b.get_bs_val();

   // locations where values are different
   bitset<NUM_BITS> diff_val = a_bs_val ^ b_bs_val;
   // maximize num locations with 'X'
   bitset<NUM_BITS> conservative_x = a_bs_x | b_bs_x;
   // all places with different values are also marked as 'X'
   bitset<NUM_BITS> final_conservative_x = conservative_x | diff_val;

   xbitset conservative_xbitset(final_conservative_x, a_bs_val);
   return conservative_xbitset;
}

bool system_state::compare_and_update_state(system_state& sys_state)
{
    bool can_skip = true;
    assert(PC == sys_state.PC);
    sys_state_debug_file << " EVALUATING FOR PC " << bin2hex(PC) << " at cycle " << cycle_num <<  endl;

    // compare net_sim_value_map
    map<int, string>::iterator it;
    for (it = net_sim_value_map.begin(); it!= net_sim_value_map.end(); it++)
    {
       int id = it->first; string sim_val = it->second; 
       string sys_state_sim_val = sys_state.net_sim_value_map[id];  
       //if (sys_state_sim_val != sim_val)
       if (sim_val != "X" && sys_state_sim_val != sim_val)
       { 
          Net * net = PowerOpt::getInstance()->getNet(id);
          string net_name = net->getName();
          if (net_name != "n355" && net_name != "n1164" && net_name != "n1167" && net_name != "n1161") // status register
          {
            sys_state_debug_file << "Net : " << net_name  << " ( " << net->getDriverGate()->getName() << " ) "  << sim_val << " : " << sys_state_sim_val << endl ;
            // BUILD CONSERVATIVE STATE 
            it->second = "X";
            can_skip = false; 
          }
       }
    }

    // compare DMemory
    map<int, xbitset>:: iterator dit;
    map<int, xbitset>:: iterator my_dit;
    map<int, xbitset>& other_DMemory = sys_state.DMemory;
    for (dit = other_DMemory.begin(); dit != other_DMemory.end(); dit++)
    {
       int addr = dit->first; xbitset & other_val = dit->second;
       my_dit = DMemory.find(addr);
       if (my_dit == DMemory.end()) // conservative state doesn't have this address stored at all
       {
         DMemory[addr] = other_val; // just copy that value at that address;
         can_skip = false;
         sys_state_debug_file << " New Mem Location : " << addr << " val : " << other_val.to_string() << endl;
       }
       else // addr exists
       {
         xbitset old_val = my_dit->second;
         if (old_val == other_val) { } // nothing to do
         else 
         {
           my_dit->second = get_conservative_val(my_dit->second, other_val);
           sys_state_debug_file << " Mem Location different : " << addr << " Old val : " << old_val.to_string() << 
           " Other val : " << other_val.to_string() <<
           " New val : " << my_dit->second.to_string() << endl;
           can_skip = false;
         }
       }
    }


    return can_skip;
}


}
