#ifndef __GATE_H__
#define __GATE_H__

#include <cassert>
#include <cfloat>
#include <math.h>
#include <bitset>
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
  enum CellFunc { AND, AOI, BUFF, DFF, INV, LH, MUX, NAND, NOR, OAI, OR, XNOR, XOR};

  class Gate {
  public:
    //constructors
    Gate(int i, bool ffflag, string gName, string cName, double l, int tlx, int tby, int twidth, int theight):id(i), FFFlag(ffflag), name(gName), cellName(cName), biasCellName(cName), bestCellName(cName), channelLength(l), type(GATEUNKNOWN), optChannelLength(l), lx(tlx), by(tby), oldLX(tlx), oldBY(tby), width(twidth), height(theight), weight(0), critical(false), swapped(false), checked(false), exclude(false), checkSrc(false), checkDes(false), toggled(false), holded(false), fixed(false), div(NULL), div_src(NULL), div_des(NULL), leakWeight(1.0), disable(false), rolledBack(false), m_compDelay(false), m_LgateBias(0), m_WgateBias(0), Ad(0), Bd(0), Al(0), Bl(0), Cl(0), WAd(0), WBd(0), WAl(0), WBl(0), swActivity(0), slack(0), delta_slack(0), sensitivity(0), deltaLeak(0), CTSFlag(false), cellLibIndex(-1), RZFlag(false), is_end_point(false)
    {
      bbox.set(tlx, tby, tlx+twidth, tby+theight);
      oldCenterX = centerX = tlx + twidth/2;
      oldCenterY = centerY = tby + theight/2;
      toggle_count = 0;
           if (cellName.compare(0,2,"AN") == 0) { func = AND;   gate_op = "&";}
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
      else if (cellName.compare(0,2,"XO") == 0) { func = XOR;   gate_op = "^";}
    
        isClkTree = false;
      cluster_id = -1;
      dead_toggle = false;
      visited = false;
    }
    // a public structure that holds power info for each possible master cell this cell instance can have
    vector<double> cellPowerLookup;
    static int max_toggle_profile_size;
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
    bool getFFFlag() { return FFFlag; }
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
                bool transferDtoQ(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf);
                string getSimValue();


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
    string gate_op;
    bool isClkTree;
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
