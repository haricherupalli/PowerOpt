#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

#include "xbitset.h"


namespace POWEROPT {

  class system_state{

    public:
     system_state();
     system_state(vector<Net*>& nets, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf_inp, map<int, xbitset>& DMemory_inp, int cycle_num_inp, string PC_inp, string instr_inp, bool inp_dep);
     system_state(vector<Net*>& nets, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf_inp, map<int, xbitset>& DMemory_inp, int cycle_num_inp, string HADDR_inp, string HRDATA_inp, int input_count_inp, int branch_id);
     long get_sys_state_count () { return sys_state_count; }
     int cycle_num;
     map<int, string> net_sim_value_map;// <net_id, sim_value>
     priority_queue<GNode*, vector<GNode*>, sim_wf_compare> sim_wf;
     map<int, xbitset> DMemory;
     string PC;
     string instr;
     string HADDR;
     string HRDATA;
     int input_count;
     bool taken;
     bool not_taken;
     bool inp_dependent;
     bool compare_and_update_state(system_state& sys_state);
     bool compare_and_update_state_ARM(system_state& sys_state);
     void openFiles(string outDir);
     string get_state_short(); 
     int global_id;
     int local_id; // local to branches generated at a HADDR
    private:
      static ofstream sys_state_debug_file;
      static long sys_state_count;
      static map <string, int> HADDR_to_branch_count;
  };

}


#endif //__GATE_H__
