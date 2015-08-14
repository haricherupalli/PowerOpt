#ifndef __GNODE_H__
#define __GNODE_H__

#include <cassert>
#include <cfloat>
#include <math.h>
#include <queue>
#include "typedefs.h"

namespace POWEROPT {
  class GNode {
    GNode()
    {
      gate = NULL;
      pad = NULL;
      isSource = false;
      isSink= false;
      isGate = false;
      isPad = false;
      visited = false;
      //isNode= false;
    }
    public:
    void setGate(Gate* g) { gate = g; }
    void setPad(Pad* p) { pad = p;}
    void setIsSource(bool val) { isSource = val; }
    void setIsSink(bool val) { isSink = val; }
    bool getIsSource () { return isSource; }
    bool getIsSink () { return isSink; }

    void setIsPad(bool val) { isPad = val; }
    bool getIsPad() { return isPad; }
    void setIsGate(bool val) { isGate = val; }
    bool getIsGate() { return isGate; }

    void setVisited( bool val ) { visited = val; }
    bool getVisited() { return visited; }

    bool isInWf() { return inWf;}
    void setInWf(bool val) { inWf = val; }

    Pad* getPad() { return pad; }
    Gate* getGate() { return gate; }
    void setId(int val) { id = val; }
    int getId() { return id; }
    void setTopoId(int val) { topo_id = val; }
    int getTopoId() const { return topo_id; }
    string getName() ;
    bool allInputsReady();
    bool getIsFF();
    void checkTopoOrder();
    string getSimValue(); 
    //void setIsNode(bool val) { isNode = val; }
    private:
    Pad* pad;
    Gate* gate;
    bool inWf;
    bool isPad;
    bool isGate;
    bool isSource;
    bool isSink;
    bool isNode   ; // neither source nor sink
    bool visited;
    int id;
    int topo_id;
  } ;
}

#endif
