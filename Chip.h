#ifndef __CHIP_H__
#define __CHIP_H__

#include <cassert>
#include "Gate.h"
#include "typedefs.h"

namespace POWEROPT {
  class Gate;
  
  class Chip {
  public:
    //constructors
    Chip() { }
    Chip(int i, int rNum, int cNum, int rW, int rH, int x1, int y1, int x2, int y2) : id(i), siteRowNum(rNum), siteColNum(cNum), lx(x1), rx(x2), by(y1), ty(y2)
    {
    }
    //modifiers
    void setId(int i) { id = i; }
    void addGate(Gate *g) { gates.push_back(g); }
    void removeGate(int gId);
    void setDim(int x1, int y1, int x2, int y2)
      {
        lx = x1;
        rx = x2;
        by = y1;
        ty = y2;
        width = x2-x1;
        height = y2-y1;
      }
    //accossers
    int getId() { return id; }
    int getLX() { return lx; }
    int getRX() { return rx; }
    int getBY() { return by; }
    int getTY() { return ty; }
    Gate* getGate(int i) { assert(0 <= i && i < (int)gates.size()); return gates[i]; }
    GateVector &getGates() { return gates; }
    int getGateNum() { return gates.size(); }
    void setRowW(int w) { rowW = w; }
    void setRowH(int h) { rowH = h; }
    int getRowW() { return rowW; }
    int getRowH() { return rowH; }
		void setSiteRowNum(int n) { siteRowNum = n; }
		void setSiteColNum(int n) { siteColNum = n; }
		int getSiteRowNum() { return siteRowNum; }
		int getSiteColNum() { return siteColNum; }
		int getWidth() { return rx - lx + 1; }
		int getHeight() { return ty - by + 1; }
		void initSiteOrient(int numH) { siteOrient.assign(numH, 0); }
		void setSiteOrient(int index, int value) { assert(0 <= index && index <= siteOrient.size()); siteOrient[index] = value; }
		int getSiteOrient(int index) { assert(0 <= index && index <= siteOrient.size()); return siteOrient[index]; }
		void setChipLeft(int l) { lx = l; }
		void setChipRight(int r) { rx = r; }
		void setChipBottom(int b) { by = b; }
		void setChipTop(int t) { ty = t; }
		void setChipX(int x) { width = x; }
		void setChipY(int y) { height = y; }
		int getChipLeft() { return lx; }
		int getChipRight() { return rx; }
		int getChipBottom() { return by; }
		int getChipTop() { return ty; }
		int getChipX() { return width; }
		int getChipY() { return height; }
		void setLefDefFactor(int i) { m_lefDefFactor = i; }
		int getLefDefFactor() { return m_lefDefFactor; }
    void print();
    
  private:
    int id;
    int siteRowNum, siteColNum;
    int rowW, rowH;
    int lx;
    int rx;
    int by;
    int ty;
    int width, height;
    GateVector gates;
		IntVector siteOrient;
		int m_lefDefFactor;
  };

  class GateLSiteColS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getLSiteColIndex() < g2->getLSiteColIndex();
  		}
  };
  class GateBSiteRowS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getBSiteRowIndex() < g2->getBSiteRowIndex();
  		}
  };

}

#endif //__CHIP_H__
