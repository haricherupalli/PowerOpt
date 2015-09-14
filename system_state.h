#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__


namespace POWEROPT {

  class system_state{

    public:
     int cycle_num;
     map<int, string> net_sim_value_map;// <net_id, sim_value>
     priority_queue<GNode*, vector<GNode*>, sim_wf_compare> sim_wf; 
     map<int, xbitset> DMemory;
  };

}


#endif //__GATE_H__