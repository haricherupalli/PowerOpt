#ifndef __PAD_H__
#define __PAD_H__
#include <string>

#include "typedefs.h"
#include "Pin.h"
#include "Box.h"
#include "Net.h"
#include "Subnet.h"
#include "Graph.h" // Class GNode, Graph

namespace POWEROPT {
  enum PadType {PrimiaryInput, PrimiaryOutput, InputOutput};
  class sim_wf_compare ;
  
  class Pad {
  public:
    Pad(int tId, string tName):id(tId), name(tName), delay(0)
    {
    }
    Pad(int tId, string tName, PadType t):id(tId), name(tName), type(t), delay(0)
    {
    }
    Pad(int tId, string tName, PadType t, int lx, int by, int rx, int ty):id(tId), name(tName), type(t), delay(0)
    {
      bbox.set(lx, by, rx, ty);
      pin.setX((lx+rx)/2);
      pin.setY((by+ty)/2);
    }
    Pad(int tId, string tName, PadType t, double d, int lx, int by, int rx, int ty):id(tId), name(tName), type(t), delay(d)
    {
      bbox.set(lx, by, rx, ty);
      pin.setX((lx+rx)/2);
      pin.setY((by+ty)/2);
    }
    
    //modifiers
    void addNet(Net *n) { nets.push_back(n); }
    //accessors
    int getId() { return id; }
    string getName() { return name; }
    PadType getType() { return type; }
    int getLX() { return bbox.left(); }
    int getRX() { return bbox.right(); }
    int getBY() { return bbox.bottom(); }
    int getTY() { return bbox.top(); }
    void setPinX(int x) { pin.setX(x); }
    void setPinY(int y) { pin.setY(y); }
    int getNetNum() { return nets.size(); }
    Net *getNet(int index) { assert(0 <= index && index < nets.size()); return nets[index]; }
    void setDelay(double d) { delay = d; }
    double getDelay() { return delay; }
    void addSubnet(Subnet *s) { m_subnets.push_back(s); }
    int getSubnetNum() { return m_subnets.size(); }
    Subnet *getSubnet(int index) { assert(0 <= index && index < m_subnets.size()); return m_subnets[index]; }
    void setGNode(GNode * gnode) { node = gnode; }
    GNode* getGNode () { return node; }
    void print();
    void setExpr();
    void setSimValue(string value, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
    //void setSimValue(string value);
    string getSimValue() ;
    int getTopoId() { return node->getTopoId(); }
    ToggleType getSimToggType() ;
    
  private:
    int id;
    string name;
    Pin pin;
    PadType type;
    Box bbox;
    NetVector nets;
    double delay;
    SubnetVector m_subnets;
    GNode* node;
  };
}
#endif //__PAD_H__
