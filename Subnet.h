#ifndef __SUBNET_H__
#define __SUBNET_H__

#include <string>
#include "Pin.h"
#include "typedefs.h"
#include "Terminal.h"
#include "Pad.h"

using namespace std;

namespace POWEROPT {
  class Subnet
    {
    public:
      Subnet() {}
      Subnet(int sId, string sName):id(sId), name(sName), delay(0), inputTerm(NULL), outputTerm(NULL)
        {
        }
      Subnet(int sId, string sName, bool iPad, bool oPad):id(sId), name(sName), iPad(iPad), oPad(oPad), inputTerm(NULL), outputTerm(NULL), inputPad(NULL), outputPad(NULL), delay(0) {  }
      ~Subnet() {}
      //modifiers
      void setInputTerminal(Terminal *t) { inputTerm = t; }
      void setOutputTerminal(Terminal *t) { outputTerm = t; }
      void setInputPad(Pad *p) { inputPad = p; }
      void setOutputPad(Pad *p) { outputPad = p; }
      Terminal *getInputTerminal()
        {
          assert(!iPad);
          return inputTerm;
        }

      Terminal *getOutputTerminal()
        {
          assert(!oPad);
          return outputTerm;
        }
      
      Pad *getInputPad()
        {
          assert(iPad);
          return inputPad;
        }
      
      Pad *getOutputPad()
        {
          assert(oPad);
          return outputPad;
        }
      bool inputIsPad() { return iPad; }
      bool outputIsPad() { return oPad; }
      void setInputIsPad(bool b) { iPad = b; }
      void setOutputIsPad(bool b) { oPad = b; }
      
      void setDelay(double d) { delay = d; }
      
      //accessors
      int getId() { return id; }
      string getName() { return name; }
      double getDelay() { return delay; }
      
      //algs
      void print();
    private:
      int id;
      string name;
      Terminal *inputTerm;
      Terminal *outputTerm;
      Pad *inputPad;
      Pad *outputPad;
      bool iPad;
      bool oPad;
      double delay;
    };
}
#endif //__SUBNET_H__
