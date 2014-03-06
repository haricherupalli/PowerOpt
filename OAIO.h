#ifndef __OAIO_H__
#define __OAIO_H__

#include <cassert>
#include "PowerOpt.h"

namespace POWEROPT {
  
  class OAReader {
  public:
    //constructors
    OAReader() {}
    
    //modifiers
    //accossers
    //functions
    bool readDesign(PowerOpt *po);
    void updateModel(PowerOpt *op, bool swap);
    void updateModelTest(PowerOpt *op);
    void updateCoord(PowerOpt *op, bool all);
    
  private:
  };
  
}

#endif //__OAIO_H__
