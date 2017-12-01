#ifndef __GATE_H__
#define __GATE_H__

#include <cassert>
#include <cfloat>
#include <math.h>
#include <bitset>
#include <string>
#include "Grid.h"
#include "Box.h"
#include "typedefs.h"
#include "Subnet.h"
#include "Net.h"
#include "analyzeTiming.h"
#include "Graph.h" // Class GNode, Graph

namespace POWEROPT {

  class Grid;
  class Terminal;
  enum GateType {GATEPI, GATEPO, GATEIN, GATEUNKNOWN, GATEPADPI, GATEPADPO, GATEPADPIO};
  enum CellFunc { AND, AOI, BUFF, DFF, INV, LH, MUX, NAND, NOR, OAI, OR, XNOR, XOR,
/* New adds */    ADDF, ADDFH, ADDH, AND2, AND3, AND4, AO21, AO22, AO2B2, AO2B2B, AOI21, AOI211, AOI21B, AOI22, AOI221, AOI222, AOI2B1, AOI2BB1, AOI2BB2, AOI31, AOI32, AOI33, BUF, MX2, MX3, MX4, MXI2, MXI2D, MXI3, MXI4, NAND2, NAND2B, NAND3, NAND3B, NAND4, NAND4B, NAND4BB, NOR2, NOR2B, NOR3, NOR3B, NOR4, NOR4B, NOR4BB, OA21, OA22, OAI21, OAI211, OAI21B, OAI22, OAI221, OAI222, OAI2B1, OAI2B11, OAI2B2, OAI2BB1, OAI2BB2, OAI31, OAI32, OAI33, OR2, OR3, OR4, SDFF, SDFFH, SDFFHQ, SDFFHQN, SDFFNH, SDFFNSRH, SDFFQ, SDFFQN, SDFFR, SDFFRHQ,  SDFFRQ, SDFFS, SDFFSHQ, SDFFSQ, SDFFSR, SDFFSRHQ, SDFFTR, SEDFF, SEDFFHQ, SEDFFTR, SMDFFHQ, TBUF, TIEHI, TIELO, TLAT, TLATN, TLATNCA, TLATNSR,  TLATNTSCA, TLATSR,  XNOR2, XNOR3, XOR2, XOR3,
/* New of PL*/    BUFZ, DLY4, DFFQ, DFFRPQ, PREICG, SDFFRPQ, RF1R1WS, RFWL, LATCH};

    static string cellFunc_to_name (CellFunc cellfunc)
    {
      switch (cellfunc)
      {
        case AND       : return "AND";
        case AOI       : return "AOI";
        case BUFF      : return "BUFF";
        case DFF       : return "DFF";
        case INV       : return "INV";
        case LH        : return "LH";
        case MUX       : return "MUX";
        case NAND      : return "NAND";
        case NOR       : return "NOR";
        case OAI       : return "OAI";
        case OR        : return "OR";
        case XNOR      : return "XNOR";
        case XOR       : return "XOR";
        case ADDF      : return "ADDF";
        case ADDFH     : return "ADDFH";
        case ADDH      : return "ADDH";
        case AND2      : return "AND2";
        case AND3      : return "AND3";
        case AND4      : return "AND4";
        case AO21      : return "AO21";
        case AO22      : return "AO22";
        case AO2B2     : return "AO2B2";
        case AO2B2B    : return "AO2B2B";
        case AOI21     : return "AOI21";
        case AOI211    : return "AOI211";
        case AOI21B    : return "AOI21B";
        case AOI22     : return "AOI22";
        case AOI221    : return "AOI221";
        case AOI222    : return "AOI222";
        case AOI2B1    : return "AOI2B1";
        case AOI2BB1   : return "AOI2BB1";
        case AOI2BB2   : return "AOI2BB2";
        case AOI31     : return "AOI31";
        case AOI32     : return "AOI32";
        case AOI33     : return "AOI33";
        case BUF       : return "BUF";
        case MX2       : return "MX2";
        case MX3       : return "MX3";
        case MX4       : return "MX4";
        case MXI2      : return "MXI2";
        case MXI2D     : return "MXI2D";
        case MXI3      : return "MXI3";
        case MXI4      : return "MXI4";
        case NAND2     : return "NAND2";
        case NAND2B    : return "NAND2B";
        case NAND3     : return "NAND3";
        case NAND3B    : return "NAND3B";
        case NAND4     : return "NAND4";
        case NAND4B    : return "NAND4B";
        case NAND4BB   : return "NAND4BB";
        case NOR2      : return "NOR2";
        case NOR2B     : return "NOR2B";
        case NOR3      : return "NOR3";
        case NOR3B     : return "NOR3B";
        case NOR4      : return "NOR4";
        case NOR4B     : return "NOR4B";
        case NOR4BB    : return "NOR4BB";
        case OA21      : return "OA21";
        case OA22      : return "OA22";
        case OAI21     : return "OAI21";
        case OAI211    : return "OAI211";
        case OAI21B    : return "OAI21B";
        case OAI22     : return "OAI22";
        case OAI221    : return "OAI221";
        case OAI222    : return "OAI222";
        case OAI2B1    : return "OAI2B1";
        case OAI2B11   : return "OAI2B11";
        case OAI2B2    : return "OAI2B2";
        case OAI2BB1   : return "OAI2BB1";
        case OAI2BB2   : return "OAI2BB2";
        case OAI31     : return "OAI31";
        case OAI32     : return "OAI32";
        case OAI33     : return "OAI33";
        case OR2       : return "OR2";
        case OR3       : return "OR3";
        case OR4       : return "OR4";
        case SDFF      : return "SDFF";
        case SDFFH     : return "SDFFH";
        case SDFFHQ    : return "SDFFHQ";
        case SDFFHQN   : return "SDFFHQN";
        case SDFFNH    : return "SDFFNH";
        case SDFFNSRH  : return "SDFFNSRH";
        case SDFFQ     : return "SDFFQ";
        case SDFFQN    : return "SDFFQN";
        case SDFFR     : return "SDFFR";
        case SDFFRHQ   : return "SDFFRHQ";
        case SDFFRQ    : return "SDFFRQ";
        case SDFFS     : return "SDFFS";
        case SDFFSHQ   : return "SDFFSHQ";
        case SDFFSQ    : return "SDFFSQ";
        case SDFFSR    : return "SDFFSR";
        case SDFFSRHQ  : return "SDFFSRHQ";
        case SDFFTR    : return "SDFFTR";
        case SEDFF     : return "SEDFF";
        case SEDFFHQ   : return "SEDFFHQ";
        case SEDFFTR   : return "SEDFFTR";
        case SMDFFHQ   : return "SMDFFHQ";
        case TBUF      : return "TBUF";
        case TIEHI     : return "TIEHI";
        case TIELO     : return "TIELO";
        case TLAT      : return "TLAT";
        case TLATN     : return "TLATN";
        case TLATNCA   : return "TLATNCA";
        case TLATNSR   : return "TLATNSR";
        case TLATNTSCA : return "TLATNTSCA";
        case TLATSR    : return "TLATSR";
        case XNOR2     : return "XNOR2";
        case XNOR3     : return "XNOR3";
        case XOR2      : return "XOR2";
        case XOR3      : return "XOR3";
        case BUFZ      : return "BUFZ";
        case DLY4      : return "DLY4";
        case DFFQ      : return "DFFQ";
        case DFFRPQ    : return "DFFRPQ";
        case PREICG    : return "PREICG";
        case SDFFRPQ   : return "SDFFRPQ";
        case RF1R1WS   : return "RF1R1WS";
        case RFWL      : return "RFWL";
        case LATCH     : return "LATCH";
        default        : assert(0);
      }
    }

  class Gate {
  public:
    //constructors
    Gate(int i, bool ffflag, string gName, string cName, double l, int tlx, int tby, int twidth, int theight):id(i), FFFlag(ffflag), name(gName), cellName(cName), biasCellName(cName), bestCellName(cName), channelLength(l), type(GATEUNKNOWN), optChannelLength(l), lx(tlx), by(tby), oldLX(tlx), oldBY(tby), width(twidth), height(theight), weight(0), critical(false), swapped(false), checked(false), exclude(false), checkSrc(false), checkDes(false), toggled(false), holded(false), fixed(false), div(NULL), div_src(NULL), div_des(NULL), leakWeight(1.0), disable(false), rolledBack(false), m_compDelay(false), m_LgateBias(0), m_WgateBias(0), Ad(0), Bd(0), Al(0), Bl(0), Cl(0), WAd(0), WBd(0), WAl(0), WBl(0), swActivity(0), slack(0), delta_slack(0), sensitivity(0), deltaLeak(0), CTSFlag(false), cellLibIndex(-1), RZFlag(false), is_end_point(false)
    {
      bbox.set(tlx, tby, tlx+twidth, tby+theight);
      oldCenterX = centerX = tlx + twidth/2;
      oldCenterY = centerY = tby + theight/2;
      toggle_count = 0;
      SRAM_stored_value = "0";
/*           if (cellName.compare(0,2,"AN") == 0) { func = AND;   gate_op = "&";}
      else if (cellName.compare(0,2,"AO") == 0) { func = AOI;   gate_op = "aoi";}
      else if (cellName.compare(0,2,"BU") == 0) { func = BUFF;  gate_op = "";}
      else if (cellName.compare(0,2,"DF") == 0) { func = DFF;   gate_op = "";}
      else if (cellName.compare(0,2,"IN") == 0) { func = INV;   gate_op = "~";}
      else if (cellName.compare(0,2,"LH") == 0) { func = LH;    gate_op = "";}
      else if (cellName.compare(0,2,"MU") == 0) { func = MUX;   gate_op = "";}
      else if (cellName.compare(0,2,"ND") == 0) { func = NAND;  gate_op = "~&";}
      else if (cellName.compare(0,2,"NR") == 0) { func = NOR;   gate_op = "~|";}
      else if (cellName.compare(0,2,"OA") == 0) { func = OAI;   gate_op = "oai";}
      else if (cellName.compare(0,2,"OR") == 0) { func = OR;    gate_op = "|";}
      else if (cellName.compare(0,2,"XN") == 0) { func = XNOR;  gate_op = "~^";}
      else if (cellName.compare(0,2,"XO") == 0) { func = XOR;   gate_op = "^";}*/
           if (cellName.substr(0,6 ) == "ADDF_X"       ) { func = ADDF;     }
      else if (cellName.substr(0,7 ) == "ADDFH_X"     ) { func = ADDFH;    }
      else if (cellName.substr(0,6 ) == "ADDH_X"      ) { func = ADDH;     }
      else if (cellName.substr(0,6 ) == "AND2_X"      ) { func = AND2;     }
      else if (cellName.substr(0,6 ) == "AND3_X"      ) { func = AND3;     }
      else if (cellName.substr(0,6 ) == "AND4_X"      ) { func = AND4;     }
      else if (cellName.substr(0,6 ) == "AO21_X"      ) { func = AO21;     }
      else if (cellName.substr(0,6 ) == "AO22_X"      ) { func = AO22;     }
      else if (cellName.substr(0,7 ) == "AO2B2_X"     ) { func = AO2B2;    }
      else if (cellName.substr(0,8 ) == "AO2B2B_X"    ) { func = AO2B2B;   }
      else if (cellName.substr(0,7 ) == "AOI21_X"     ) { func = AOI21;    }
      else if (cellName.substr(0,8 ) == "AOI211_X"    ) { func = AOI211;   }
      else if (cellName.substr(0,8 ) == "AOI21B_X"    ) { func = AOI21B;   }
      else if (cellName.substr(0,7 ) == "AOI22_X"     ) { func = AOI22;    }
      else if (cellName.substr(0,8 ) == "AOI221_X"    ) { func = AOI221;   }
      else if (cellName.substr(0,8 ) == "AOI222_X"    ) { func = AOI222;   }
      else if (cellName.substr(0,8 ) == "AOI2B1_X"    ) { func = AOI2B1;   }
      else if (cellName.substr(0,9 ) == "AOI2BB1_X"   ) { func = AOI2BB1;  }
      else if (cellName.substr(0,9 ) == "AOI2BB2_X"   ) { func = AOI2BB2;  }
      else if (cellName.substr(0,7 ) == "AOI31_X"     ) { func = AOI31;    }
      else if (cellName.substr(0,7 ) == "AOI32_X"     ) { func = AOI32;    }
      else if (cellName.substr(0,7 ) == "AOI33_X"     ) { func = AOI33;    }
      else if (cellName.substr(0,5 ) == "BUF_X"       ) { func = BUF;      }
      else if (cellName.substr(0,6 ) == "BUFZ_X"      ) { func = BUFZ;     }
      else if (cellName.substr(0,5 ) == "INV_X"       ) { func = INV;      }
      else if (cellName.substr(0,5 ) == "MX2_X"       ) { func = MX2;      }
      else if (cellName.substr(0,5 ) == "MX3_X"       ) { func = MX3;      }
      else if (cellName.substr(0,5 ) == "MX4_X"       ) { func = MX4;      }
      else if (cellName.substr(0,6 ) == "MXI2_X"      ) { func = MXI2;     }
      else if (cellName.substr(0,7 ) == "MXI2D_X"     ) { func = MXI2D;    }
      else if (cellName.substr(0,6 ) == "MXI3_X"      ) { func = MXI3;     }
      else if (cellName.substr(0,6 ) == "MXI4_X"      ) { func = MXI4;     }
      else if (cellName.substr(0,7 ) == "NAND2_X"     ) { func = NAND2;    }
      else if (cellName.substr(0,8 ) == "NAND2B_X"    ) { func = NAND2B;   }
      else if (cellName.substr(0,7 ) == "NAND3_X"     ) { func = NAND3;    }
      else if (cellName.substr(0,8 ) == "NAND3B_X"    ) { func = NAND3B;   }
      else if (cellName.substr(0,7 ) == "NAND4_X"     ) { func = NAND4;    }
      else if (cellName.substr(0,8 ) == "NAND4B_X"    ) { func = NAND4B;   }
      else if (cellName.substr(0,9 ) == "NAND4BB_X"   ) { func = NAND4BB;  }
      else if (cellName.substr(0,6 ) == "NOR2_X"      ) { func = NOR2;     }
      else if (cellName.substr(0,7 ) == "NOR2B_X"     ) { func = NOR2B;    }
      else if (cellName.substr(0,6 ) == "NOR3_X"      ) { func = NOR3;     }
      else if (cellName.substr(0,7 ) == "NOR3B_X"     ) { func = NOR3B;    }
      else if (cellName.substr(0,6 ) == "NOR4_X"      ) { func = NOR4;     }
      else if (cellName.substr(0,7 ) == "NOR4B_X"     ) { func = NOR4B;    }
      else if (cellName.substr(0,8 ) == "NOR4BB_X"    ) { func = NOR4BB;   }
      else if (cellName.substr(0,6 ) == "OA21_X"      ) { func = OA21;     }
      else if (cellName.substr(0,6 ) == "OA22_X"      ) { func = OA22;     }
      else if (cellName.substr(0,7 ) == "OAI21_X"     ) { func = OAI21;    }
      else if (cellName.substr(0,8 ) == "OAI211_X"    ) { func = OAI211;   }
      else if (cellName.substr(0,8 ) == "OAI21B_X"    ) { func = OAI21B;   }
      else if (cellName.substr(0,7 ) == "OAI22_X"     ) { func = OAI22;    }
      else if (cellName.substr(0,8 ) == "OAI221_X"    ) { func = OAI221;   }
      else if (cellName.substr(0,8 ) == "OAI222_X"    ) { func = OAI222;   }
      else if (cellName.substr(0,8 ) == "OAI2B1_X"    ) { func = OAI2B1;   }
      else if (cellName.substr(0,9 ) == "OAI2B11_X"   ) { func = OAI2B11;  }
      else if (cellName.substr(0,8 ) == "OAI2B2_X"    ) { func = OAI2B2;   }
      else if (cellName.substr(0,9 ) == "OAI2BB1_X"   ) { func = OAI2BB1;  }
      else if (cellName.substr(0,9 ) == "OAI2BB2_X"   ) { func = OAI2BB2;  }
      else if (cellName.substr(0,7 ) == "OAI31_X"     ) { func = OAI31;    }
      else if (cellName.substr(0,7 ) == "OAI32_X"     ) { func = OAI32;    }
      else if (cellName.substr(0,7 ) == "OAI33_X"     ) { func = OAI33;    }
      else if (cellName.substr(0,5 ) == "OR2_X"       ) { func = OR2;      }
      else if (cellName.substr(0,5 ) == "OR3_X"       ) { func = OR3;      }
      else if (cellName.substr(0,5 ) == "OR4_X"       ) { func = OR4;      }
      else if (cellName.substr(0,6 ) == "SDFF_X"      ) { func = SDFF    ; }
      else if (cellName.substr(0,7 ) == "SDFFH_X"     ) { func = SDFFH   ; }
      else if (cellName.substr(0,8 ) == "SDFFHQ_X"    ) { func = SDFFHQ  ; }
      else if (cellName.substr(0,9 ) == "SDFFHQN_X"   ) { func = SDFFHQN ; }
      else if (cellName.substr(0,8 ) == "SDFFNH_X"    ) { func = SDFFNH  ; }
      else if (cellName.substr(0,10) == "SDFFNSRH_X"  ) { func = SDFFNSRH; }
      else if (cellName.substr(0,7 ) == "SDFFQ_X"     ) { func = SDFFQ   ; }
      else if (cellName.substr(0,8 ) == "SDFFQN_X"    ) { func = SDFFQN  ; }
      else if (cellName.substr(0,7 ) == "SDFFR_X"     ) { func = SDFFR   ; }
      else if (cellName.substr(0,9 ) == "SDFFRHQ_X"   ) { func = SDFFRHQ ; }
      else if (cellName.substr(0,8 ) == "SDFFRQ_X"    ) { func = SDFFRQ  ; }
      else if (cellName.substr(0,7 ) == "SDFFS_X"     ) { func = SDFFS   ; }
      else if (cellName.substr(0,9 ) == "SDFFSHQ_X"   ) { func = SDFFSHQ ; }
      else if (cellName.substr(0,8 ) == "SDFFSQ_X"    ) { func = SDFFSQ  ; }
      else if (cellName.substr(0,8 ) == "SDFFSR_X"    ) { func = SDFFSR  ; }
      else if (cellName.substr(0,10) == "SDFFSRHQ_X"  ) { func = SDFFSRHQ; }
      else if (cellName.substr(0,8 ) == "SDFFTR_X"    ) { func = SDFFTR  ; }
      else if (cellName.substr(0,7 ) == "SEDFF_X"     ) { func = SEDFF   ; }
      else if (cellName.substr(0,9 ) == "SEDFFHQ_X"   ) { func = SEDFFHQ ; }
      else if (cellName.substr(0,9 ) == "SEDFFTR_X"   ) { func = SEDFFTR ; }
      else if (cellName.substr(0,9 ) == "SMDFFHQ_X"   ) { func = SMDFFHQ ; }
      else if (cellName.substr(0,11) == "TLATNTSCA_X" ) { func = TLATNTSCA; }
      else if (cellName.substr(0,11) == "TIEL0_A"     ) { func = TIELO   ; }
      else if (cellName.substr(0,7 ) == "XNOR2_X"     ) { func = XNOR2;    }
      else if (cellName.substr(0,7 ) == "XNOR3_X"     ) { func = XNOR3;    }
      else if (cellName.substr(0,6 ) == "XOR2_X"      ) { func = XOR2;     }
      else if (cellName.substr(0,6 ) == "XOR3_X"      ) { func = XOR3;     }
      // PLASTIC ARM STUFF
      else if (cellName.substr(2,7 ) == "IOINV_X"     ) { func = INV;      }
      else if (cellName.substr(2,7 ) == "IOBUF_X"     ) { func = BUF;      }
      else if (cellName.substr(2,8 ) == "IOBUFZ_X"    ) { func = BUFZ;     }
      else if (cellName.substr(2,5 ) == "INV_X"       ) { func = INV;      }
      else if (cellName.substr(2,5 ) == "BUF_X"       ) { func = BUF;      }
      else if (cellName.substr(2,7 ) == "NAND2_X"     ) { func = NAND2;    }
      else if (cellName.substr(2,7 ) == "NAND3_X"     ) { func = NAND3;    }
      else if (cellName.substr(2,6 ) == "NOR2_X"      ) { func = NOR2;     }
      else if (cellName.substr(2,6 ) == "NOR3_X"      ) { func = NOR3;     }
      else if (cellName.substr(2,7 ) == "AOI21_X"     ) { func = AOI21;    }
      else if (cellName.substr(2,7 ) == "AOI22_X"     ) { func = AOI22;    }
      else if (cellName.substr(2,8 ) == "AOI211_X"    ) { func = AOI211;   }
      else if (cellName.substr(2,7 ) == "OAI21_X"     ) { func = OAI21;    }
      else if (cellName.substr(2,7 ) == "OAI22_X"     ) { func = OAI22;    }
      else if (cellName.substr(2,6 ) == "DLY4_X"      ) { func = DLY4;     }
      else if (cellName.substr(2,7 ) == "TIEHI_X"     ) { func = TIEHI;    }
      else if (cellName.substr(2,7 ) == "TIELO_X"     ) { func = TIELO;    }
      else if (cellName.substr(2,6 ) == "DFFQ_X"      ) { func = DFFQ;     }
      else if (cellName.substr(2,8 ) == "DFFRPQ_X"    ) { func = DFFRPQ;   }
      else if (cellName.substr(2,8 ) == "PREICG_X"    ) { func = PREICG;   }
      else if (cellName.substr(2,7 ) == "SDFFQ_X"     ) { func = SDFFQ;    }
      else if (cellName.substr(2,9 ) == "SDFFRPQ_X"   ) { func = SDFFRPQ;  }
      else if (cellName.substr(2,5 ) == "MX2_X"       ) { func = MX2;      }
      else if (cellName.substr(2,8 ) == "RFAND2_X"    ) { func = AND2;     }
      else if (cellName.substr(2,9 ) == "RF1R1WS_X"   ) { func = RF1R1WS;  }
      else if (cellName.substr(2,10) == "RF1R1WSX_X"  ) { func = RF1R1WS;  }
      else if (cellName.substr(2,6 ) == "RFWL_X"      ) { func = RFWL;     }
      else if (cellName.substr(2,8 ) == "LATCH_X1"    ) { func = LATCH;    }

      //cout << " Gate is " << name << " Cell is " << cellName << " Func is " << cellFunc_to_name(func) << endl;

      isClkTree = false;
      cluster_id = -1;
      dead_toggle = false;
      visited = false;
    }
    // a public structure that holds power info for each possible master cell this cell instance can have
    vector<double> cellPowerLookup;
    static int max_toggle_profile_size;
    void openFiles(string outDir);
    //modifiers
    void setVariation(double v) { variation = v; }
    void setDelayVariation(double v) { delayVariation = v; }
    void setBiasCellName(string bName) { biasCellName = oldBiasCellName = bName; }
    void setCellName(string bName) { cellName = bName; }
    void setBaseName(string bName) { baseName = bName; }
    void setBestCellName(string bName) { bestCellName = bName; }
    void updateBiasCellName(string bName) { biasCellName = bName; }
    string getBiasCellName() { return biasCellName; }
    void setId(int i) { id = i; }
    void setTopoId(int i) { node->setTopoId(i);}
    void setClusterId(int i) { cluster_id = i;}
    void setFFFlag(bool flag) { FFFlag = flag; } // never invoked
    // JMS-SHK begin
    void setRZFlag(bool flag) { RZFlag = flag; }
    // JMS-SHK end
    void setisendpoint(bool val) { is_end_point = val; }
    bool isendpoint() { return is_end_point;}
    void setCTSFlag(bool flag) { CTSFlag = flag; }
    void setChannelLength(double l) { channelLength = l; }
    void setOptChannelLength(double optCL) { oldOptChannelLength = optChannelLength = optCL; }
    void updateOptChannelLength(double optCL) { optChannelLength = optCL; }
    void setGrid(Grid *a) { oldPGrid = pGrid = a; }
    void updateGrid(Grid *a) { pGrid = a; }
    void calDelay() { return; optDelay = delay = Ad*channelLength + Bd; }
    void setDelay(double d) { delay = d; }
    double getGateDelay() { return delay; }
    void calOptDelay();
    //      {
    //        assert(!FFFlag);
    //        //        cout<<"delay: "<<delay<<endl;
    //        optDelay = delay + Ad*(optChannelLength-channelLength);
    //        //        cout<<"channelLength: "<<channelLength<<endl;
    //        //        cout<<"optChannelLength: "<<optChannelLength<<endl;
    //        //        cout<<"optDelay: "<<optDelay<<endl;
    //      }
    void calLeakage() { optLeakage = leakage = Bl*channelLength + Al*channelLength*channelLength + Cl; }
    void calOptLeakage() { optLeakage = Bl*optChannelLength + Al*optChannelLength*optChannelLength + Cl; }
    void setGateType(GateType t) { type = t; }
    void addFaninGate(Gate *g);
    void addFanoutGate(Gate *g);
    int getFaninGateNum() { return fanin.size(); }
    Gate *getFaninGate(int index) { assert(0 <= index && index < fanin.size()); return fanin[index]; }
    int getFanoutGateNum() { return fanout.size(); }
    Gate *getFanoutGate(int index) { assert(0 <= index && index < fanout.size()); return fanout[index]; }
    void addFaninTerminal(Terminal *t) { faninTerms.push_back(t); }
    void addFanoutTerminal(Terminal *t) { fanoutTerms.push_back(t); }
    int getFaninTerminalNum() { return faninTerms.size(); }
    Terminal *getFaninTerminal(int index) { assert(0 <= index && index < faninTerms.size()); return faninTerms[index]; }
    Terminal *getFaninTerminalByName(string &name);
    int getFanoutTerminalNum() { return fanoutTerms.size(); }
    Terminal *getFanoutTerminal(int index) { assert(0 <= index && index < fanoutTerms.size()); return fanoutTerms[index]; }
    Terminal *getFanoutTerminalByName(string &name);
    void addFaninPad(Pad *p);
    void addFanoutPad(Pad *p);
    int getFaninPadNum() { return faninPads.size(); }
    Pad *getFaninPad(int index) { assert(0 <= index && index < faninPads.size()); return faninPads[index]; }
    int getFanoutPadNum() { return fanoutPads.size(); }
    Pad *getFanoutPad(int index) { assert(0 <= index && index < fanoutPads.size()); return fanoutPads[index]; }
    void setDelayCoeff(double A, double B) { Ad = A; Bd = B; }
    void setLeakageCoeff(double A, double B, double C) { Al = A; Bl = B; Cl = C; }
    void getDelayCoeff(double &A, double &B) { A = Ad; B = Bd; }
    void getLeakageCoeff(double &A, double &B, double &C) { A = Al; B = Bl; C = Cl; }
    void setWDelayCoeff(double A, double B) { WAd = A; WBd = B; }
    void setWLeakageCoeff(double A, double B) { WAl = A; WBl = B; }
    void getWDelayCoeff(double &A, double &B) { A = WAd; B = WBd; }
    void getWLeakageCoeff(double &A, double &B) { A = WAl; B = WBl; }
    double getLoadCap();
    double getInputSlew();
    void initFanoutLoop() { fanoutLoop.assign(fanout.size(), false); }
    void setFanoutLoopGate(int index) { assert(0 <= index && index < fanoutLoop.size()); fanoutLoop[index] = true; }
    bool fanoutGateInLoop(int index) { assert(0 <= index && index < fanoutLoop.size()); return fanoutLoop[index]; }
    void initFaninLoop()
      {
        faninLoop.assign(fanin.size(), false);
      }
    void setFaninLoopGate(int index)
      {
        assert(0 <= index && index < faninLoop.size()); faninLoop[index] = true;
      }
    void setFaninLoopGatePtr(Gate *g);
    bool faninGateInLoop(int index)
    {
      //	  cout<<"index: "<<index<<" faninLoop.size()"<<faninLoop.size()<<endl;
      //        cout<<"fanin.size()"<<fanin.size()<<endl;
      assert(0 <= index && index < faninLoop.size());
      return faninLoop[index];
    }
    void addInputSubnet(Subnet *s) { inputSubnets.push_back(s); }
    void addOutputSubnet(Subnet *s) { outputSubnets.push_back(s); }
    int getInputSubnetNum() { return inputSubnets.size(); }
    int getOutputSubnetNum() { return outputSubnets.size(); }
    Subnet *getInputSubnet(int index) { assert(0 <= index && index < inputSubnets.size()); return inputSubnets[index]; }
    Subnet *getOutputSubnet(int index) { assert(0 <= index && index < outputSubnets.size()); return outputSubnets[index]; }
    //accossers
    int getId() { return id; }
    int getTopoId() { return node->getTopoId();}
    int getClusterId() { return cluster_id;}
    //bool getFFFlag() { return (FFFlag || func == DFF || func == LH); }
    bool getFFFlag() { return FFFlag; }
    bool getIsTIE() { return (func == TIEHI || func == TIELO); }
    bool getIsMux() { return (func == MUX);}
    bool isClusterBoundaryDriver() ;
    // JMS-SHK begin
    bool getRZFlag() { return RZFlag; }
    // JMS-SHK end
    bool getCTSFlag() { return CTSFlag; }
    string getName() { return name; }
    string getCellName() { return cellName; }
    string getBaseName() { return baseName; }
    string getBestCellName() { return bestCellName; }
    double getChannelLength() { return channelLength; }
    double getOptChannelLength() { return optChannelLength; }
    Grid* getGrid() { return pGrid; }
    double getDelay();
    double getFFDelay(int index);
    double getMaxFFDelay();
    double getOptDelay() { return optDelay; }
    double getLeakage() { return leakage; }
    double getOptLeakage() { return optLeakage; }
    double getSlack() { return slack; }
    double getDeltaSlack() { return delta_slack; }
    double getVariation() { return variation; }
    double getDelayVariation() { return delayVariation; }
    GateType getGateType() { return type; }
    void setNetBox(int lx, int by, int rx, int ty) { netBox.set(lx, by, rx, ty); }
    int getNetLX() { return netBox.left(); }
    int getNetRX() { return netBox.right(); }
    int getNetBY() { return netBox.bottom(); }
    int getNetTY() { return netBox.top(); }
    int getLX() { return lx; }
    int getRX() { return lx+width; }
    int getBY() { return by; }
    int getTY() { return by+height; }
    int getWidth() { return width; }
    int getHeight() { return height; }
    void setGridX(int gx) { gridX = gx; }
    void setGridY(int gy) { gridY = gy; }
    void setLSiteColIndex(int l) { lSiteColIndex = l; }
    void setBSiteRowIndex(int b) { bSiteRowIndex = b; }
    void setRSiteColIndex(int r) { rSiteColIndex = r; }
    void setTSiteRowIndex(int t) { tSiteRowIndex = t; }
    int getLSiteColIndex() { return lSiteColIndex; }
    int getBSiteRowIndex() { return bSiteRowIndex; }
    int getRSiteColIndex() { return rSiteColIndex; }
    int getTSiteRowIndex() { return tSiteRowIndex; }
    void setSlack(double x) { slack = x; }
    void setDeltaSlack(double x) { delta_slack = x; }

    //the cell name when OA read in the case, can be biased name
    void setOrgCellName(string orgName) { orgCellName = orgName; }
	  void updateXY(int x, int y)
	  {
	  	lx = x;
	  	by = y;
	  	centerX = x + width/2;
	  	centerY = y + height/2;
	  }

	  void rollBack();
    void calNetBox();
    int calHPWL();
    int calHPWL(int x, int y);
    int getHPWL() { return hpwl; }

    void setCritical(bool b) { critical = b; }
    bool isCritical() { return critical; }
    void setFixed(bool b) { fixed = b; }
    bool isFixed() { return fixed; }
    void setDiv(Divnet *d) { div = d; }
    void setDiv_src(Divnet *d) { div_src = d; }
    void setDiv_des(Divnet *d) { div_des = d; }
    Divnet* getDiv() { return div; }
    Divnet* getDiv_src() { return div_src; }
    Divnet* getDiv_des() { return div_des; }
	void    setSwapped(bool b) { swapped = b; }
	bool    isSwapped() { return swapped; }
	void    setChecked(bool b) { checked = b; }
	bool    isChecked() { return checked; }
	void    setExclude(bool b) { exclude = b; }
	bool    isExclude() { return exclude; }
	void    setCheckSrc(bool b) { checkSrc = b; }
	bool    isCheckSrc() { return checkSrc; }
	void    setCheckDes(bool b) { checkDes = b; }
	bool    isCheckDes() { return checkDes; }
	void    setToggled(bool b, string val) { toggled = b; toggledTo = val; }
	bool    isToggled() { return toggled; }
  bool check_for_dead_toggle(int cycle_num);
  void setDeadToggle(bool val) { dead_toggle = val; }
  bool isDeadToggle() { return dead_toggle; }
  void setVisited(bool val) { visited = val; }
  bool isVisited() { return visited; }
  void trace_back_dead_gate(int& dead_gates_count, int cycle_num);
        string  getToggledTo() {return toggledTo; }
        void    incToggleCount() {toggle_count++ ;}
        void    updateToggleProfile(int cycle_num);
        void    printToggleProfile(ofstream& file);
        void    print_terms(ofstream& file);
        void    resizeToggleProfile(int val);
        int     getToggleCountFromProfile() ;
        int     getToggleCorrelation(Gate* gate);
        int     getToggleCount() {return toggle_count ;}
        bool    notInteresting() ;
	void    setHolded(bool b) { holded = b; }
	bool    isHolded() { return holded; }
    int     getCellLibIndex() { return cellLibIndex; }
    void    setCellLibIndex(int ind) { cellLibIndex = ind; }
    string  getSmallerMasterCell() { return smallerMasterCell; }
    void    setSmallerMasterCell(string cs) { smallerMasterCell = cs; }
    void    setSwActivity(double val) { swActivity = val;}
    double  getSwActivity() { return swActivity;}
    void    setSensitivity(double val) { sensitivity = val;}
    double  getSensitivity() { return sensitivity;}
    void    setDeltaLeak(double val) { deltaLeak = val;}
    double  getDeltaLeak() { return deltaLeak;}
    void    setDeltaDelay(double val) { deltaDelay = val;}
    double  getDeltaDelay() { return deltaDelay;}
    void    setLeakList(double val) { leakList.push_back(val);}
    void    setDelayList(double val) { delayList.push_back(val);}
    void    setSlackList(double val) { slackList.push_back(val);}
    void    setCellList(string str) { cellList.push_back(str);}
    double  getCellLeakTest(int i) { return leakList[i]; }
    double  getCellLeak() {
        for (int i = 0; i < cellList.size(); i++)
            if (cellName.compare(cellList[i]) == 0)
                return leakList[i];
        cout << "[SensOpt] Fatal error: Gate.getCellLeak()" << endl;
        return 0;
    }
    double  getCellDelay() {
        for (int i = 0; i < cellList.size(); i++)
            if (cellName.compare(cellList[i]) == 0)
                return delayList[i];
        cout << "[SensOpt] Fatal error: Gate.getCellDelay()" << endl;
        return 0;
    }
    double  getCellSlack() {
        for (int i = 0; i < cellList.size(); i++)
            if (cellName.compare(cellList[i]) == 0)
                return slackList[i];
        cout << "[SensOpt] Fatal error: Gate.getCellSlack()" << endl;
        return 0;
    }

    void    setPathStr( string path_name ) { PathStr = path_name;}
    string  getPathStr() { return PathStr;}
    string getOrgCellName() { return orgCellName; }
//    void setX(int x1) { oldX = x = x1; }
//    void setY(int y1) { oldY = y = y1; }
//    void updateX(int x1) { x = x1; }
//    void updateY(int y1) { y = y1; }
//    int getX() { return x; }
//    int getY() { return y; }
    int getGridX() { return gridX; }
    int getGridY() { return gridY; }
    int getCenterX() { return centerX; }
    int getCenterY() { return centerY; }
    Pad *getInputPad();
    Pad *getOutputPad();
    Gate *getInputFF();
    Gate *getOutputFF();
    bool outputIsPad();
    bool outputIsFF();
    bool inputIsPad();
    bool inputIsFF();
    void addDelayVal(string tName, double delay);
    double getDelayVal(string tName);
    double getOptDelayVal(string tName);
    double getWeight() { return weight; }
    void setWeight(double w) { weight = w; }
    void incWeight(double w) { weight += w; }
    void print();
    int getPathNum() { return paths.size(); }
		Path *getPath(int index) { assert(0 <= index && index < paths.size()); return paths[index]; }
		bool addPath(Path *p);
                // JMS-SHK begin
                int getMinusPathNum() { return minus_paths.size(); }
		Path *getMinusPath(int index) { assert(0 <= index && index < minus_paths.size()); return minus_paths[index]; }
		bool addMinusPath(Path *p);
                // JMS-SHK end
                bool removePath(Path *p);
		void clearPaths() { paths.clear(); }
		bool hasTerminal(Terminal *t);
		Terminal *getFaninConnectedTerm(int gId);
		bool allFaninTermDisabled();
		bool allFanoutGateDisabled();
		double getMinDelay();
        bool allInpNetsVisited();
		bool calLeakWeight();
		double calMaxPossibleDelay();
                string getMuxSelectPinVal();
                void untoggleMuxInput(string input);
                int numToggledInputs();
                void checkControllingMIS(vector<int> & false_term_ids, designTiming* T);
                bool check_toggle_fine();
                bool checkANDToggle  ( );
                bool checkAOIToggle  ( );
                bool checkBUFFToggle ( );
                bool checkDFFToggle  ( );
                bool checkINVToggle  ( );
                bool checkLHToggle   ( );
                bool checkMUXToggle  ( );
                bool checkNANDToggle ( );
                bool checkNORToggle  ( );
                bool checkOAIToggle  ( );
                bool checkORToggle   ( );
                bool checkXNORToggle ( );
                bool checkXORToggle  ( );
                void handleANDMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleAOIMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleBUFFMIS (vector<int>& false_term_ids, designTiming* T );
                void handleDFFMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleINVMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleLHMIS   (vector<int>& false_term_ids, designTiming* T );
                void handleMUXMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleNANDMIS (vector<int>& false_term_ids, designTiming* T );
                void handleNORMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleOAIMIS  (vector<int>& false_term_ids, designTiming* T );
                void handleORMIS   (vector<int>& false_term_ids, designTiming* T );
                void handleXNORMIS (vector<int>& false_term_ids, designTiming* T );
                void handleXORMIS  (vector<int>& false_term_ids, designTiming* T );
                void computeNetExpr();
                void handleANDExpr  ( );
                void handleAOIExpr  ( );
                void handleBUFFExpr ( );
                void handleDFFExpr  ( );
                void handleINVExpr  ( );
                void handleLHExpr   ( );
                void handleMUXExpr  ( );
                void handleNANDExpr ( );
                void handleNORExpr  ( );
                void handleOAIExpr  ( );
                void handleORExpr   ( );
                void handleXNORExpr ( );
                void handleXORExpr  ( );
                bool computeANDVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeAOIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeBUFFVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeDFFVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeINVVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeLHVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeMUXVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeNANDVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeNORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeOAIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeORVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeXNORVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeXORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeVal(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeValOne(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool computeValOne_new(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);

                // BASIC GATES
                void ComputeBasicAND (string A, string B, string& Y);
                void ComputeBasicOR (string A, string B, string& Y);
                void ComputeBasicXOR (string A, string B, string& Y);
                void ComputeBasicINV (string A, string& Y);
                void ComputeBasicMUX (string A, string B, string S,  string& Y);

                void ComputeBasicAND_new (string& Y, string A, string B);
                void ComputeBasicAND_new (string& Y, string A, string B, string C);
                void ComputeBasicAND_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicOR_new (string& Y, string A, string B);
                void ComputeBasicOR_new (string& Y, string A, string B, string C);
                void ComputeBasicOR_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicXOR_new (string& Y, string A, string B);
                void ComputeBasicXOR_new (string& Y, string A, string B, string C);
                void ComputeBasicXOR_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicXNOR_new (string& Y, string A, string B);
                void ComputeBasicXNOR_new (string& Y, string A, string B, string C);
                void ComputeBasicXNOR_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicNAND_new (string& Y, string A, string B);
                void ComputeBasicNAND_new (string& Y, string A, string B, string C);
                void ComputeBasicNAND_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicNOR_new (string& Y, string A, string B);
                void ComputeBasicNOR_new (string& Y, string A, string B, string C);
                void ComputeBasicNOR_new (string& Y, string A, string B, string C, string D);

                void ComputeBasicINV_new (string& Y, string A);
                void ComputeBasicMUX_new (string& Y, string A, string B, string S);

                bool ComputeValADDF_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValADDFH_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValADDH_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND2_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO21_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO22_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO2B2_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO2B2B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI21_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI211_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI21B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI22_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI221_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI222_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2B1_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2BB1_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2BB2_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI31_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI32_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI33_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValBUF_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValBUFZ_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValDLY4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValTIELO_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValTIEHI_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValINV_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValLATCH_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX2_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX3_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX4_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI2_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI2D_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND2_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND2B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND3_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND3B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4BB_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR2_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR2B_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR3B_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4B_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4BB_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOA21_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOA22_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI21_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI211_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI21B_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI22_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI221_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI222_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B1_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B11_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B2_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2BB1_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2BB2_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI31_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI32_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI33_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR2_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR3_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR4_new       ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValPREICG_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValRFWL_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValRF1R1WS_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFF_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFH_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFHQ_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFQ_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFR_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFRHQ_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFRQ_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFS_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFSHQ_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFSQ_new    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValTLATNTSCA_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXNOR2_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXNOR3_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXOR2_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXOR3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);

                bool ComputeValADDF     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValADDFH    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValADDH     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAND4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO21     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO22     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO2B2    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAO2B2B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI21    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI211   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI21B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI22    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI221   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI222   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2B1   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2BB1  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI2BB2  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI31    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI32    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValAOI33    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValBUF      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValBUFZ     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValDLY4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValDFFQ     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValDFFRPQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValINV      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValLATCH    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX2      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX3      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMX4      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI2D     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValMXI4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND2    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND2B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND3    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND3B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNAND4BB  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR2B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR3B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValNOR4BB    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOA21     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOA22     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI21    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI211   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI21B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI22    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI221   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI222   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B1   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B11  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2B2   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2BB1  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI2BB2  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI31    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI32    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOAI33    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR2      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR3      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValOR4      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValPREICG   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValRF1R1WS  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValRFWL     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFF     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFH    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFHQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFQ    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFR    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFRHQ  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFRPQ  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFRQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFS    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFSHQ  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValSDFFSQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValTLATNTSCA( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXNOR2    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXNOR3    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXOR2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool ComputeValXOR3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                bool transferDtoQ(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                string getSimValue();
                void setSimValueReg(string val, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                void print_gate_debug_info();


    //assume gate delay is computed as the average value of TPLH and TPHL
    void clearMem();
    void setSortIndex(double val) { sortIndex = val; }
    double getSortIndex() { return sortIndex; }
    double getLeakWeight() { return leakWeight; }
    double getMaxDelay();
		double getMaxOptDelay();
    void setDisable(bool b) { disable = b; }
    bool isDisabled() { return disable; }
    void setOrgOrient(int o) { orgOrient = o; }
    int getOrgOrient() { return orgOrient; }
    bool isRollBack() { return rolledBack; }
    void setRollBack(bool b) { rolledBack = b; }

    void setGNode(GNode* gnode) {node = gnode;}
    GNode* getGNode () { return node; }
    NetVector &getNets() { return nets; }
    void getDriverTopoIds(vector<int>& topo_ids);
    void addNet(Net *n);
    Net *getNet(int index) { assert(0 <= index && index < nets.size()); return nets[index]; }
    void setCompDelay(bool b) { m_compDelay = b; }
    bool getCompDelay() { return m_compDelay; }
    void setLgateBias(double d) { m_LgateBias = d; }
    void setWgateBias(double d) { m_WgateBias = d; }
    double getLgateBias() { return m_LgateBias; }
    double getWgateBias() { return m_WgateBias; }
    void setIsClkTree(bool val) { isClkTree = val; }
    bool getIsClkTree() { return isClkTree; }
		//need norminal Lgate
		double compMaxDeltaLLeakage()
		{
			//100 = 10^2
			m_maxDeltaLLeak = fabs(Al*100 + (2*Al*channelLength + Bl)*10);
			return m_maxDeltaLLeak;
		}
		double compMaxDeltaWLeakage() { m_maxDeltaWLeak = fabs(WAl)*10; return m_maxDeltaWLeak; }
		double compMaxDeltaLDelay() { m_maxDeltaLDelay = fabs(Ad)*10; return m_maxDeltaLDelay; }
		double compMaxDeltaWDelay() { m_maxDeltaWDelay = fabs(WAd)*10; return m_maxDeltaWDelay; }

  private:
    double variation;  // normalized sample value of delay variation for this gate (A*W)
    double delayVariation;  // sample value of delay variation for this gate
    int cellLibIndex;  // tells which master cell is being used
    string smallerMasterCell;  // the next smaller master cell, used to when downsizing by 1
    int id;
    int topo_id;
    int cluster_id;
    string name;
    string cellName;
    string bestCellName;
    string biasCellName;
    string oldBiasCellName;
    string orgCellName;
    string baseName;
    GateType type;
    CellFunc func;
    double channelLength;// nm
    double optChannelLength;
    double oldOptChannelLength;
    double m_LgateBias;
    double m_WgateBias;
    double delay;
    double optDelay;
    double leakage;
    double optLeakage;
    double slack;
    double delta_slack;
    Grid *pGrid;
    Grid *oldPGrid;
    GateVector fanin;
    GateVector fanout;
    BoolVector faninLoop;
    BoolVector fanoutLoop;
    TerminalVector faninTerms;
    TerminalVector fanoutTerms;
    PadVector faninPads;
    PadVector fanoutPads;
    Box bbox;
    Box netBox;
    bool FFFlag;
    bool is_end_point;
    // JMS-SHK begin
    bool RZFlag;
    // JMS-SHK end
    bool CTSFlag;
    double Ad, Bd;
    double Al, Bl, Cl;
    double WAd, WBd;
    double WAl, WBl;
    int gridX;
    int gridY;
    int centerX;
    int centerY;
    int oldCenterX;
    int oldCenterY;
    int lx, by, oldLX, oldBY;
    int width, height;
//    int x, y;
//    int oldX, oldY;
    SubnetVector inputSubnets;
    SubnetVector outputSubnets;
    StrDMap delayMap;
    StrDMap optDelayMap;
    double weight;
    bool critical;
    int hpwl;
    PathVector paths;
    // JMS-SHK begin
    PathVector minus_paths;
    // JMS-SHK end
    bool swapped;
    bool checked;
    bool exclude;
    bool checkSrc;
    bool checkDes;
    bool toggled;
    bool visited;
    string toggledTo;
    bool dead_toggle;
    bool holded;
    bool fixed;
    Divnet* div;
    Divnet* div_src;
    Divnet* div_des;
    double sortIndex;
    double leakWeight;
    bool disable;
    int toggle_count;
    vector<bitset<64> > toggle_profile; // ulong is 64
    int orgOrient;
    bool rolledBack;
    int lSiteColIndex;
    int bSiteRowIndex;
    int rSiteColIndex;
    int tSiteRowIndex;
    NetVector nets;
    bool m_compDelay;
    double m_maxDeltaLLeak;
    double m_maxDeltaWLeak;
    double m_maxDeltaLDelay;
    double m_maxDeltaWDelay;
    double swActivity;          // Switching Activity
    double sensitivity;
    double deltaLeak;
    double deltaDelay;
    vector<double> leakList;
    vector<double> delayList;
    vector<double> slackList;
    vector<string> cellList;
    string PathStr;
    GNode* node;
    string SRAM_stored_value; // PLASTICARM CELL
    string gate_op;
    bool isClkTree;
    static ofstream gate_debug_file;
  };


  struct GateSensitivitySorterS2L : public std::binary_function<Gate*,Gate*,bool>
  {
    bool operator()(Gate *g1, Gate *g2) const
    {
      return (g1->getSensitivity() < g2->getSensitivity());
    };
  };

  class GateSensitivityS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSensitivity() < g2->getSensitivity();
  		}
  };
  class GateSensitivityL2S {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSensitivity() > g2->getSensitivity();
  		}
  };
  class GateIndexS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSortIndex() < g2->getSortIndex();
  		}
  };
  class GateWeightL2S {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getWeight() > g2->getWeight();
  		}
  };
  class GateWeightS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getWeight() < g2->getWeight();
  		}
  };
  class GateSlackS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSlack() < g2->getSlack();
  		}
  };
  class GateSWL2S {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSwActivity() > g2->getSwActivity();
  		}
  };
  class GateSWS2L {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSwActivity() < g2->getSwActivity();
  		}
  };
  class GateSWSlackL2S {
  	public:
  		inline bool operator () (Gate *g1, Gate *g2)
  		{
  			return g1->getSwActivity()*g1->getSlack() < g2->getSwActivity()*g2->getSlack();
  		}
  };

  class Libcell {
  public:
    //constructors
    Libcell() {}
    Libcell(string name):cellName(name) {}
    ~Libcell() {}
    string getCellName() { return cellName; }
    string getCellFuncId() { return cellFuncId; }
    double getCellDelay() { return cellDelay; }
    double getCellLeak() { return cellLeak; }
    Libcell* getUpCell() { return upCell; }
    Libcell* getDnCell() { return dnCell; }
    void setCellName(string x) { cellName = x; }
    void setCellFuncId(string x) { cellFuncId = x; }
    void setCellDelay(double x) { cellDelay = x; }
    void setCellLeak(double x) { cellLeak = x; }
    void setUpCell(Libcell* l) { upCell = l; }
    void setDnCell(Libcell* l) { dnCell = l; }
  private:
    string  cellName;
    string  cellFuncId;
    double  cellDelay;
    double  cellLeak;
    Libcell* upCell;
    Libcell* dnCell;
  };

  class LibcellDelayL2S {
  public:
    inline bool operator () (Libcell *c1, Libcell *c2)
    {
      return c1->getCellDelay() > c2->getCellDelay();
    }
  };

  class LibcellLeakS2L {
  public:
    inline bool operator () (Libcell *c1, Libcell *c2)
    {
      return c1->getCellLeak() < c2->getCellLeak();
    }
  };

}
#endif //__GATE_H__
