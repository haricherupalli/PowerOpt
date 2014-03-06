#ifndef __PIN_H__
#define __PIN_H__
#include <string>

#include "Box.h"
#include "typedefs.h"
#include "macros.h"

using namespace std;

namespace POWEROPT {
  
  enum PinType { INPUT, OUTPUT, INOUT, UNKNOWN };
  
  class Pin {
  public:
    Pin() {}
    Pin(int pId, string pName, PinType t, int leftIn, int bottomIn, int rightIn, int topIn)
      :id(pId), name(pName), type(t), bbox(leftIn, bottomIn, rightIn, topIn), centerX((leftIn+rightIn)/2), centerY((bottomIn+topIn)/2)
      {
      }
    
    //modifiers
    void setX(int x) { centerX = x; }
    void setY(int y) { centerY = y; }
    void setType(PinType t) { type = t; }
    //accessors
    int getId() { return id; }
    string getName() { return name; }
    int &x() { return centerX; }
    int &y() { return centerY; }
    PinType getType() { return type; }
    
    void print();
    
  private:
    int id;
    string name;
    int netId;
    Box bbox;
    PinType type;
    int centerX, centerY;
  };
  
  class PinXS2L
    {
    public:
      inline bool operator()(Pin *pin1, Pin *pin2) { return pin1->x() < pin2->x(); }
    };
  
  class PinYS2L
    {
    public:
      inline bool operator()(Pin *pin1, Pin *pin2) { return pin1->y() < pin2->y(); }
    };
}
#endif //__PIN_H__
