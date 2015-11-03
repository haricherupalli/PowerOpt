#ifndef __NET_H__
#define __NET_H__

#include <string>
#include "Pin.h"
#include "typedefs.h"
#include "Terminal.h"
#include "Pad.h"

using namespace std;

namespace POWEROPT {

static bool replace_substr (std::string& str,const std::string& from, const std::string& to)
{
  size_t start_pos = str.find(from);
  bool found = false;
  size_t from_length = from.length();
  size_t to_length = to.length();
  while (start_pos != string::npos)
  {
    str.replace(start_pos, from.length(), to);
    size_t adjustment =  (to_length>from_length)?(to_length - from_length):0;
    start_pos = str.find(from, start_pos+ adjustment +1);
    found = true;
  }
  return found;
/*  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;*/

}


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
        vcdAbbrev = "";
      }
      Net(int nId, string nName):id(nId), name(nName)
      {
        replace_substr(name, "\[", "_");
        driver_gate = NULL;
        prev_val = "x";
        new_val = "x";
        sim_val = "X";
        sim_ignore_once = true;
        toggled  = false;
        topo_sort_marked = false;
        vcdAbbrev = "";
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
      string getVCDAbbrev() { return vcdAbbrev; }
      void setVCDAbbrev(string new_abbrev) { vcdAbbrev = new_abbrev; }
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
      string vcdAbbrev;
      string new_val;// these values are used for reading vcd;
      ToggleType toggle_type;
      string sim_val;// value used for simulation (exe_op = 20)
      ToggleType sim_toggle_type;
    };
}
#endif //__NET_H__
