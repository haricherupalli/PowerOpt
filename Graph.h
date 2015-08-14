#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <cassert>
#include <cfloat>
#include <math.h>
#include <queue>
#include "Grid.h"
#include "Box.h"
#include "typedefs.h"
#include "Subnet.h"
#include "Net.h"
#include "GNode.h"

namespace POWEROPT {
/*  class GNode {
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

    Pad* getPad() { return pad; }
    Gate* getGate() { return gate; }
    void setId(int val) { id = val; }
    int getId() { return id; }
    //void setIsNode(bool val) { isNode = val; }
    private:
    Pad* pad;
    Gate* gate;
    bool isPad;
    bool isGate;
    bool isSource ;
    bool isSink   ;
    bool isNode   ; // neither source nor sink
    bool visited;
    int id;
  } ;*/

  class Graph {

    public:
      void addSource(GNode* src) { sources.push_back(src); }
      void addSink(GNode* sink) { sinks.push_back(sink); }
      void addNode(GNode* node) { node->setId(nodes.size()); nodes.push_back(node); }
      void addWfNode(GNode* wf_node) { wf_node-> setInWf(true); sink_wf.push(wf_node);}
      void popWfNode() {sink_wf.front()->setInWf(false); sink_wf.pop();}
      int getWfSize() { return sink_wf.size(); }
      vector<GNode*>& getNodeVec() { return nodes; }
      GNode* getWfNode() { return sink_wf.front();}
      vector<GNode*> & getSources() { return sources; }
      void clear_visited();
      void print();
      void print_sources();
      void print_wf();
    private:
      //queue<GNode*> src_wf;
      queue<GNode*> sink_wf;
      vector<GNode*> sources;
      vector<GNode*> sinks;
      vector<GNode*> nodes;
  };

  class sim_wf_compare 
  {
    public:
      //sim_wf_compare(const bool& revparam=true) {reverse=revparam;}
      inline bool operator() (GNode* lhs,  GNode*rhs) 
      {
        return (lhs->getTopoId() > rhs->getTopoId());
      }
  };
}

#endif // __GRAPH_H__
