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
long system_state::sys_state_count = 0;
map<string, int> system_state::HADDR_to_branch_count;

static string bin2hex (string binary_str)
{
  bitset<16> set(binary_str);
  stringstream ss;
  ss << hex << uppercase << set.to_ulong();
  return ss.str();
}

static string bin2hex32 (string binary_str)
{
  bitset<32> set(binary_str);
  stringstream ss;
  ss << hex << uppercase << set.to_ulong();
  return ss.str();
}

void system_state::openFiles(string outDir)
{
    string sys_state_debug_file_name = outDir+"/PowerOpt/sys_state_debug_file";
    sys_state_debug_file.open(sys_state_debug_file_name.c_str(), std::ofstream::out);
}

string system_state::get_state_short()
{
  stringstream ss;
  //ss << "( GLOBAL ID : " << global_id << " PC: " << bin2hex(PC) << ", " << " Instr: " << bin2hex(instr) << ", " << "Cycle: " << cycle_num << " ) ";
  ss << "( GLOBAL ID : " << global_id << ", LOCAL ID " << local_id << ", HADDR: " << bin2hex32(HADDR) << ", " << " HRDATA: " << bin2hex32(HRDATA) << ", " << "Cycle: " << cycle_num << " ) ";
  return ss.str();
}

system_state::system_state()
{
  taken = false;
  not_taken = false;
  inp_dependent = true;
  global_id = sys_state_count++;
  cout << " NEW SYS_STATE WITH GLOBAL ID : " << global_id << endl;
}

system_state::system_state(vector<Net*>& nets, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf_inp, map<int, xbitset>& DMemory_inp, int cycle_num_inp, string PC_inp, string instr_inp, bool inp_dep)
{
  taken = false; not_taken = false;
  for (int i = 0; i < nets.size(); i++)
  {
    net_sim_value_map.insert(make_pair(i, nets[i]->getSimValue()));
  }
  cycle_num = cycle_num_inp;
  sim_wf = sim_wf_inp; // copy all contents!
  DMemory = DMemory_inp; // copy all contents!
  PC = PC_inp;
  instr = instr_inp;
  inp_dependent = inp_dep;
  global_id = sys_state_count++;
  cout << " NEW SYS_STATE WITH GLOBAL ID : " << global_id << endl;
}

system_state::system_state(vector<Net*>& nets, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf_inp, map<int, xbitset>& DMemory_inp, int cycle_num_inp, string HADDR_inp, string HRDATA_inp, int input_count_inp, int branch_id)
{
  taken = false; not_taken = false;
  for (int i = 0; i < nets.size(); i++)
  {
    net_sim_value_map.insert(make_pair(i, nets[i]->getSimValue()));
  }
  cycle_num = cycle_num_inp;
  sim_wf = sim_wf_inp; // copy all contents!
  DMemory = DMemory_inp; // copy all contents!
  HADDR = HADDR_inp;
  HRDATA = HRDATA_inp;
  input_count = input_count_inp;
  global_id = sys_state_count++;
  map<string, int>::iterator it = HADDR_to_branch_count.find(HADDR);
  local_id = branch_id;
/*  if (it == HADDR_to_branch_count.end())
  {
    HADDR_to_branch_count.insert(make_pair(HADDR, 1));// first branch
    local_id = 1;
  }
  else
  {
    local_id = ++it->second;
  }*/
  cout << " NEW SYS_STATE WITH GLOBAL ID : " << global_id << " AND LOCAL ID : " << local_id << endl;
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

bool system_state::compare_and_update_state_ARM(system_state& sys_state)
{
    bool can_skip = true;
    assert(HADDR == sys_state.HADDR);
    sys_state_debug_file << "MY GLOBAL ID : " << global_id << " OTHER GLOBAL ID : " << sys_state.global_id << " EVALUATING FOR HADDR " << bin2hex32(HADDR) << " at cycle " << cycle_num <<  endl;
    string design = PowerOpt::getInstance()->getDesign();
    set<string>& ignore_nets = PowerOpt::getInstance()->get_ignore_nets();
//    ignore_nets.insert("u_top_u_sys_u_core_cc_mux[0]");
//    ignore_nets.insert("n13889");
//    ignore_nets.insert("u_top_u_sys_u_core_cc_mux[1]");
//    ignore_nets.insert("u_top_u_sys_u_core_cc_mux[3]");
//    ignore_nets.insert("u_top_u_sys_u_core_cc_mux[2]");
//    ignore_nets.insert("u_top_u_sys_u_core_readport_0__1_");
//    ignore_nets.insert("u_top_u_sys_u_core_readport_1__0_");
      
    // compare net_sim_value_map
    map<int, string>::iterator it;
    for (it = net_sim_value_map.begin(); it!= net_sim_value_map.end(); it++)
    {
       int net_id = it->first; string sim_val = it->second;
       string sys_state_sim_val = sys_state.net_sim_value_map[net_id];
       Net * net = PowerOpt::getInstance()->getNet(net_id);
       Gate* gate = net->getDriverGate();
       //if (sys_state_sim_val != sim_val)
       if ((sim_val != "X" && sys_state_sim_val != sim_val)&&(gate == NULL || gate->getFFFlag() == true))
       {
          string net_name = net->getName();
          if (ignore_nets.find(net_name) == ignore_nets.end()) // status register
          {
            can_skip = false;
            Gate* gate = net->getDriverGate();
            string gate_name;
            if (gate != NULL) gate_name = gate->getName();
            else gate_name = "NO DRIVER GATE (confirm Pad?)";
            sys_state_debug_file << "Net : " << net_name  << " ( " << gate_name << " ) "  << sim_val << " : " << sys_state_sim_val << endl ;
            // BUILD CONSERVATIVE STATE
            it->second = "X";
          }
          else it->second = sys_state_sim_val; // update my stored state with the new value since old value has been already simulated
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
         if (old_val.is_conservative(other_val)) { } // nothing to do
         else
         {
           my_dit->second = get_conservative_val(my_dit->second, other_val);
           assert(my_dit->second.is_conservative(other_val));
           sys_state_debug_file << " Mem Location different : " << addr << " Old val : " << old_val.to_string() << " Other val : " << other_val.to_string() << " New val : " << my_dit->second.to_string() << endl;
           can_skip = false;
         }
       }
    }


    return can_skip;
}

bool system_state::compare_and_update_state(system_state& sys_state)
{
    bool can_skip = true;
    assert(PC == sys_state.PC);
    sys_state_debug_file << "GLOBAL ID : " << global_id << " EVALUATING FOR PC " << bin2hex(PC) << " at cycle " << cycle_num <<  endl;
    string design = PowerOpt::getInstance()->getDesign();
    set<string> ignore_nets;
    if (design == "flat_no_clk_gt")
    {
      ignore_nets.insert("n355");
      ignore_nets.insert("n1164");
      ignore_nets.insert("n1167");
      ignore_nets.insert("n1161");
    }
    else  if (design == "modified_9_hier")
    {
      ignore_nets.insert("execution_unit_0/register_file_0/n406");
      ignore_nets.insert("execution_unit_0/register_file_0/n400");
      ignore_nets.insert("execution_unit_0/register_file_0/n424");
      ignore_nets.insert("execution_unit_0/register_file_0/n403");
    }

    else assert(0);
      
    // compare net_sim_value_map
    map<int, string>::iterator it;
    for (it = net_sim_value_map.begin(); it!= net_sim_value_map.end(); it++)
    {
       int net_id = it->first; string sim_val = it->second;
       string sys_state_sim_val = sys_state.net_sim_value_map[net_id];
       //if (sys_state_sim_val != sim_val)
       if (sim_val != "X" && sys_state_sim_val != sim_val)
       {
          Net * net = PowerOpt::getInstance()->getNet(net_id);
          string net_name = net->getName();
          if (ignore_nets.find(net_name) == ignore_nets.end()) // status register
          {
            Gate* gate = net->getDriverGate();
            string gate_name;
            if (gate != NULL) gate_name = gate->getName();
            else gate_name = "NO DRIVER";
            sys_state_debug_file << "Net : " << net_name  << " ( " << gate_name << " ) "  << sim_val << " : " << sys_state_sim_val << endl ;
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
         if (old_val.is_conservative(other_val)) { } // nothing to do
         else
         {
           my_dit->second = get_conservative_val(my_dit->second, other_val);
           assert(my_dit->second.is_conservative(other_val));
           sys_state_debug_file << " Mem Location different : " << addr << " Old val : " << old_val.to_string() << " Other val : " << other_val.to_string() << " New val : " << my_dit->second.to_string() << endl;
           can_skip = false;
         }
       }
    }


    return can_skip;
}


}
