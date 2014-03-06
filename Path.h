#ifndef __PATH_H__
#define __PATH_H__

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <math.h>
#include "Gate.h"
#include "typedefs.h"

namespace POWEROPT {
  class Gate;
  
  class Path {
  public:
    //constructors
    Path():swapCellNum(0) { }
    Path(int i):id(i), ignoreFlag(false), swapCellNum(0) { }
    Path(int i, double s, double t):id(i), slack(s), treq(t), swapCellNum(0) { }
    ~Path() { gates.clear(); }
    //modifiers
    void setId(int i) { id = i; }
    bool addGate(Gate *g);
    bool addGateInDiffGrid(Gate *g);
    void setSlack(double s) { slack = s; }
    void setVariation(double v) { delayVariation = v; }
    void setSlackOrg(double s) { slack_org = s; slack = s;}
    void setTReq(double t) { treq = t; }
    void calDelay();
    void calOptDelay();
    double setDelay(double d) { delay = d; }
    double setMCT(double m) { mct = m; }
    void addCycle(int cycle_num) { toggle_cycles.push_back(cycle_num); }
    //accossers
    int getNumToggled() { return toggle_cycles.size(); }
    vector<int>::iterator getToggleCycles() { return toggle_cycles.begin(); }
    double getMCT() { return mct; }
    int getId() { return id; }
    Gate* getGate(int i) { assert(0 <= i && i < gates.size()); return gates[i]; }
    int getGateNum() { return gates.size(); }
    GateVector &getGates() { return gates; }
    double getSlack() { return slack; }
    double getVariation() { return delayVariation; }
    double getSlackOrg() { return slack_org; }
    double getTReq() { return treq; }
    double getDelay() { return delay; }
    double getOptDelay() { return optDelay; }
    void incSwappedCell() { swapCellNum ++; }
    void decSwappedCell() { swapCellNum --; }
    int getSwappedCellNum() { return swapCellNum; }
    void fixCells();
    void setPathName(string pName) { pathName = pName;}
    string getPathName() { return pathName;}
    void setPathStr(string pName) { pathStr = pName;}
    void setPathRegStr(string pName) { pathRegStr = pName;}
    string getPathStr() { return pathStr;}
    string getPathRegStr() { return pathRegStr;}
    void    setDeltaER(int val) { deltaER = val;}
    int     getDeltaER() { return deltaER;}
    void    setSwActivity(double val) { swActivity = val; swActivity2 = val;}
    void    setSwActivity2(double val) { swActivity2 = val;}
    double  getSwActivity() { return swActivity;}
    double  getSwActivity2() { return swActivity2;}
    double  getSwSlack()    { return swActivity*slack*((double)-1);}
    void    setSwActivityFF(double val) { swActivityFF = val;}
    double  getSwActivityFF() { return swActivityFF;}
	void    setIgnore(bool b) { ignoreFlag = b; }
	bool    isIgnore() { return ignoreFlag; }
    void print();
    
  private:
    vector<int> toggle_cycles; // keeps track of cycles in which the path toggles
    int id;
    GateVector gates;
    double slack;
    double delayVariation; // this is the sum of delayVariation of the gates in the path (plus wire delay variation?)
    double slack_org;
    double mct;
    double treq;
    double delay;
    double optDelay;
    int swapCellNum;
    string pathName;
    string pathStr;
    string pathRegStr;
    double swActivity;          // Switching Activity
    double swActivity2;          // Switching Activity
    double swActivityFF;        // Switching Activity (F/F)
    int deltaER;
    bool ignoreFlag;
  };


  struct PathDeltaERSorterS2L : public std::binary_function<Path*,Path*,bool>
  {
    bool operator()(Path *p1, Path *p2) const
    {
      return (p1->getDeltaER() < p2->getDeltaER());
    };
  };

  class PathSlackS2L {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getSlack() < p2->getSlack();
  		}
  };
  class PathDelayL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getDelay() > p2->getDelay();
  		}
  };
  class PathSwL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getSwActivity() > p2->getSwActivity();
  		}
  };
  class PathTgL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getNumToggled() > p2->getNumToggled();
  		}
  };
  class PathTgPerNegSlackL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return (double(p1->getNumToggled()))/(fabs(p1->getSlack())) > (double(p2->getNumToggled()))/(fabs(p2->getSlack()));
  		}
  };
  class PathDeltaERS2L {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getDeltaER() < p2->getDeltaER();
  		}
  };
  class PathSwSlackL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getSwSlack() > p2->getSwSlack();
  		}
  };
  class PathSwFFL2S {
  	public:
  		inline bool operator () (Path *p1, Path *p2)
  		{
  			return p1->getSwActivityFF() > p2->getSwActivityFF();
  		}
  };

  class Divnet {
  public:
    //constructors
    Divnet() { }
    Divnet(int i):id(i) { }
    ~Divnet() { gates.clear(); ff_src.clear(); ff_des.clear(); }
    int getId() { return id; }
    int getPartNum() { return partNum; }
    void setPartNum( int x ) { partNum = x; }   
    Gate* getGate(int i) { 
        assert(0 <= i && i < gates.size()); 
        return gates[i]; 
    }
    Gate* getFF_src(int i) { 
        assert(0 <= i && i < ff_src.size()); 
        return ff_src[i]; 
    }
    Gate* getFF_des(int i) { 
        assert(0 <= i && i < ff_des.size()); 
        return ff_des[i]; 
    }
    bool add_gate(Gate *g){
        //GateVectorItr itr;
        //for (itr = gates.begin(); itr != gates.end(); itr++) {
        //    if ((*itr)->getId() == g->getId()) return false;
        // }
        gates.push_back(g);
        return true;
    }
    bool addFF_src(Gate *g){
        GateVectorItr itr;
        for (itr = ff_src.begin(); itr != ff_src.end(); itr++) {
            if ((*itr)->getId() == g->getId()) return false;
        }
        if (itr == ff_src.end())ff_src.push_back(g);
        return true;
    }
    bool addFF_des(Gate *g){
        GateVectorItr itr;
        for (itr = ff_des.begin(); itr != ff_des.end(); itr++) {
            if ((*itr)->getId() == g->getId()) return false;
        }
        if (itr == ff_des.end())ff_des.push_back(g);
        return true;
    }
    int getGateNum()   { return gates.size(); }
    int getFF_srcNum() { return ff_src.size(); }
    int getFF_desNum() { return ff_des.size(); }
    string getFF_srcName() {
        std::ostringstream str;
        str << "scr_" << id;
        return str.str(); 
    }
    string getFF_desName() {
        std::ostringstream str;
        str << "des_" << id;
        return str.str(); 
    }
  private:
    int id;
    int partNum;
    GateVector ff_src;
    GateVector ff_des;
    GateVector gates;
};
  class DivnetSizeL2S {
  	public:
  		inline bool operator () (Divnet *d1, Divnet *d2)
  		{
  			return d1->getGateNum() > d2->getGateNum();
  		}
  };

}
#endif //__PATH_H__
