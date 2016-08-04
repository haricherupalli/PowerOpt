#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

#include "xbitset.h"


namespace POWEROPT {

  class system_state{

    public:
     system_state() { taken = false; not_taken = false; }
     int cycle_num;
     map<int, string> net_sim_value_map;// <net_id, sim_value>
     priority_queue<GNode*, vector<GNode*>, sim_wf_compare> sim_wf;
     map<int, xbitset> DMemory;
     string PC;
     bool taken;
     bool not_taken;
     bool compare_and_update_state(system_state& sys_state);
     void openFiles(string outDir);
    private:
      static ofstream sys_state_debug_file;
  };

}


#endif //__GATE_H__
