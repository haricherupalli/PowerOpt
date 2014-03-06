#ifndef __AREA_H__
#define __AREA_H__

#include <cassert>
#include "Gate.h"
#include "typedefs.h"

namespace POWEROPT {
  class Gate;
  
  class Grid {
  public:
    //constructors
    Grid() { }
    Grid(int i):id(i) { }
    Grid(int i, int x1, int y1, int x2, int y2):id(i), lx(x1), rx(x2), by(y1), ty(y2) { }
    Grid(int i, int rIndex, int cIndex, int x1, int y1, int x2, int y2):id(i), rowIndex(rIndex), colIndex(cIndex), lx(x1), rx(x2), by(y1), ty(y2) { }
    //modifiers
    void setId(int i) { id = i; }
    void addGate(Gate *g) { gates.push_back(g); }
    void removeGate(int gId);
    void setDoseL(double d) { m_doseL = d; }
    void setDoseW(double d) { m_doseW = d; }
    void setDim(int x1, int y1, int x2, int y2)
    {
      lx = x1;
      rx = x2;
      by = y1;
      ty = y2;
    }
    //accossers
    int getId() { return id; }
    int getLX() { return lx; }
    int getRX() { return rx; }
    int getBY() { return by; }
    int getTY() { return ty; }
    int getWidth() { return rx - lx; }
    int getHeight() { return ty - by; }
    Gate* getGate(int i) { assert(0 <= i && i < (int)gates.size()); return gates[i]; }
    GateVector &getGates() { return gates; }
    int getGateNum() { return gates.size(); }
    int getRowIndex() { return rowIndex; }
    int getColIndex() { return colIndex; }
    double getDoseL() { return m_doseL; }
    double getDoseW() { return m_doseW; }
    void print();
    
  private:
    int id;
    int rowIndex;
    int colIndex;
    int lx;
    int rx;
    int by;
    int ty;
    GateVector gates;
    double m_doseL;
    double m_doseW;
  };

  class GridDoseLL2S {
	public:
		inline bool operator () (Grid *g1, Grid *g2)
		{
			return g1->getDoseL() > g2->getDoseL();
		}
	};
	
  class GridDoseWL2S {
	public:
		inline bool operator () (Grid *g1, Grid *g2)
		{
			return g1->getDoseW() > g2->getDoseW();
		}
	};

}

#endif //__AREA_H__
