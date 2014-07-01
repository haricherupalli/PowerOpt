#ifndef __POWEROPT_H__
#define __POWEROPT_H__

#include <map>
#include <cassert>
#include <fstream>
#include <iostream>
//#include <unordered_map>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <list>
#include <set>

#include "Gate.h"
#include "Bus.h"
#include "Grid.h"
#include "typedefs.h"
#include "macros.h"
#include "Net.h"
#include "Subnet.h"
#include "Chip.h"
#include "Box.h"
#include "analyzeTiming.h"
#include "SetTrie.h"

using namespace std;

namespace POWEROPT {

class PowerOpt {
    public:
    //constructors
    PowerOpt() // TODO HARI: MAKE THIS A SINGLETON CLASS!!
    { }

    // destructor
    ~PowerOpt()
    { }


    //modifiers
    void readStdDevMap();
    void initVariations(int iterNo);
    void readDivFile(string divFileStr);
    void readEnvFile(string envFileStr);
    void readCmdFile(string cmdFileStr);
    void openFiles();
    void closeFiles();
    void wait(int seconds);
    void makeOA();
    void exePTServer(bool isPost);
    void exePTServerOne(bool isPost);
    void exeGTServerOne(bool isPost);
    void restorePTLib(int volt, designTiming *T);
    void exeMMMC();
    void exePTServerMMMC(int index);
    void exeGTServerMMMC(int index);
    void exeSOCE();

    void reportToggleFF(designTiming *T);
    void parsePtReport();
    void readFlopWorstSlacks();
    void check_for_toggles(int cycle_num, int cycle_time);
    void check_for_flop_toggles(int cycle_num, int cycle_time, designTiming* T);
    void check_for_flop_toggles_fast(int cycle_num, int cycle_time, designTiming* T);
    void check_for_flop_toggles_fast_subset(int cycle_num, int cycle_time, designTiming* T);
    void find_dynamic_slack(designTiming* T);
    void find_dynamic_slack_2(designTiming* T);
    void reset_all (designTiming* T);
    void read_unt_dump_file();
    void check_for_flop_paths(int cycle_num, int cycle_time);
    void trace_toggled_path(int cycle_num, int cycle_time);
    void parseVCDALL(designTiming *T);
    void parseVCDALL_mode_15(designTiming *T);
    int parseVCD_mode_15(string VCDfilename, designTiming *T, int parse_cyc, int cycle_offset);
    int parseVCDMode15(string VCDfilename, designTiming *T, int parse_cyc, int cycle_offset);
    int parseVCD_mode_15_new(string vcdfilename, designTiming  *T, int parse_cyc, int cycle_offset);
    void handle_toggled_nets(vector<string> & toggled_nets, designTiming* T, int cycle_num, int cycle_time);
    void read_modules_of_interest();
    void createSetTrie();
    void optimizeTargER(designTiming *T);
    void reducePower(designTiming *T);
    int  resizePaths(designTiming *T);
    void reducePowerSO(designTiming *T);
    bool resizePathCell(designTiming *T, Gate *g, string PathString, int PathItr);
    void calER(designTiming *T);
    void calER_SSTA(designTiming *T);
    double reportRZOH(designTiming *T);
    double reportHBOH(designTiming *T);
    void printSlackDist(designTiming *T);
    double calErrorRate(int total_cycle, designTiming *T);
    int  checkNumErFF();
    void clearCheck();
    void extCriticalPaths(designTiming *T);
    // JMS-SHK begin
    void swap_razor_flops(designTiming *T);
    // JMS-SHK end
    void resizeCycleVectors(int total_cyc);
    int  parseVCD(string VCDfilename, designTiming *T, int parse_cyc, int cycle_offset);
    void extractPaths(int cycle_num, int &pathID);
    void extractPaths_mode_15(int cycle_num, int &pathID , designTiming * T);
    void clearToggled();
    void findToggledPaths(Gate *g, Gate *faninD, vector<GateVector> &paths);
    void findToggledPaths_mode_15(Gate *g, Gate *faninD, vector<GateVector> &paths, designTiming * T);
    string getPathString (GateVector paths);


    void updateCellDelay(designTiming *T, int voltage);
    void updateBiasCell();
    void writeOut(designTiming *T);
    void setDontTouch();
    void findLeakage(designTiming *T);
    void findLeakageLib(int lib_idx);
    void findLeakagePT(designTiming *T);
    double getLeakPower();
    void testCellPowerDelay(string cellName, designTiming *T);
    void testCellSize();
    void testRunTime();
    void testSlack();
    void restoreNet(designTiming *T);
    double getCellSlack(Gate *g, designTiming *T);
    double computeSF1(Gate *g, designTiming *T, bool change);
    double computeSF2(Gate *g, designTiming *T, bool change);
    double computeSF3(Gate *g, designTiming *T, bool change);
    double computeSF4(Gate *g, designTiming *T, bool change);
    double computeSF5(Gate *g, designTiming *T, bool change);
    double computeSF6(Gate *g, designTiming *T, bool change);
    double computeSF7(Gate *g, designTiming *T, bool change);
    double computeSF8(Gate *g, designTiming *T);
    double computeSF9(Gate *g, designTiming *T);
    double computeSF10(Gate *g, designTiming *T);
    double computeSensitivity(Gate *g, designTiming *T, bool change);
    double computeTimingSens(Gate *g, designTiming *T);
    double computeTimingSF1(Gate *g, designTiming *T);
    double computeTimingSF2(Gate *g, designTiming *T, bool change);
    double computeTimingSF3(Gate *g, designTiming *T, bool change);
    int downsizeAll(designTiming *T);
    int downsizeTest(designTiming *T);
    int kick_move(designTiming *T, double kick_val);
    void updateCellLib(designTiming *T);
    void setBestLib(designTiming *T);
    void restoreLib(designTiming *T);
    void checkCTSCells();
    void setBaseName();
    string findBaseName(string master);
    string findSuffixName(string master);
    string findDownCell(string master);
    string findUpCell(string master);
    string findDownSwap(string master);
    string findUpSwap(string master);
    string findDownSize(string baseName);
    string findUpSize(string baseName);
    bool isDownsizable(Gate *g);
    bool isUpsizable(Gate *g);
    void sensitivityUpdate(Gate *g, designTiming *T);
    void sensitivityUpdateTiming(Gate *g, designTiming *T);
    void sensitivityUpdateAll(list<Gate *> gate_list, designTiming *T);
    void sensitivityUpdateTimingAll(list<Gate *> gate_list, designTiming *T);
    void sensitivityUpdateFast(list<Gate *> gate_list, designTiming *T);
    void reduceLeak(designTiming *T);
    void reduceLeakMultiSwap(designTiming *T);
    void checkGateFanin(Gate *g);
    void checkGateFanout(Gate *g);
    void optTiming(designTiming *T);
    void PTEvaluation(designTiming *T);
    void PTEvaluation(designTiming *T, int freq, int test);
    void swapCells();
    void reportWNS();
    void makeLibList();
    void readLibCells(designTiming *T);
    //void readLibCellsFile();
    void updateLibCells();
    void restoreCells();
    void setInitWNS(double x) { initWNS = x; }
    void setInitWNSMin(double x) { initWNSMin = x; }
    double  getInitWNS() { return initWNS; }
    double  getInitWNSMin() { return initWNSMin; }
    double  getTargWNS() {
        if (initWNS > 0) return (double) 0;
        else return initWNS; }
    double  getTargWNSMin() {
        if (initWNSMin > 0) return (double) 0;
        else return initWNSMin; }
    double  getTargWNS_mmmc(int i) {
        if (initWNS_mmmc[i] > 0) return (double) 0;
        else return initWNS_mmmc[i]; }
    double  getTargWNSMin_mmmc(int i) {
        if (initWNSMin_mmmc[i] > 0) return (double) 0;
        else return initWNSMin_mmmc[i]; }
    void setInitLeak(double x) { initLeak = x; }
    string getRegCellsFile() { assert(regCellsFile.length()); return regCellsFile; }
    double  getInitLeak() { return initLeak; }
    void setInitPower(double x) { initPower = x; }
    double  getInitPower() { return initPower; }
    void setCurWNS(double x) { curWNS = x; }
    double  getCurWNS() { return curWNS; }
    void setCurLeak(double x) { curLeak = x; }
    double  getCurLeak() { return curLeak; }
    void setItrcnt(int x) { itrcnt = x; }
    int  getItrcnt() { return itrcnt; }
    void incItrcnt() { itrcnt ++; }
    void setVltcnt(int x) { vltcnt = x; }
    int  getVltcnt() { return vltcnt; }
    void incVltcnt() { vltcnt ++; }
    void setSwapcnt(int x) { swapcnt = x; }
    int  getSwapcnt() { return swapcnt; }
    void incSwapcnt() { swapcnt ++; }
    void setSwaptry(int x) { swaptry = x; }
    int  getSwaptry() { return swaptry; }
    void incSwaptry() { swaptry ++; }
    //void setPtcnt(int x) { ptcnt = x; }
    //int  getPtcnt() { return ptcnt; }
    //void incPtcnt() { ptcnt ++; }
    void setSwapstep(int x) { swapstep = x; }
    int  getSwapstep() { return swapstep; }
    void setExeOp(int x) { exeOp = x; }
    int  getExeOp() { return exeOp; }
    void setCurOp(int x) { curOp = x; }
    int  getCurOp() { return curOp; }
    void setSwapOp(int x) { swapOp = x; }
    int  getSwapOp() { return swapOp; }
    void setSensFunc(int x) { sensFunc = x; }
    int  getSensFunc() { return sensFunc; }
    void setUpdateOp(int x) { updateOp = x; }
    int  getUpdateOp() { return updateOp; }
    void setStopCond(int x) { stopCond = x; }
    int  getStopCond() { return stopCond; }
    void setStopCond2(int x) { stopCond2 = x; }
    int  getStopCond2() { return stopCond2; }
    void setHoldCheck(int x) { holdCheck = x; }
    int  getHoldCheck() { return holdCheck; }
    void setOaGenFlag(int x) { oaGenFlag = x; }
    int  getOaGenFlag() { return oaGenFlag; }
    void setPtClientTcl(string x) { ptClientTcl = x; }
    string getPtClientTcl() { return ptClientTcl; }
    void setReportFile(string x) {  reportFile= x; }
    string getReportFile() { return reportFile; }
    void setMaxTrCheck(int x) { maxTrCheck = x; }
    int  getMaxTrCheck() { return maxTrCheck; }
    void setDivNum(int x) { divNum = x; }
    int  getDivNum() { return divNum; }
    void setGuardBand(float xx) { guardBand = xx; }
    float getGuardBand() { return guardBand; }
    void setSensVar(float xx) { sensVar = xx; }
    float getSensVar() { return sensVar; }
    void setChglimit(double x) { chglimit = x; }
    double getChglimit() { return chglimit; }
    bool getPtLogSave() { return ptLogSave; }
    bool getVarSave() { return varSave; }
    bool getLeakPT() { return leakPT; }
    bool getMmmcOn() { return mmmcOn; }
    bool getPtpxOff() { return ptpxOff; }
    bool getUseGT() { return useGT; }
    bool getUseGTMT() { return useGTMT; }
    bool getNoSPEF() { return noSPEF; }
    bool getNoDEF() { return noDEF; }
    bool getSOCEFlag() { return exeSOCEFlag; }
    void setSOCEFlag(bool x) { exeSOCEFlag = x; }
    string getVerilogOut() { return verilogOutFile; }
    int  getTestValue1() { return testValue1; }
    int  getTestValue2() { return testValue2; }
    void clearToggledPath() { toggledPaths.clear();}
    void clearCriticalPath() { criticalPaths.clear();}

    //string pathConv(string PathIn);
    void clearHolded();
    void compBiasCellName();
    double convertToDouble(const string& s);
    void setLDelayWeight(double w) { m_LDelayWeight = w; }
    void setWDelayWeight(double w) { m_WDelayWeight = w; }
    void addGate(Gate *g);
    void addToggledPath(Path *p) { toggledPaths.push_back(p);}
    void addCriticalPath(Path *p) { criticalPaths.push_back(p);}
    void addPlusPath(Path *p) { plusPaths.push_back(p);}
    void addMinusPath(Path *p);    // add a path to P_-
    void removePlusPath(Path *p) { plusPaths.remove(p); }  // remove a path from P_+
    void addTerminal(Terminal *t) { terms.push_back(t); }
    void addPad(Pad *p) { m_pads.push_back(p); }
    void addNet(Net *n) { nets.push_back(n); }
    void setDoseSens(double sens) { doseSensitivity = sens; }
    void setDim(int x1, int y1, int x2, int y2)
    {
      chip.setDim(x1, y1, x2, y2);
    }
    void addSubnet(Subnet *s) { subnets.push_back(s); }
    void setLayoutName(string lName) { layoutName = lName; }
    void setCellName(string cName) { circuitName = cName; }
    void setLibName(string lName) { libraryName = lName; }
    void setClockName(string cName) { clockName = cName; }
    void setOADir(char *dir) { oaDir = dir; }
    void setResDir(char *dir) { resDir = dir; }
    void setVCDName(char *VCDName) { VCDfilename = VCDName; }
    void test();
    bool netPartition(designTiming *T);
    void netPartitionPT(designTiming *T);
    bool addGateDFS (Gate *g, Divnet *d);
    bool addGateFiDFS (Gate *g, GateVector *gv);
    bool addGateFoDFS (Gate *g, GateVector *gv);
    int checkDES( Divnet *d, designTiming *T );
    int checkSRC( Divnet *d, designTiming *T );
    int checkMaxTr( designTiming *T );
    int fixMaxTr( designTiming *T );
    bool checkViolation( string cellInst, designTiming *T );
    bool checkViolationPT( string cellInst, designTiming *T );
    bool checkViolationMMMC( string cellInst, int mmmc_num );
    void initSTA( designTiming *T );
    void exitPT();

    //accossers
    void getCellPowers(designTiming *T);
    int getNumErrorCycles() { return numErrorCycles; }
    int getNumMinusCycles() { return numMinusCycles; }
    int getDeltaErrors(Path *p);  // returns the additional number of error cycles if the path becomes negative a slack path
    double getDoseSens() { return doseSensitivity; }
    StrIntMap &getGateNameIdMap() { return gateNameIdMap; }
    StrIntMap &getLibNameIdMap() { return libNameIdMap; }
    StrVector &getLibNameVector() { return libNameVect; }
    //vector< vector<Libcell> > &getCellLibrary() { return cellLibrary; }
    StrIntMap &getPadNameIdMap() { return padNameIdMap; }
    StrIntMap &getSubnetNameIdMap() { return subnetNameIdMap; }
    StrVector &getCellNameVector() { return cellNameVect; }
    void print();
    int getGridNum() { return grids.size(); }
    Grid *getGrid(int i) { assert(0 <= i && i < grids.size()); return grids[i]; }
    int getGateNum() { return m_gates.size(); }
    Gate *getGate(int i) { assert(0 <= i && i < m_gates.size()); return m_gates[i]; }
    int getTerminalNum() { return terms.size(); }
    Terminal *getTerminal(int i) { assert(0 <= i && i < terms.size()); return terms[i]; }
    int getPadNum() { return m_pads.size(); }
    Pad *getPad(int i) { assert(0 <= i && i < m_pads.size()); return m_pads[i]; }
    int getNetNum() { return nets.size(); }
    Net *getNet(int i) { assert(0 <= i && i < nets.size()); return nets[i]; }
    int getToggledPathsNum() { return toggledPaths.size(); }
    int getCriticalPathsNum() { return criticalPaths.size(); }
    int getPlusPathsNum() { return plusPaths.size(); }
    int getMinusPathsNum() { return minusPaths.size(); }
    Path *getCriticalPath(int i) { assert(0 <= i && i < criticalPaths.size()); return criticalPaths[i]; }
    void saveResult();
    void saveLeakResult();
    void savePowerOpt();
    int getLX() { return chip.getLX(); }
    int getRX() { return chip.getRX(); }
    int getBY() { return chip.getBY(); }
    int getTY() { return chip.getTY(); }
    int getChipWidth() { return chip.getWidth(); }
    int getChipHeight() { return chip.getHeight(); }
    Chip & getChip() { return chip; }
    void printBox(int lx, int by, int rx, int ty) { cout<<"["<<lx<<","<<by<<"]->["<<rx<<","<<ty<<"]"; }
    void printPoint(int x, int y) { cout<<"["<<x<<","<<y<<"]"; }
    //functions
    void calTotalSPEFDelay(bool opt, bool compPath);
    void calTotalLeakage(bool opt);
    void analyzeTiming();
    void saveCellDelay();
    void loadCellDelay();
    void saveSlewAndCap();
    void loadSlewAndCap();
    void loadDelayTab();
    void loadLeakageTab();
    void loadWDelayTab();
    void loadWLeakageTab();
    void prepareTabs();
    void constructCellNameVect();
    void decideCoeff();
    int findCapIndex(double cap);
    int findSlewIndex(double slew);
    void chipPartition();
    void calDelayAndLeakage();
    void calHoldDelay();
    void makeSMLPProgram();
    void solveSMLP();
    double SMAval(int row, int col);
    void makeSMQuadraticProgram();
    void solveSMQP();
    void makeWLSMQuadraticProgram();
    void solveWLSMQP();
    void saveGates();
    void removeLoop();
    void saveWireDelay();
    void loadWireDelay();
    void findSPEFCriticalPath(bool opt, DVector &dist, double totalDelay);
    double getMaxPOWireDelay(Gate *g, Terminal * &t);
    double getMaxPIWireDelay(Gate *g, Terminal * &t);
    double getMinWireDelayBetweenGates(Gate *source, Terminal * &sT, Gate *sink, Terminal * &tT);
    double getMaxWireDelayBetweenGates(Gate *source, Terminal * &sT, Gate *sink, Terminal * &tT);
    double getWireDelayBetweenGates(Gate *source, Terminal * sT, Gate *sink, Terminal * tT);
    int calWLFFVar(int doseVarNum, int gateVarNum);
    int calFFVar(int doseVarNum, int gateVarNum);
    void STA(int index);
    void DM_ld_timing(int index);
    void DM_load_timing(char * TFile);
    bool cellSwappingByPath();
    double getMCT() { return mct; }
    double getTNS() { return tns; }
    double getCPS() { return cps; }
    int getNVP() { return nvp; }
    void setRowHeight(int h) { chip.setRowH(h); }
    void setRowWidth(int w) { chip.setRowW(w); }
    int getRowHeight() { return chip.getRowH(); }
    int getRowWidth() { return chip.getRowW(); }
    void initSiteOrient(int numH) { chip.initSiteOrient(numH); }
    void setSiteOrient(int index, int value) { chip.setSiteOrient(index, value); }
    int getSiteOrient(int index) { chip.getSiteOrient(index); }
    void setChipLeft(int l) { chip.setChipLeft(l); }
    void setChipRight(int r) { chip.setChipRight(r); }
    void setChipBottom(int b) { chip.setChipBottom(b); }
    void setChipTop(int t) { chip.setChipTop(t); }
    void setChipX(int x) { chip.setChipX(x); }
    void setChipY(int y) { chip.setChipY(y); }
    int getChipLeft() { return chip.getChipLeft(); }
    int getChipRight() { return chip.getChipRight(); }
    int getChipBottom() { return chip.getChipBottom(); }
    int getChipTop() { return chip.getChipTop(); }
    int getChipX() { return chip.getChipX(); }
    int getChipY() { return chip.getChipY(); }
    string getBiasCellName(Gate *g, double optCL);
    void stat();
    void setRowNum(int r) { m_rowNum = r; }
    void setColNum(int c) { m_colNum = c; }
    void setSaveAnaData(bool b) { saveAnaData = b; }
    string getLibName() { return libraryName; }
    string getClockName() { return clockName; }
    string getCellName() { return circuitName; }
    string getViewName() { return layoutName; }
    string getVCDName() { return VCDfilename; }
    bool DM_ld_qor(int index);
    bool DM_load_qor(char * TFile, int index);
    void DM_ld_leak(int index);
    void DM_load_leak(char * TFile);
    void setDelta(double d) { delta = d; }
    bool needItr();
    void saveSTAResult();
    void calHPWL();
    void openLogFile(char *fileName);
    void closeLogFile() { logFile.close(); }
    void setKeepLog(bool b) { keepLog = b; }
    bool getKeepLog() { return keepLog; }
    void setIter(int i);
    ofstream &getLogFile() { return logFile; }
    void addRuntime(double t) { runtimes.push_back(t); }
    void setLU(int l, int u) { L = l; U = u; }
    void setPathBoundRatio(double r) { pathBoundRatio = r; }
    void setGateBoundRatio(double r) { gateBoundRatio = r; }
    void fastStat();
    void fixGates();
    void rollBack();
    void decSwapPairs() { totalSwapPairs -= swapPairs; }
    int getSwapPairNum() { return swapPairs; }
    int getTotalSwapPairNum() { return totalSwapPairs; }
    void clearMem();
    void storeOptSol(int index);
    void setHierarCase(bool b) { hierarCase = b; }
    void setSwapNumEachRound(int num) { swapNumEachRound = num; }
    void setNeedFix(bool b) { needFix = b; }
    void setWholeTimingGraph(bool b) { wholeTimingGraph = b; }
    bool optWholeTimingGraph() { return wholeTimingGraph; }
    void setGridWidth(int i) { m_gridWidth = i; }
    void setGridHeight(int i) { m_gridHeight = i; }
    int getGridWidth() { return m_gridWidth; }
    int getGridHeight() { return m_gridHeight; }
    void addSmoothCons();
    void makeFFQP();
    void solveFFQP();
    void BFSFFCons(Gate *ff, int &varIndex);
    void BFSCLKPath(int &varIndex);
    void BFSMarkCells();
    void setBothWL(bool b) { m_bothWL = b; }
    void setOnlyW(bool b) { m_onlyW = b; }
    void setOnlyL(bool b) { m_onlyL = b; }
    void setOptClock(bool b) { m_optClock = b; }
    bool getOptClock() { return m_optClock; }
    void compWLCellMasters();
    void calTotalDeltaLeakage();
    void calTotalDeltaDelay();
    void calLeakDelayFactor();
    void setSiteBox(int l, int b, int r, int t)
    {
        m_siteBox.set(l, b, r, t);
    }
    int getSiteLeft() { return m_siteBox.left(); }
    int getSiteRight() { return m_siteBox.right(); }
    int getSiteBottom() { return m_siteBox.bottom(); }
    int getSiteTop() { return m_siteBox.top(); }
    void setCellSwapping(bool b) { m_cellSwapping = b; }

    int getTokenI(string line, string option);
    float getTokenF(string line, string option);
    char* getTokenC(string line, string option);
    string getTokenS(string line, string option);

    int cmpString(string wild, string pattern);

    private:
    int varMapWidth;      // width in grids of the variation map (currently width*width grids)
    int varMapSeed;       // random seed for gaussian deviates
    map<string,vector<double> > stdDevMap; // used to lookup the stdDev for a gate
    string varMapMatfile; // file containing the A matrix for the variation map
    string varMapSigmafile; // file containing the A matrix for the variation map
    int varIteration; // number of iteration

    vector<double> path_slack_store;  // stores slack of P+ paths before swaps to know which paths should be in a Gate's path list (based on which path slacks are affected by changing the gate)
    vector<string> gate_master_store; // stores the master cell used for a gate
    map<string,Path*> path_exists_dictionary;  // used to lookup whether or not path already exists
    vector<int> error_cycles;  // set of cycles in which at least one negative slack path is toggled
    vector<int> minus_cycles;  // set of cycles in which at least one path in P_- (minus_paths) is toggled
    vector<int> delta_cycles;  // set of unique toggle cycles for a path (declared here so we don't have to allocate and destroy for every call to getDeltaErrors)
    vector< vector<int> > error_cycle; // temporary storage used during calErrorRate, size is (2, total_cycles);
    int numErrorCycles;    // number of cycles in which a negative slack path toggles (not necessarily equal to errror_cycles.size() )
    int numMinusCycles;    // number of cycles in which a path in P_- toggles (not necessarily equal to minus_cycles.size() )
    double doseSensitivity; //dose sensitivity
    GateVector m_gates;
    map <string, Gate*> gate_name_dictionary;
    TerminalVector terms;
    PadVector m_pads;
    NetVector nets;
    SubnetVector subnets;
    Chip chip;
    int m_rowNum, m_colNum;
    int m_gridWidth;
    int m_gridHeight;
    GridVector grids;
    FVector dosemapW;
    FVector dosemapL;
    PathVector toggledPaths;
    PathVector criticalPaths;
    list<Path *> plusPaths;
    PathVector minusPaths;
#ifdef EXP6
    PathVector checkPaths;  // check these paths to make sure downsizing made them into negative slack paths
    int spareCycles;  // extra cycles from paths that we put into P_- that still have positive slack
#endif
    PathVector totalCriticalPaths;
    int repOnePath;
    DVector pathSlacks;
    int m_M, m_N;
    FVector m_B, m_C, m_C2;
    CharVector Z;
    FVector m_LB, m_UB;
    DVector X;
    int minGateNumOnPath;
    int maxGateNumOnPath;
    double LPowerUB;
    double m_totalDelay;
    double m_optTotalDelay;
    double totalLeakage;
    double optTotalLeakage;
    double m_totalHoldDelay;
    StrIntMap gateNameIdMap;
    StrIntMap subnetNameIdMap;
    StrIntMap padNameIdMap;
    StrIntMap m_gateVarMap;
    StrVector libNameVect;
    StrIntMap libNameIdMap;
    StrStrMap libFootprintMap;
    StrIntMap libNameMap;
    StrDoubleMap libDelayMap;
    StrDoubleMap libLeakageMap;

    vector< LibcellVector > cellLibrary;  // reference master cells by type
    IntStrMap m_QPVars;
    IntStrMap LPVars;
    StrDMap m_setup;
    StrDMap m_hold;
    StrVector cellNameVect;
    StrIntMap cellNameIdMap;
    DVector slewIndices;
    DVector capIndices;
    DDDDVector delayTab;
    DDVector leakageTab;
    DDDDVector WDelayTab;
    DDVector WLeakageTab;
    SparseM SMA;
    double m_LDelayWeight;
    double m_WDelayWeight;
    string layoutName, circuitName, libraryName, clockName, VCDfilename, dutName;
    int NumCritGates;
    vector<string> lefFiles;
    string defFile;
    string sdcFile;
    string spefFile;
    string verilogFile;
    string dbLibPath;
    string libLibPath;
    vector<string> dbLibs;
    vector<string> libLibs;
    vector<string> libSuffix;
    vector<string> mmmcFile;
    vector<designTiming*> T_mmmc;
    vector<string> dontTouchInst;
    vector<string> dontTouchCell;
    vector<string> libNames;
    LibcellVector  m_libCells;
    string vcdPath;
    string ptReport;
    vector<string> vcdFile;
    string saifPath;
    vector<string> saifFile;
    string initSwapFile;
    designTiming* T_main;
    string verilogOutFile;
    string defOutFile;
    string spefOutFile;
    string ptOption;
    string optEffort;
    int totalSimCycle;
    int testValue1;
    int testValue2;
    bool ptLogSave;
    bool varSave;
    bool leakPT;
    bool mmmcOn;
    bool ptpxOff;
    bool useGT;
    bool useGTMT;
    bool noSensUp;
    bool noSPEF;
    bool noDEF;

    IntVector criticalGateIds;
    bool saveAnaData;
    string resDir;
    string oaDir;

    DVector tnss;
    DVector cpss;
    DVector holdTnss;
    DVector holdCpss;
    DVector leaks;
    DVector runtimes;
    int swapPairs;
    int totalSwapPairs;
    double cpl, oldCPL;
    double mct, oldMCT;
    double tns, oldTNS;
    double cps, oldCPS;
    double holdCPS, holdTNS;
    int nvp, oldNVP;
    double optCPS, optTNS;
    double optHoldCPS, optHoldTNS;
    double leak, oldLeak;
    double delta;
    ofstream logFile;
    ofstream m_timingFile;
    ofstream toggle_info_file;
    ofstream unique_not_toggle_gate_sets;
    bool keepLog;
    int iter;
    int L, U;
    double pathBoundRatio;
    double gateBoundRatio;
    double leakageRatio;
    double orgLeak;
    bool hierarCase;
    int swapNumEachRound;
    bool needFix;
    //optimize for leakage
    bool wholeTimingGraph;
    bool m_bothWL;
    bool m_onlyL;
    bool m_onlyW;
    bool m_optClock;
    double m_totalDeltaLLeakage;
    double m_totalDeltaWLeakage;
    double m_totalDeltaLDelay;
    double m_totalDeltaWDelay;
    double m_leakDelayFactor;

    Box m_siteBox;
    bool m_cellSwapping;
    double initWNS;
    double initWNSMin;
    vector<double> initWNS_mmmc;
    vector<double> initWNSMin_mmmc;

    int clockPeriod;
    double initLeak;
    double initPower;
    double curWNS;
    double curLeak;
    int itrcnt;
    int vltcnt;
    int swapcnt;
    int swaptry;
    //int ptcnt;
    int swapstep;
    int sensFunc;
    int sensFuncTiming;
    int exeOp;
    int curOp;
    int swapOp;
    int subsets;
    int updateOp;
    int stopCond;
    int stopCond2;
    int holdCheck;
    int maxTrCheck;
    int oaGenFlag;
    int parseCycle;
    int vcdStartCycle;
    int vcdStopCycle;
    int ignoreClockGates;
    int ignoreMultiplier;
    int vStart;
    int vEnd;
    int vStep;
    float targErrorRate;
    string homeDir;
    string ptClientTcl;
    string ptServerTcl;
    string ptRestoreTcl;
    string ptServerCmdTcl;
    string gtServerCmdTcl;
    string soceCapTableWorst;
    string soceCapTableBest;
    string soceConfFile;
    string soceTclFile;
    string soceConfScript;
    string leakListTcl;
    string leakListFile;
    string libListTcl;
    string libListFile;
    string regCellsFile;
    string moduleNamesFile;
    string worstSlacksFile;
    string ptServerName;
    string reportFile;
    string untDumpFile;
    int ptPort;
    int divNum;
    float guardBand;
    float sensVar;
    double chglimit;
    bool exeSOCEFlag;
    bool chkWNSFlag;
    bool totPWRFlag;

    // JMS-SHK begin
    int numFF;
    int numRZ;
    double areaRZ;
    double areaHB;
    // JMS-SHK end

    list<Gate *> ff_src_list;       // Flip-flop list (source side)
    list<Gate *> ff_des_list;       // Flip-flop list (destination)

    map<string,string> gate_outpin_dictionary;  // dictionary of gate name to outpin gate
    map<string,string>::iterator outpin_lookup; // a pointer to the gate's entry in the source_gate_dictionary
    map<string,string> net_dictionary;          // dictionary of net abbreviation to net name for VCD parsing
    map <string, Bus> bus_dict;
    map<string,string> source_gate_dictionary;  // dictionary of net name to source gate for vcd parsing

    endpoint_pair_list_t endpoint_pair_list;
    bool_vec_list_t path_toggle_bin_list;
    set<string> toggled_gate_set;
    map<vector<string>, float> not_toggled_gate_map;// map of not_toggled_gates and min_worst slack among toggled flip flops
    vector<vector<string> > not_toggled_gate_vector;
    map<string, float> endpoint_worst_slacks;
    SetTrie* tree;


};

}
#endif //__POWEROPT_H__
