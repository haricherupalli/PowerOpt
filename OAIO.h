#ifndef __OAIO_H__
#define __OAIO_H__

#include <cassert>
#include <set>
#include "PowerOpt.h"


using namespace std;

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
    void fill_reg_cells_list(PowerOpt *po);

  private:
    set<string> reg_cells_set;

  };

}

#endif //__OAIO_H__
