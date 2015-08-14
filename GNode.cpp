
#include <iostream>
#include "GNode.h"
#include "Graph.h"

using namespace std;

namespace POWEROPT {
    
    string GNode::getName()
    {
        assert(!(isPad&isGate));
        if (isPad)  return pad->getName();
        if (isGate)  return gate->getName();
    }

    bool GNode::allInputsReady()
    {
       assert(isGate);
       //assert(!isSource);

       for (int i = 0; i < gate->getFaninTerminalNum(); i++)
       {
            Terminal * term = gate->getFaninTerminal(i);
            //assert(term->getNetNum() == 2);
            Net* net = term->getNet(0); 
            if (net->getExpr().length() == 0) return false;
       }
       return true;
    }
    
    bool GNode::getIsFF()
    {
        assert(isGate);
        return gate->getFFFlag();
    }
    
    void GNode::checkTopoOrder()
    {
        if (isPad || getIsFF() ) return;
        vector<int> driver_topo_ids;
        gate->getDriverTopoIds(driver_topo_ids);
        for (int i=0; i < driver_topo_ids.size(); i++)
        {
            int driver_id = driver_topo_ids[i]; 
            if (driver_id >= topo_id ) 
            {
              cout << " FAILED for gate " << gate->getName() << " With id " << id << " and top id " << gate->getTopoId() <<  endl;
              assert(0);
            }
        }
    }
    
    string GNode::getSimValue()
    {
        if(isPad) return pad->getSimValue();
        return gate->getSimValue();
    }
}
