#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

#include "xbitset.h"


namespace POWEROPT {

  class system_state{

    public:
     system_state();
     system_state(vector<Net*>& nets, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf_inp, map<int, xbitset>& DMemory_inp, int cycle_num_inp, string PC_inp, string instr_inp, bool inp_dep);
     int cycle_num;
     map<int, string> net_sim_value_map;// <net_id, sim_value>
     priority_queue<GNode*, vector<GNode*>, sim_wf_compare> sim_wf;
     map<int, xbitset> DMemory;
     string PC;
     string instr;
     bool taken;
     bool not_taken;
     bool inp_dependent;
     bool compare_and_update_state(system_state& sys_state);
     void openFiles(string outDir);
     string get_state_short(); 
     int id;
    private:
      static ofstream sys_state_debug_file;
      static long sys_state_count;
  };

}


#endif //__GATE_H__
