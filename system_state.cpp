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

void system_state::openFiles(string outDir)
{
    string sys_state_debug_file_name = outDir+"/PowerOpt/sys_state_debug_file";
    sys_state_debug_file.open(sys_state_debug_file_name.c_str(), std::ofstream::out);
}

bool system_state::compare_and_update_state(system_state& sys_state)
{
    bool can_skip = true;
    assert(PC == sys_state.PC);
    sys_state_debug_file << " EVALUATING FOR PC " << PC << endl;

    // compare net_sim_value_map
    map<int, string>::iterator it;
    for (it = net_sim_value_map.begin(); it!= net_sim_value_map.end(); it++)
    {
       int id = it->first; string sim_val = it->second; 
       string sys_state_sim_val = sys_state.net_sim_value_map[id];  
       if (sys_state_sim_val != sim_val)
       { 
          Net * net = PowerOpt::getInstance()->getNet(id);
          string net_name = net->getName();
          sys_state_debug_file << "Net : " << net_name  << " ( " << net->getDriverGate()->getName() << " ) "  << sim_val << " : " << sys_state_sim_val << endl ;
          if (net_name != "n355") // status register
            can_skip = false; 
       }
    }

    // compare DMemory


    return can_skip;
}


}