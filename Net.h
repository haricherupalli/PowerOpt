#ifndef __NET_H__
#define __NET_H__

#include <string>
#include "Pin.h"
#include "typedefs.h"
#include "Terminal.h"
#include "Pad.h"

using namespace std;

namespace POWEROPT {
  class Net
    {
    public:
      Net() {
        driver_gate  = NULL;
        prev_val = "x";
        new_val = "x";
        sim_val = "X";
        sim_ignore_once = true;
        toggled = false;
        topo_sort_marked = false;
      }
      Net(int nId, string nName):id(nId), name(nName)
      {
        driver_gate = NULL;
        prev_val = "x";
        new_val = "x";
        sim_val = "X";
        sim_ignore_once = true;
        toggled  = false;
        topo_sort_marked = false;
      }
      ~Net() {}
      //modifiers
      void addInputTerminal(Terminal *t) { terms.insert(terms.begin(), t); inp_terms.push_back(t);}
      void addOutputTerminal(Terminal *t) { terms.push_back(t); out_terms.push_back(t);}
      void addTerminal(Terminal *t) { terms.push_back(t); }
      void addPad(Pad *p) { pads.push_back(p); }
      void setDriverGate(Gate* g) { driver_gate = g; }
      Gate* getDriverGate() { return driver_gate; }
      int getTerminalNum() { return terms.size(); }
      Terminal *getTerminal(int index) { assert(0 <= index && index < terms.size()); return terms[index]; }
      Terminal *getInputTerminal();
      Terminal *getOutputTerminal(); 
      void makeFalseInputTerminal();
      int getPadNum() { return pads.size(); }
      Pad *getPad(int index) { assert(0 <= index && index < pads.size()); return pads[index]; }
      void addExpression(string expr) { Expr.append("("+expr+")");}
      string getExpr() {return Expr; }
      void setExpr(string expr) { Expr = expr; }
      void updateValue(string to) { new_val = to; }
      void setTopoSortMarked(bool val) { topo_sort_marked = val; }
      static void flip_check_toggles();
      void addFanoutGate(Gate* g);
      int getFanoutGateNum() { return fanout_gates.size(); }
      Gate* getFanoutGate(int i ) { return fanout_gates[i]; }
      
      //accessors
      int getId() { return id; }
      string getName() { return name; }
      //algs
      void print();
      void updateToggled();
      void updatePrevVal() { prev_val = new_val ; }
      void setToggled(bool val) { toggled = val; }
      string getPrevVal() { return prev_val; }
      string getToggledTo() { return  new_val;}
      bool getToggled() { return toggled; }
      ToggleType getToggleType() { return toggle_type; }
      bool getTopoSortMarked() { return topo_sort_marked; }
      bool setSimValue(string value); 
      string getSimValue() { return sim_val; }
      ToggleType getSimToggType() { return sim_toggle_type;}

    private:
      int id;
      string name;
      bool sim_ignore_once;
      TerminalVector terms;
      TerminalVector inp_terms;
      TerminalVector out_terms;
      GateVector fanout_gates;
      bool topo_sort_marked;
      PadVector pads;
      Gate* driver_gate;
      string Expr;
      bool toggled;
      string prev_val;
      string new_val;// these values are used for reading vcd;
      ToggleType toggle_type;
      string sim_val;// value used for simulation (exe_op = 20)
      ToggleType sim_toggle_type;
    };
}
#endif //__NET_H__
