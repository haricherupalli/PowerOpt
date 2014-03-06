#ifndef __TERMINAL_H__
#define __TERMINAL_H__
#include <string>

#include "typedefs.h"
#include "Gate.h"
#include "Pin.h"

namespace POWEROPT {
  
  class Pin;
  class Terminal {
  public:
    Terminal(int tId):id(tId), riseCap(-1), fallCap(-1), riseSlew(-1), fallSlew(-1), disable(false)
      {
      }
    Terminal(int tId, string tName):id(tId), name(tName), riseCap(-1), fallCap(-1), riseSlew(-1), fallSlew(-1), disable(false)
      {
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
    void setDisable(bool b) { disable = b; }
    bool isDisabled() { return disable; }
    int getPinX() { return pin.x(); }
    int getPinY() { return pin.y(); }
    
  private:
    int id;
    string name;
    Gate *gatePtr;
    Pin pin;
    NetVector nets;
    double riseCap;
    double fallCap;
    double riseSlew;
    double fallSlew;
    double delay;
    SubnetVector subnets;
    bool disable;
  };
  
}
#endif //__TERMINAL_H__
