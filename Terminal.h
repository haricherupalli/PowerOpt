#ifndef __TERMINAL_H__
#define __TERMINAL_H__
#include <string>

#include "typedefs.h"
#include "Gate.h"
#include "Pin.h"
#include "Pad.h"
#include <queue>
#include <vector>
#include <functional>

namespace POWEROPT {

  
  class Pin;
  class Terminal {
  public:
    Terminal(int tId):id(tId), riseCap(-1), fallCap(-1), riseSlew(-1), fallSlew(-1), disable(false)
      {
        prev_val = "x";
        new_val = "x";
        toggle_type = UNKN;
      }
    Terminal(int tId, string tName):id(tId), name(tName), riseCap(-1), fallCap(-1), riseSlew(-1), fallSlew(-1), disable(false)
      {
        prev_val = "x";
        new_val = "x";
        toggle_type = UNKN;
      }
    
    //modifiers
    void setGate(Gate *g) { gatePtr = g; }
    void setPinX(int x) { pin.setX(x); }
    void setPinY(int y) { pin.setY(y); }
    void setDelay(double d) { delay = d; }
    void setRiseSlew(double r) { riseSlew = r; }
    void setFallSlew(double f) { fallSlew = f; }
    void setRiseCap(double r) { riseCap = r; }
    void setFallCap(double f) { fallCap = f; }
    void addNet(Net *n) { nets.push_back(n); }
    void setPinType(PinType t) { pin.setType(t); }
    //accessors
    int getId() { return id; }
    string getName() { return name; }
    string getFullName() { return fullName; }
    Gate *getGate() { return gatePtr; }
    int getNetNum() { return nets.size(); }
    Net *getNet(int index) { assert(0 <= index && index < (int)nets.size()); return nets[index]; }
    PinType getPinType() { return pin.getType(); }
    double getRiseSlew() { return riseSlew ; }
    double getFallSlew() { return fallSlew; }
    double getRiseCap() { return riseCap; }
    double getFallCap() { return fallCap; }
    double getLoadCap() { double cap = (riseCap > fallCap?riseCap:fallCap); assert(cap >= riseCap && cap >= fallCap); return cap; }
    double getInputSlew() { double slew = (riseSlew > fallSlew?riseSlew:fallSlew); assert(slew >= riseSlew && slew >= fallSlew); return slew; }
    double getDelay() { return delay; }
    int getSubnetNum() { return subnets.size(); }
    Subnet *getSubnet(int index) { assert(0 <= index && index < (int)subnets.size()); return subnets[index]; }
    void addSubnet(Subnet *s) { subnets.push_back(s); }
    void print();
    void printNets(ostream& debug_file);
    void setDisable(bool b) { disable = b; }
    bool isDisabled() { return disable; }
    int getPinX() { return pin.x(); }
    int getPinY() { return pin.y(); }
    Pin* getPin() {return &pin;}
    void setToggled (bool val, string to);
    void setToggled (bool val) { toggled = val;}
    bool isToggled() { return toggled; }
    ToggleType getToggType() { return toggle_type;}
    string getToggledTo() { return  new_val;}
    string getPrevVal() { return prev_val; }
    void computeFullName();// { fullName = gatePtr->getName()+"/"+name;}
    bool isMuxOutput();
    void inferToggleFromNet();
    string getExpr() ;
    void setExpr(string Expr);
    string getNetName(); 
    void setSimValue(string value, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
    void setSimValue(string value);
    string getSimValue() ;
    ToggleType getSimToggType() ;
    
  private:
    int id;
    string name;
    string fullName;
    Gate *gatePtr;
    Pin pin;
    NetVector nets;
    string expr;
    double riseCap;
    double fallCap;
    double riseSlew;
    double fallSlew;
    double delay;
    SubnetVector subnets;
    bool disable;
    string prev_val;
    string new_val; 
    ToggleType toggle_type;
    bool toggled;
    string sim_val;
  };
  
}
#endif //__TERMINAL_H__
