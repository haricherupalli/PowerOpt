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
      Net() {}
      Net(int nId, string nName):id(nId), name(nName)
        {
        }
      ~Net() {}
      //modifiers
      void addInputTerminal(Terminal *t) { terms.insert(terms.begin(), t); }
      void addOutputTerminal(Terminal *t) { terms.push_back(t); }
      void addTerminal(Terminal *t) { terms.push_back(t); }
      void addPad(Pad *p) { pads.push_back(p); }
      int getTerminalNum() { return terms.size(); }
      Terminal *getTerminal(int index) { assert(0 <= index && index < terms.size()); return terms[index]; }
      Terminal *getInputTerminal();
      void makeFalseInputTerminal();
      int getPadNum() { return pads.size(); }
      Pad *getPad(int index) { assert(0 <= index && index < pads.size()); return pads[index]; }
      
      //accessors
      int getId() { return id; }
      string getName() { return name; }
      //algs
      void print();
    private:
      int id;
      string name;
      TerminalVector terms;
      PadVector pads;
    };
}
#endif //__NET_H__
