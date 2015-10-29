#include "macros.h"
// ****************************************************************************
// analyzeTiming.cpp
//
// Make contact with PT Tcl server, pass commands, get results and report
// ****************************************************************************

#include "analyzeTiming.h"
#include "typedefs.h"
#include <tcl.h>
#include <stdio.h>
#include "string.h"

//#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

namespace POWEROPT {

  // ****************************************************************************
  // analyzeTiming constructor
  // ****************************************************************************
  designTiming::designTiming()
  {
  }

  // ****************************************************************************
  // analyzeTiming destructor
  // ****************************************************************************
  designTiming::~designTiming()
  {
  }

  // ****************************************************************************
  // Initialize Tcl interpreter and contact server
  //
  // REMEMBER: do not delete the interpreter until all processing is done
  // ****************************************************************************
  void
  designTiming::initializeServerContact(string clientName)
  {
    _interpreter = Tcl_CreateInterp();

    // check for the presence of file "ptclient.tcl" in the current
    // working directory If it is not found, then you cannot proceed
    //
    // Quit immediately
    ifstream    _clientTcl(clientName.c_str());
    if(!_clientTcl) {
      cerr << "Fatal error: cannot proceed without ptclient.tcl file" << endl;
      cerr << "\t\t\t copy ptclient.tcl file into current working directory" << endl;
      exit(1);
    }
    _tclInputString = "source " + clientName;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclExpression = "InitClient";
    Tcl_Eval(_interpreter, _tclExpression);
  }

  // ****************************************************************************
  // closeServerContact
  // ****************************************************************************
  void
  designTiming::closeServerContact()
  {
    _tclExpression = "CloseClient";
    Tcl_Eval(_interpreter, _tclExpression);
    Tcl_DeleteInterp(_interpreter);
  }


  // ****************************************************************************
  // swapCell
  // ****************************************************************************
  void
  designTiming::swapCell(string	  cellInstance,
                         string	  cellMaster)
  {
    _tclInputString = "DoOnePtCommand \"PtSwapCell " + cellInstance + " " + cellMaster + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
        exit(1);
      }
  }

 string
  designTiming::reportTiming(int cycle_time)
  {
    string  _strCycleTime = _convertToString(cycle_time);
    _tclInputString = "DoOnePtCommand \"PtReportTiming " + _strCycleTime + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  return _answerStr;
  }

  void
  designTiming::reportQoR()
  {
    _tclInputString = "DoOnePtCommand \"PtReportQoR\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  // ****************************************************************************
  // runTiming
  // ****************************************************************************
  double
  designTiming::runTiming(double  clockCycleTime)
  {
    string  _strCycleTime = _convertToString(clockCycleTime);
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetWorstSlack " + _strCycleTime + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _worstSlack = _convertToDouble(_answerStr);
    return(_worstSlack);
  }


  string 
  designTiming::getMaxFallSlack(string term)
  {
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetMaxFallSlack " + term + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return _answerStr;
/*    double _worstSlack ;
    if (_answerStr == "INFINITY")
       _worstSlack = _convertToDouble(_answerStr);
    else 
        _worstSlack = 10000.0;
    return(_worstSlack);*/
  }

  string 
  designTiming::getMaxFallArrival(string term)
  {
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetMaxFallArrival " + term + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return _answerStr;
/*    double _worstSlack ;
    if (_answerStr == "INFINITY")
       _worstSlack = _convertToDouble(_answerStr);
    else 
        _worstSlack = 10000.0;
    return(_worstSlack);*/
  }

  string 
  designTiming::getMaxRiseArrival(string term)
  {
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetMaxRiseArrival " + term + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return _answerStr;
/*    double _worstSlack ;
    if (_answerStr == "INFINITY")
       _worstSlack = _convertToDouble(_answerStr);
    else 
        _worstSlack = 10000.0;
    return(_worstSlack);*/
  }

  string 
  designTiming::getMaxRiseSlack(string term)
  {
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetMaxRiseSlack " + term + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return _answerStr;
/*    double _worstSlack ;
    if (_answerStr == "INFINITY")
       _worstSlack = _convertToDouble(_answerStr);
    else 
        _worstSlack = 10000.0;
    return(_worstSlack);*/
  }

  string designTiming::getClockTreeCells()
  {
    _tclInputString = "DoOnePtCommand \"PtGetClkTreeCells \"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return _answerStr;
  }
  
  double
  designTiming::runTimingPBA(double  clockCycleTime)
  {
    string  _strCycleTime = _convertToString(clockCycleTime);
    //cout << "Trying to get worst slack" << endl;
    _tclInputString = "DoOnePtCommand \"PtGetWorstSlackPBA " + _strCycleTime + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _worstSlack = _convertToDouble(_answerStr);
    return(_worstSlack);
  }

  void
  designTiming::getMaxCellDelay(double &delay, string cellName)
  {
    _tclInputString = "DoOnePtCommand \"report_timing_arc " + cellName + "\"";
//     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
//     cout<<"_tclAnswer: "<<_tclAnswer<<endl;
    float temp;
    sscanf(_tclAnswer, "%f", &temp);
    delay = temp;
  }



  void
  designTiming::getCellDelay(double &delay, string cellInPin, string cellOutPin)
  {
    _tclInputString = "DoOnePtCommand \"gate_delay " + cellInPin + " " + cellOutPin +  "\"";
    //     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    //     cout<<"_tclAnswer: "<<_tclAnswer<<endl;
    float temp;
    sscanf(_tclAnswer, "%f", &temp);
    delay = temp;
  }

  void
  designTiming::getNetDelay(double &delay, string sourcePinName, string sinkPinName)
  {
    _tclInputString = "DoOnePtCommand \"report_wire_delay " + sourcePinName + " " + sinkPinName + "\"";
    //_tclInputString = "DoOnePtCommand \"report_wire_delay U21008/Y U21006/A\"";

    //     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    //     cout<<"_tclAnswer: "<<_tclAnswer<<endl;

    float temp;
    sscanf(_tclAnswer, "%f", &temp);
    delay = temp;
  }

  void
  designTiming::getFFDelay(double &rise, double &fall, string pinName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetFFDelay " + pinName + "\"";
//     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
//     cout<<"_tclAnswer: "<<_tclAnswer<<endl;

    float temp1;
    float temp2;
    sscanf(_tclAnswer, "%f%f", &temp1, &temp2);
    rise = temp1;
    fall = temp2;
  }

  void
  designTiming::getInputSlew(double &riseSlew, double &fallSlew, string pinName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetCellInputSlew " + pinName + "\"";

//     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
//     cout<<"_tclAnswer: "<<_tclAnswer<<endl;
    float temp1;
    float temp2;
    sscanf(_tclAnswer, "%f%f", &temp1, &temp2);
    riseSlew = temp1;
    fallSlew = temp2;
  }

  void
  designTiming::getOutLoadCap(double &cap, string pinName)
  {
    _tclInputString = "DoOnePtCommand \"report_outcap " + pinName + "\"";
//     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    //     cout<<"_tclAnswer: "<<_tclAnswer<<endl;
    float temp;
    sscanf(_tclAnswer, "%f", &temp);
    cap = temp;
  }

  void
  designTiming::getLoadCap(double &riseCap, double &fallCap, string pinName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetCellLoadCap " + pinName + "\"";

//     cout<<"_tclInputString: "<<_tclInputString<<endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
//     cout<<"_tclAnswer: "<<_tclAnswer<<endl;
    char *tempStr = _tclAnswer;
    int i = 0;
    int len = strlen(tempStr);
    while(i < len)
      {
        if (tempStr[i] == '{' || tempStr[i] == '}')
          {
            tempStr[i] = ' ';
          }
        i ++;
      }

    char *p;
    p = strtok(tempStr, " {}");
    DVector dv;
    while(p)
      {
//         cout<<p<<endl;
        if (p[0] >= '0' && p[0] <= '9')
          {
            double n = atof(p);
            dv.push_back(n);
          }
        p = strtok(NULL, " {}");
      }

    int num = dv.size();
//     cout<<num<<endl;
//    assert(num > 2);
    riseCap = dv[num-2];
    fallCap = dv[num-1];
  }

  void designTiming::suppressMessage (string message)
  {
    _tclInputString = "DoOnePtCommand \"PtSuppressMessage "+ message + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return;
  }
  // ****************************************************************************
  // getSetupSlack
  // ****************************************************************************
  double
  designTiming::getSetupSlack(string  startFF,
                              string  endFF)
  {
    _tclInputString = "DoOnePtCommand \"PtGetWorstSlackBetweenFlops " + startFF + " " + endFF + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  // ****************************************************************************
  // getHoldSlack
  // ****************************************************************************
  double
  designTiming::getHoldSlack(string	startFF,
                             string	endFF)
  {
    _tclInputString = "DoOnePtCommand \"PtGetBestSlackBetweenFlops " + startFF + " " + endFF + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _holdSlack = _convertToDouble(_answerStr);
    return(_holdSlack);
  }

  // ****************************************************************************
  // getLoadCap
  // ****************************************************************************
  double
  designTiming::getBufferLoadCap(string buffer)
  {
    _tclInputString = "DoOnePtCommand \"PtGetBufferLoadCap " + buffer + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _loadCap = _convertToDouble(_answerStr);
    return(_loadCap);
  }

  // ****************************************************************************
  // getBufInputSlew
  // ****************************************************************************
  double
  designTiming::getBufferInputSlew(string buffer)
  {
    _tclInputString = "DoOnePtCommand \"PtGetBufferInputSlew " + buffer + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _inputSlew = _convertToDouble(_answerStr);
    return(_inputSlew);
  }

  // ****************************************************************************
  // getZeta
  // ****************************************************************************
  double
  designTiming::getZeta(string	parentBuffer,
                        string	childBuffer,
                        string	swapMaster)
  {
    _tclInputString = "DoOnePtCommand \"PtGetCapacitiveShieldingFactor " + parentBuffer + " " + childBuffer + " " + swapMaster + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _zeta = _convertToDouble(_answerStr);
    return(_zeta);
  }

  // ****************************************************************************
  // getMasterOfInstance
  // ****************************************************************************
  string
  designTiming::getMasterOfInstance(string	cellInstance)
  {
    _tclInputString = "DoOnePtCommand \"PtGetMasterOfInstance " + cellInstance + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  // ****************************************************************************
  // getMasterOfInstance
  // ****************************************************************************
  vector<double>
  designTiming::getNewLoads(vector<pair<string, string> >	swapSet,
                            vector<string>				measureSet)
  {
    // We are given a vector pair containing the list of instances and
    // their new cell masters. The goal is to swap the instances to the
    // specified master and measure the loadCap at the output of
    // instance in the measureSet

    // Ideally, we would want to a pass a vector as a tcl list to the pt
    // server and do this operation completely in tcl. But we don't know
    // how to do this yet
    //
    // So, to overcome this, we do the following
    //
    // - for each element (i.e., pair) in the swapSet, we get the
    // cellInstance and the master to which it should be swapped to
    //
    //   - for each cellInstance, we get its current master and store
    //     it.  We can do this PtGetMasterOfInstance
    //
    //   - Now, for each element in the pair, perform swap
    //   - (sequentially)
    //
    // - After all swaps are complete, measure the load cap of each
    // instance in measureSet - We can do this using PtGetBufferLoadCap
    //
    // - After all measurements are complete, we have to restore the
    // original state of the netlist
    vector<pair<string, string> >	_inputSwapSet = swapSet;
    vector<string>				_measureSet = measureSet;

    string _originalMasterOfInstance;

    string _currentCellInstance;
    string _currentCellMaster;

    string _newCellMaster;
    string _instanceUnderMeasurement;

    string _restoreCellInstance;
    string _restoreCellMaster;

    vector<pair<string, string> > _origCellAssociation;
    pair<string, string> _currentCellAssociation;

    vector<double> _measureSetLoadCaps;

    // iterate over the input swap set and get the list of all cellInstances and
    // their original masters
    for(vector<pair<string, string> >::iterator _inputSwapIter = _inputSwapSet.begin();
        _inputSwapIter != _inputSwapSet.end();
        _inputSwapIter++)
      {
        _currentCellInstance = _inputSwapIter->first;
        _newCellMaster = _inputSwapIter->second;

        // get the current master of _currentCellInstance
        _tclInputString = "DoOnePtCommand \"PtGetMasterOfInstance " + _currentCellInstance + "\"";
        _tclExpression = (char*)_tclInputString.c_str();
        Tcl_Eval(_interpreter, _tclExpression);

        _tclAnswer = _interpreter->result;
        string _answerStr(_tclAnswer);
        _currentCellMaster = _answerStr;

        _currentCellAssociation.first = _currentCellInstance;
        _currentCellAssociation.second = _currentCellMaster;
        _origCellAssociation.push_back(_currentCellAssociation);
      }

    // now iterate over the input swap set and perform swaps we do not
    // need to query timing / caps here...just a bunch of swaps
    for(vector<pair<string, string> >::iterator _inputSwapIter = _inputSwapSet.begin();
        _inputSwapIter != _inputSwapSet.end();
        _inputSwapIter++)
      {
        _currentCellInstance = _inputSwapIter->first;
        _newCellMaster = _inputSwapIter->second;

        _tclInputString = "DoOnePtCommand \"PtSwapCell " + _currentCellInstance + " " + _newCellMaster + "\"";
        _tclExpression = (char*)_tclInputString.c_str();
        Tcl_Eval(_interpreter, _tclExpression);

        _tclAnswer = _interpreter->result;
        string _answerStr(_tclAnswer);
        int _returnStatus = _convertToInt(_answerStr);
        if(_returnStatus == 0)
          {
            cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
            exit(1);
          }
      }

    // now query for cap at the output of all the cells specified in the measure set
    for(vector<string>::iterator _measureIter = _measureSet.begin();
        _measureIter != _measureSet.end();
        _measureIter++)
      {
        _instanceUnderMeasurement = *_measureIter;
        _tclInputString = "DoOnePtCommand \"PtGetBufferLoadCap " + _instanceUnderMeasurement + "\"";
        _tclExpression = (char*)_tclInputString.c_str();
        Tcl_Eval(_interpreter, _tclExpression);

        _tclAnswer = _interpreter->result;
        string _answerStr(_tclAnswer);
        double _loadCap = _convertToDouble(_answerStr);
        _measureSetLoadCaps.push_back(_loadCap);
      }

    // now restore the netlist in the server to its original state
    // iterate over original cell associations and perform swap cells
    for(vector<pair<string, string> >::iterator _restoreSwapIter = _origCellAssociation.begin();
        _restoreSwapIter != _origCellAssociation.end();
        _restoreSwapIter++)
      {
        _restoreCellInstance = _restoreSwapIter->first;
        _restoreCellMaster = _restoreSwapIter->second;

        _tclInputString = "DoOnePtCommand \"PtSwapCell " + _restoreCellInstance + " " + _restoreCellMaster + "\"";
        _tclExpression = (char*)_tclInputString.c_str();
        Tcl_Eval(_interpreter, _tclExpression);

        _tclAnswer = _interpreter->result;
        string _answerStr(_tclAnswer);
        int _returnStatus = _convertToInt(_answerStr);
        if(_returnStatus == 0)
          {
            cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
            exit(1);
          }
      }
    return(_measureSetLoadCaps);
  }


  // ****************************************************************************
  // convertToString
  // ****************************************************************************
  string
  designTiming::_convertToString(double x)
  {
    std::ostringstream o;
    o << x;
    return o.str();
  }

  // ****************************************************************************
  // convertToDouble
  // ****************************************************************************
  double
  designTiming::_convertToDouble(const string& s)
  {
    std::istringstream i(s);
    double x;
    i >> x;
    return x;
  }

  // ****************************************************************************
  // convertToInt
  // ****************************************************************************
  int
  designTiming::_convertToInt(const string& s)
  {
    std::istringstream i(s);
    int x;
    i >> x;
    return x;
  }

  // ****************************************************************************
  // getWNSList (SHK)
  // ****************************************************************************
  string
  designTiming::getWNSList(string _clkName)
  {
    //string  _clkName = "rclk";
    _tclInputString = "DoOnePtCommand \"PtListInstance " + _clkName + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    incPtcnt();
    return(_answerStr);
  }

  double
  designTiming::getPathSlack(string  PathString)
  {
    if ( PathString == "" )
    {
      return (0);
    }
    else {
    _tclInputString = "DoOnePtCommand \"PtPathSlack " + PathString + "\"";
    //cout << "tcl :" << _tclInputString << endl;
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << "slack : " << _answerStr << endl;
    double _pathSlack = _convertToDouble(_answerStr);
    incPtcnt();
    return(_pathSlack);
    }
  }

  double
  designTiming::getWorstSlack(string _clkName)
  {
    //string  _clkName = "clk";
    _tclInputString = "DoOnePtCommand \"PtWorstSlack " + _clkName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << "slack : " << _answerStr << endl;
    double _pathSlack = _convertToDouble(_answerStr);
    return(_pathSlack);
  }

  double
  designTiming::getWorstSlackMin(string _clkName)
  {
    //string  _clkName = "clk";
    _tclInputString = "DoOnePtCommand \"PtWorstSlackMin " + _clkName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << "slack : " << _answerStr << endl;
    double _pathSlack = _convertToDouble(_answerStr);
    return(_pathSlack);
  }

  string
  designTiming::getLibCell(string CellName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetLibCell " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getOutPin(string CellName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetOutPin " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getCellFromNet(string NetName)
  {
    if (NetName == "") return "NULL_CELL";
    _tclInputString = "DoOnePtCommand \"PtGetCellFromNet " + NetName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getTermsFromNet(string NetName)
  {
    if (NetName == "") return "NULL_CELL";
    _tclInputString = "DoOnePtCommand \"PtGetTermsFromNet " + NetName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }


  string
  designTiming::getPortsFromNet(string NetName)
  {
    if (NetName == "") return "NULL_CELL";
    _tclInputString = "DoOnePtCommand \"PtGetPortsFromNet " + NetName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string 
  designTiming::getFanoutCellsFromCell (string CellName)
  {
    if (CellName == "") return "EMPTY_CELL_GIVEN";
    _tclInputString = "DoOnePtCommand \"PtGetFanoutCells " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getFaninGateAtTerm (string TermName)
  {
    if (TermName == "") return "EMPTY_TERM_GIVEN";
    _tclInputString = "DoOnePtCommand \"PtGetFaninGateAtTerm " + TermName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getFaninPortAtTerm (string TermName)
  {
    if (TermName == "") return "EMPTY_TERM_GIVEN";
    _tclInputString = "DoOnePtCommand \"PtGetFaninPortAtTerm " + TermName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getTerminalConstValue (string TermName)
  {
    if (TermName == "") return "EMPTY_TERM_GIVEN";
    _tclInputString = "DoOnePtCommand \"PtGetTermConstValue " + TermName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getTopNetName (string NetName)
  {
    if (NetName == "") return "EMPTY_TERM_GIVEN";
    _tclInputString = "DoOnePtCommand \"PtGetTopNetName " + NetName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  string
  designTiming::getInst(string PinName)
  {
    _tclInputString = "DoOnePtCommand \"PtGetInst " + PinName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  void
  designTiming::removeFFDelay(string CellName)
  {
    _tclInputString = "DoOnePtCommand \"PtRemoveFFDelay " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    //_tclAnswer = _interpreter->result;
    //string _answerStr(_tclAnswer);
    //return(_answerStr);
  }

  void designTiming::setFalsePathsThroughAllCells()
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePathsForCells\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
  }

  void designTiming::setFalsePathsThroughAllPins()
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePathsForPins\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
  }

  void
  designTiming::resetPathsThroughAllCells ()
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathsForCells\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    //_tclAnswer = _interpreter->result;
    //string _answerStr(_tclAnswer);
    //return(_answerStr);
  }

  void designTiming::resetPathThrough(string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathThrough " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path_through; check the status on ptserver" << endl;
        //exit(1);
      }

  }

  void designTiming::test(string input)
  {
    _tclInputString = "DoOnePtCommand \"testPrintList " + input + "\"";// Terminals in PowerOpt and Pins in PrimeTime are equivalent
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the term" << input << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathForCell(string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathForCell " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the cell" << cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetRisePathForTerm(string term)
  {
    _tclInputString = "DoOnePtCommand \"PtRRPT " + term + "\"";// Terminals in PowerOpt and Pins in PrimeTime are equivalent
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the term" << term << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetFallPathForTerm(string term)
  {
    _tclInputString = "DoOnePtCommand \"PtRFPT " + term + "\"";// Terminals in PowerOpt and Pins in PrimeTime are equivalent
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the term" << term << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathForTerm(string term)
  {
    _tclInputString = "DoOnePtCommand \"PtRPT " + term + "\"";// Terminals in PowerOpt and Pins in PrimeTime are equivalent
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the term" << term << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::setFalsePathForTerm(string term)
  {
    _tclInputString = "DoOnePtCommand \"PtSFPT " + term + "\"";// Terminals in PowerOpt and Pins in PrimeTime are equivalent
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the term" << term << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathForCellRise(string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathForCellRise " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the cell" << cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathForCellFall(string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathForCellFall " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path for the cell" << cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathFrom(string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathFrom " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path_through; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void designTiming::resetPathTo (string cell)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPathTo " + cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: reset_false_path_through; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  bool
  designTiming::sizeCell(string	  cellInstance,
                         string	  cellMaster)
  {
    _tclInputString = "DoOnePtCommand \"PtSizeCell " + cellInstance + " " + cellMaster + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
        return false;
      }
     return true;
  }
  void
  designTiming::setFalsePath(string  PathString)
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePath " + PathString + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: set_false_path; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void
  designTiming::setFalsePathTo(string  Cell)
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePathTo " + Cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: set_false_path_to " << Cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void
  designTiming::setFalsePathFrom(string  Cell)
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePathFrom " + Cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: set_false_path_from " << Cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void
  designTiming::setFalsePathThrough(string  Cell)
  {
    _tclInputString = "DoOnePtCommand \"PtSetFalsePathThrough " + Cell + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0) {
        cout << "Fatal error: set_false_path_through " << Cell << " ; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void
  designTiming::updateTiming()
  {
    _tclInputString = "DoOnePtCommand \"PtUpdateTiming \"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
     if(_returnStatus == 0)
      {
        cout << "Fatal error: updateTiming; check the status on ptserver" << endl;
      }
    
  }

  void
  designTiming::updatePower()
  {
    _tclInputString = "DoOnePtCommand \"PtUpdatePower\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: updatePower; check the status on ptserver" << endl;
      }
  }

  void
  designTiming::resetPath(string  PathString)
  {
    _tclInputString = "DoOnePtCommand \"PtResetPath " + PathString + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: reset_path; check the status on ptserver" << endl;
        //exit(1);
      }
  }

  void
  designTiming::writeChange(string FileName)
  {
    _tclInputString = "DoOnePtCommand \"PtWriteChange " + FileName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: WriteChange; check the status on ptserver" << endl;
      }
  }

  void
  designTiming::readTcl(string FileName)
  {
    _tclInputString = "DoOnePtCommand \"PtReadTcl " + FileName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: ReadTcl; check the status on ptserver" << endl;
      }
  }

  void
  designTiming::reportSlack(string FileName)
  {
    _tclInputString = "DoOnePtCommand \"PtReportSlack " + FileName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: ReportSlack; check the status on ptserver" << endl;
      }
  }

  double
  designTiming::getFFSlack(string  endFF)
  {
    _tclInputString = "DoOnePtCommand \"PtFFSlack " + endFF + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  double
  designTiming::getFFMinSlack(string  endFF)
  {
    _tclInputString = "DoOnePtCommand \"PtFFMinSlack " + endFF + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  double
  designTiming::getCellSlack(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellSlack " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getCellMinSlack(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtMinSlack " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getCellDelay(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellDelay " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getFFQDelay(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCK2QDelay " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  // Dependancy check between two F/Fs
  bool
  designTiming::checkDependFF(string startFF, string endFF)
  {
    _tclInputString = "DoOnePtCommand \"PtGetSizePathFF " + startFF + " " + endFF + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    if ( _answerStr == "0" ) return false;
    else return true;
  }

  void
  designTiming::setCell(string cell, string cell_name)
  {
    _tclInputString = "DoOnePtCommand \"PtSetCell " + cell + " " + cell_name + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  void
  designTiming::addtoCol(string collection, string cell_name)
  {
    _tclInputString = "DoOnePtCommand \"PtAddtoCol " + collection + " " + cell_name + "\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }


  double
  designTiming::getCellToggleRate(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellToggleRate " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    //cout << _answerStr << endl;
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getCellPower(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellPower " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getCellLeak(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellLeak " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    if (_setupSlack < 0 || _setupSlack > 0.001) return ((double) 0);
    else return(_setupSlack);
    }
  }

  double
  designTiming::getCellArea(string CellName)
  {
    if(CellName == "") return((double) 0);
    else {
     _tclInputString = "DoOnePtCommand \"PtCellArea " + CellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
    }
  }

  double
  designTiming::getTotalPower()
  {
     _tclInputString = "DoOnePtCommand \"PtTotalPower \"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  double
  designTiming::getLeakPower()
  {
     _tclInputString = "DoOnePtCommand \"PtLeakPower \"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  double
  designTiming::getArea()
  {
     _tclInputString = "DoOnePtCommand \"PtArea \"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    double _setupSlack = _convertToDouble(_answerStr);
    return(_setupSlack);
  }

  void
  designTiming::writeSDF(string SDF)
  {
    _tclInputString = "DoOnePtCommand \"PtWriteSDF " + SDF + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  void
  designTiming::putCommand(string Command)
  {
    _tclInputString = "DoOnePtCommand \"PtCommand " + Command + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  void
  designTiming::saveSession(string file)
  {
    _tclInputString = "DoOnePtCommand \"PtSaveSession " + file + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  void
  designTiming::restoreSession(string file)
  {
    _tclInputString = "DoOnePtCommand \"PtRestoreSession " + file + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  void
  designTiming::linkLib(string VOLT)
  {
    _tclInputString = "DoOnePtCommand \"PtLinkLib " + VOLT + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

  bool
  designTiming::makeResizeTcl(string target1, string target2)
  {
    _tclInputString = "DoOnePtCommand \"PtMakeResizeTcl " + target1 + " " + target2 + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
        return false;
      }
     return true;
  }

  bool
  designTiming::makeEcoTcl(string file, string target)
  {
    _tclInputString = "DoOnePtCommand \"PtMakeEcoTcl " + file + " " + target + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cerr << "Fatal error: size_cell failed; check the status on ptserver" << endl;
        return false;
      }
     return true;
  }

  bool
  designTiming::makeLeakList(string inFile, string outFile)
  {
    _tclInputString = "DoOnePtCommand \"PtMakeLeakList " + inFile + " " + outFile + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cerr << "Fatal error: PtMakeLeakList: check the status on ptserver" << endl;
        return false;
      }
     return true;
  }

  string
  designTiming::getLibNames ()
  {
    _tclInputString = "DoOnePtCommand \"PtGetLibNames\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    return(_answerStr);
  }

  void
  designTiming::makeCellList(string libName, string outFile)
  {
    _tclInputString = "DoOnePtCommand \"PtMakeCellList " + libName + " " + outFile + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
  }

  void
  designTiming::makeFinalEcoTcl(string target)
  {
    _tclInputString = "DoOnePtCommand \"PtMakeFinalEcoTcl " + target + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
  }

  void
  designTiming::writeVerilog(string target)
  {
    _tclInputString = "DoOnePtCommand \"PtWriteVerilog " + target + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    int _returnStatus = _convertToInt(_answerStr);
    if(_returnStatus == 0)
      {
        cout << "Fatal error: WriteVerilog; check the status on ptserver" << endl;
      }
  }

 bool
  designTiming::checkMaxTran(string cellName)
  {
    _tclInputString = "DoOnePtCommand \"PtCheckMaxTran " + cellName + "\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    if (_answerStr == "0") return false;
    else return true;
  }

 bool
  designTiming::checkPT()
  {
    _tclInputString = "DoOnePtCommand \"PtCheckPT\"";
    incPtcnt();
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);
    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
    if (_answerStr == "1") return true;
    else return false;
  }

  void
  designTiming::Exit()
  {
    _tclInputString = "DoOnePtCommand \"PtExit\"";
    _tclExpression = (char*)_tclInputString.c_str();
    Tcl_Eval(_interpreter, _tclExpression);

    _tclAnswer = _interpreter->result;
    string _answerStr(_tclAnswer);
  }

}
