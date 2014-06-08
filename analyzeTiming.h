// ****************************************************************************
// ****************************************************************************
// analyzeTiming.h
// ****************************************************************************
// ****************************************************************************

#ifndef	__DESIGN_TIMING__
#define __DESIGN_TIMING__

#include <string>
#include <iostream>
#include <vector>
#include <tcl.h>

using namespace std;

namespace POWEROPT {

  class designTiming
    {
    public:
      // constructor
      designTiming();
      // destructor
      ~designTiming();

      void setPtcnt(int x) { _pt_access_cnt = x; }
      int  getPtcnt() { return _pt_access_cnt; }
      void incPtcnt() { _pt_access_cnt ++; }
      string getWNSList(string clockName);
      double getPathSlack(string  PathString);
      double getWorstSlack(string clockName);
      double getWorstSlackMin(string clockName);
      string getLibCell(string PinName);
      string getOutPin(string CellName);
      string getCellFromNet(string PinName);
      string getInst(string PinName);
      void removeFFDelay (string CellName);
      bool sizeCell(string cellInstance, string cellMaster);
      void swapCell(string cellInstance, string cellMaster);
      void setFalsePath(string  PathString);
      void resetPathsThroughAllCells();
      void setFalsePathThrough(string  PathString);
      void setFalsePathTo(string  Cell);
      void setFalsePathFrom(string  Cell);
      void suppressMessage (string message);
      void resetPath(string  PathString);
      void updatePower();
      void updateTiming();
      void writeChange(string FileName);
      void readTcl(string FileName);
      void reportSlack(string FileName);
      double getFFSlack(string  endFF);
      double getFFMinSlack(string  endFF);
      double getCellMinSlack(string  CellName);
      double getCellSlack(string  CellName);
      double getCellArea(string  CellName);
      double getFFQDelay(string  CellName);
      double getCellDelay(string  CellName);
      bool   checkDependFF(string startFF, string endFF);
      void   setCell(string cell, string cell_name);
      void   addtoCol(string collection, string cell_name);
      double getCellToggleRate(string  CellName);
      double getCellPower(string  CellName);
      double getCellLeak(string  CellName);
      double getTotalPower();
      double getLeakPower();
      double getArea();
      void writeSDF(string SDF);
      void putCommand(string Command);
      void saveSession(string file);
      void restoreSession(string file);
      bool makeResizeTcl(string target1, string target2);
      bool makeEcoTcl(string file, string target1);
      bool makeLeakList(string inFile, string outFile);
      string getLibNames();
      void makeCellList(string libName, string outFile);
      void makeFinalEcoTcl(string target);
      void writeVerilog(string target);
      void linkLib(string VOLT);

      bool checkMaxTran(string cellName);
      bool checkPT();
      void Exit();

      void getNetDelay(double &delay, string sourcePinName, string sinkPinName);
      void getFFDelay(double &rise, double &fall, string pinName);
      void getMaxCellDelay(double &delay, string cellName);
      void getCellDelay(double &delay, string cellInPin, string cellOutPin);
      void getInputSlew(double &riseSlew, double &fallSlew, string pinName);
      void getLoadCap(double &riseCap, double &fallCap, string pinName);
      void getOutLoadCap(double &cap, string pinName);

      double  runTiming(double clockCycleTime);
      string reportTiming(int cycle_time);
      void reportQoR();

      double  getSetupSlack(string startFF, string endFF);

      double  getHoldSlack(string startFF, string endFF);

      double  getBufferLoadCap(string buffer);

      double  getBufferInputSlew(string buffer);

      double  getZeta(string parentBuffer, string childBuffer, string swapMaster);

      string  getMasterOfInstance(string cellInstance);

      vector<double> getNewLoads(vector<pair<string, string> > swapSet, vector<string> measureSet);

      void  initializeServerContact(string clientName);
      void  closeServerContact();

    private:
      string  _convertToString(double x);
      double  _convertToDouble(const string& s);
      int     _convertToInt(const string& s);

      Tcl_Interp *_interpreter;
      int  _errorCode;
      char *_tclExpression;
      char *_tclAnswer;
      string _tclInputString;

      int   _pt_access_cnt;

    };
}
#endif

