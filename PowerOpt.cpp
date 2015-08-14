#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <unistd.h>
#include <cstdlib>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <inttypes.h>
#include <boost/algorithm/string.hpp>
#include <boost/utility/binary.hpp>
//#include <cstdlib>

// For Numerical Recipes
#include <limits.h>
//#include <errno.h>
// END

#include "PowerOpt.h"
#include "Path.h"
#include "Globals.h"
#include "Timer.h"
#include "SetTrie.h"
//#include "Bus.h"

using namespace std;
using namespace boost::algorithm;

// max # of clocks in the design
#define MAX_CLKS 256
// max length of an abbreviated net name
#define MAX_ABBREV 5
// max length of net name
#define MAX_NAME 100
// amount to increase vector size when it needs resizing
#define VECTOR_INC 32

//#define CHECK

// RANDOM NUMBER GENERATION FROM NUMERICAL RECIPES
#define        PI 3.141592654
#define        r_ia     16807
#define        r_im     2147483647
#define        r_am     (1.0/r_im)
#define        r_iq     127773
#define        r_ir     2836
#define        r_ntab   32
#define        r_ndiv   (1+(r_im-1)/r_ntab)
#define        r_eps    1.2e-7
#define        r_rnmx   (1.0-r_eps)
#define        rz_dynamic 0.22
#define        rz_static  0.65
#define        rz_area    1.00
#define        half_clk   0.625
#define        buffer_area  1.44

#define MIN(a,b) (a < b? a:b)
#define MAX(a,b) (a > b? a:b)

#define TICK()  t0 = mytimer()
#define TOCK(t) t += mytimer() - t0
double mytimer (void)
{
  struct timeval tp;
  static long start=0, startu;
  if (!start)
  {
    gettimeofday(&tp, NULL);
    start = tp.tv_sec;
    startu = tp.tv_usec;
    return(0.0);
  }
  gettimeofday(&tp, NULL);
  return ( ((double) (tp.tv_sec - start)) + (tp.tv_usec-startu)/1000000.0 );
}

/* ---------------------------------------------------------------- */
/* uniran1 static globals */
static long idum_RNG = -1;
static long iy_RNG = 0;
static long iv_RNG[r_ntab];

/* ---------------------------------------------------------------- */
/* set_seed - setup seed and temporaries for uniran rng */
static void
set_seed(long seed) {
  int j, k;

  idum_RNG = fabs(seed);
  //fprintf(stderr,"RNG: seed is set to %ld\n",idum_RNG);

  for ( j = r_ntab + 7; j>=0;j--){
    k = (idum_RNG/r_iq);
    idum_RNG = r_ia*(idum_RNG-k*r_iq)-r_ir*k;
    if (idum_RNG<0) idum_RNG += r_im;
    if (j<r_ntab) iv_RNG[j] = idum_RNG;
  }
  iy_RNG = iv_RNG[0];
}

/* ---------------------------------------------------------------- */
/* uniran1 - draw a random deviate uniformly distributed on 0 - 1

   This is a modification of ran1() to be stand alone with its own
   static global variables.
   Call set_seed to setup the seed now.
 */
static double
uniran1()
{
  int  j;
  long k;
  double temp;

  k = (idum_RNG/r_iq);
  idum_RNG = r_ia*(idum_RNG-k*r_iq)-r_ir*k;
  if (idum_RNG<0) idum_RNG += r_im;
  j = iy_RNG/r_ndiv;
  iy_RNG = iv_RNG[j];
  iv_RNG[j] = idum_RNG;
  if ((temp = r_am*iy_RNG) >r_rnmx) return r_rnmx;
  else return temp;
}

/* ---------------------------------------------------------------- */
/* gasdev - draw gaussian random deviate */
static double
gasdev() {
  double fac,r,v1,v2;
  static int iset_RNG = 0;
  static double gset_RNG;

  if  (iset_RNG == 0) {
    do {
      v1=2.0*uniran1()-1.0;
      v2=2.0*uniran1()-1.0;
      r=v1*v1+v2*v2;
    } while (r >= 1.0 || r == 0.0);
    fac=sqrt(-2.0*log(r)/r);
    gset_RNG=v1*fac;
    iset_RNG=1;
    return v2*fac;
  } else {
    iset_RNG=0;
    return gset_RNG;
  }
}


// this is same as printf, but only prints when _DEBUG is defined
inline void dprintf(const char* fmt, ...)
{
#ifdef _DEBUG
  va_list arg;
  va_start(arg, fmt);
  vprintf(fmt, arg);
  va_end(arg);
  fflush(stdout);
#endif
}

namespace POWEROPT {

static void tokenize (string s, char delim, vector<string> & tokens)
{
  stringstream iss;
  iss << s ;
  string token;
  while (getline(iss, token, delim))
  {
    if (token.size())
    {
      tokens.push_back(token);
    }
  }
  iss.clear();
}

static string binary (unsigned int x)
{
  std::string s;
  do
  {
    s.push_back('0' + (x & 1));
  } while (x >>= 1);
  //std::reverse(s.begin(), s.end());
  assert(s.size() <= 16);
  int size = s.size();
  if (s.size() != 16) s.append(16-size, '0');
  return s;
}

void PowerOpt::readStdDevMap()
{
  string sigmafileStr = varMapSigmafile;
  ifstream sigmafile;
  string cell_name;
  vector<double> cell_sigmas(9,0.0); // 9 sigma values for each cell

  sigmafile.open(sigmafileStr.c_str());
  if(!sigmafile) {
    cout << "Cannot open file:" << sigmafileStr << endl;
    exit(0);
  }

  while(!sigmafile.eof()){
    // first, get the cell name
    sigmafile >> cell_name;
    // if a cell name was gotten, read in the sigma values, else EOF
    if(!sigmafile.eof()){
      // there are 9 sigma values for each cell
      // for 1.2V -- 0.4V in 0.1V increments
      for(int j = 0; j < 9; j++){
        sigmafile >> cell_sigmas[j];
      }
      // add an entry to the sigma dictionary
      stdDevMap[cell_name] = cell_sigmas;

      // TEST
      //for(int j = 0; j < 9; j++){
      //  cout << (stdDevMap[cell_name])[j];
      //}
      // END TEST
    }
  }
  sigmafile.close();

  // TEST (has not been tested yet)
  /*
  map<char,int>::iterator it;
  for ( it=stdDevMap.begin() ; it != stdDevMap.end(); it++ ){
      for(int j = 0; j < 9; j++){
        cout << (*it)[j] << endl;
      }
  }
  */
}

void PowerOpt::initVariations(int iterNo)
{
  int width = varMapWidth;
  int seed = varMapSeed;
  string matfileStr = varMapMatfile;

  set_seed((long) (seed+iterNo)); // for the random generator

  int outwidth = width * width;

  double * W_vector = new double [outwidth];
  double * matrix_value = new double [outwidth*outwidth];
  double * result = new double [outwidth];

  int centerX,centerY;
  int gridX,gridY;
  int chipX = getChipWidth();
  int chipY = getChipHeight();

  double thisVAR;
  double inter_die_var;

  // JMS-SHK begin
  // @@ Set the value of inter-die variation for this die
  // inter_die_sigma / within_die_sigma = 0.74 (Cheng / Puneet DAC'09)
  inter_die_var = 0.74 * gasdev();
  // JMS-SHK end

  // Get matrix A from input file
  ifstream matfile;
  matfile.open(matfileStr.c_str());
  if(!matfile) {
    cout << "Cannot open file:" << matfileStr << endl;
    exit(0);
  }

  for(int i = 0; i < outwidth; i++){
    for(int j = 0; j < outwidth; j++){
      matfile >> matrix_value[outwidth * i + j];
    }
  }
  matfile.close();

  // Generate Column Vector of normally distributed gaussian deviates N(0,1)
  for(int i = 0; i < outwidth; i++){
    W_vector[i] = gasdev();
  }

  // Calculate A*W
  for(int j = 0; j < outwidth; j++){
    result[j] = 0.0;
    for(int i = 0; i < outwidth; i++){
      result[j] = result[j] + matrix_value[outwidth * i + j] * W_vector[i];
    }
  }

  // for debug
  ofstream fout;
  char filename[250];
  sprintf(filename, "variation-%d.rpt", iterNo ) ;
  if (varSave) fout.open(filename);
  // Determine the grid for each gate and assign a random variation to the gate
  Gate    *g;
  for (int i = 0; i < getGateNum(); i ++){
    g = m_gates[i];

    centerX = g->getCenterX();
    centerY = g->getCenterY();

    gridX = (int) floor( ( ((double) centerX) / ((double) chipX) ) * ((double) width) );
    gridY = (int) floor( ( ((double) centerY) / ((double) chipY) ) * ((double) width) );

    // JMS-SHK begin
    // intra-die variations
    thisVAR = result[width * gridX + gridY];

    // inter-die variations
    thisVAR += inter_die_var;
    // JMS-SHK end

    g->setVariation(thisVAR);
    if (varSave) fout << g->getName() << "\t" << g->getCellName() << "\t" << gridX << "\t" << gridY << "\t" << g->getVariation() << endl;
  }
  if (varSave) fout.close();
  // delete dynamically allocated memory
  delete[] W_vector;
  delete[] matrix_value;
  delete[] result;
}

void PowerOpt::wait (int seconds)
{
    clock_t endwait;
    endwait = clock () + seconds * CLOCKS_PER_SEC ;
    while (clock() < endwait) {}
}

void PowerOpt::readEnvFile(string envFileStr)
{
    ifstream file;
    file.open(envFileStr.c_str());
    if(!file) {
        cout << "Cannot open file:" << envFileStr << endl;
        exit(0);
    }

    //default environments
    homeDir = "";
    ptServerName = "";
    ptClientTcl = "ptclient.tcl";
    ptServerTcl = "ptserver.tcl";
    ptRestoreTcl = "ptrestore.tcl";
    soceConfFile = "soce.conf";
    soceTclFile = "soce.tcl";
    leakListTcl = "leakageList.tcl";
    leakListFile = "leakageList.rpt";
    libListTcl = "libList.tcl";
    dbLibPath = "";
    libLibPath = "";
    soceCapTableWorst = "";
    soceCapTableBest = "";
    ptPort = 5000;

    string line;
    string tmp_str;

    while(std::getline(file, line))
    {

        if (line.find("-home ") != string::npos)
            homeDir = getTokenS(line, "-home ");
        if (line.find("-srv ") != string::npos)
            ptServerName = getTokenS(line, "-srv ");
        if (line.find("-port ") != string::npos)
            ptPort = getTokenI(line, "-port ");
        if (line.find("-dbpath ") != string::npos)
            dbLibPath = getTokenS(line, "-dbpath ");
        if (line.find("-libpath ") != string::npos)
            libLibPath = getTokenS(line, "-libpath ");
        if (line.find("-db ") != string::npos)
            dbLibs.push_back(getTokenS(line, "-db "));
        if (line.find("-lib ") != string::npos)
            libLibs.push_back(getTokenS(line, "-lib "));
        if (line.find("-mmmcEnv ") != string::npos)
            mmmcFile.push_back(getTokenS(line, "-mmmcEnv "));
        if (line.find("-suffix ") != string::npos) {
            tmp_str = getTokenS(line, "-suffix ");
            if (tmp_str.find("[") != string::npos)
                tmp_str.erase(tmp_str.find_first_of("["),1);
            if (tmp_str.find("]") != string::npos)
                tmp_str.erase(tmp_str.find_last_of("]"),1);
            libSuffix.push_back(tmp_str);
        }

        if (line.find("-lef ") != string::npos)
            lefFiles.push_back(getTokenS(line, "-lef "));
        if (line.find("-capw ") != string::npos)
            soceCapTableWorst = getTokenS(line, "-capw ");
        if (line.find("-capb ") != string::npos)
            soceCapTableBest = getTokenS(line, "-capb ");
        if (line.find("-leq ") != string::npos)
            libListFile = getTokenS(line, "-leq ");
        if (line.find("-regCells") != string::npos)
            regCellsFile = getTokenS(line, "-regCells");
        if (line.find("-moduleNamesFile") != string::npos)
            moduleNamesFile = getTokenS(line, "-moduleNamesFile");
        if (line.find("-worstSlacksFile") != string::npos)
            worstSlacksFile = getTokenS(line, "-worstSlacksFile");
    }
    file.close();

    char    hostname[128];
    gethostname(hostname, sizeof hostname);
    if ( ptServerName == "" ) {
        ptServerName = hostname;
        cout << "[PowerOpt] hostname: " << hostname << endl;
    }

    if ( homeDir == "" || dbLibPath == "" || ptServerName == "" || libLibPath == "" ) {
        cout << "Error: One of following information is not specified!!"<< endl;
        cout << "-home [home directory] -srv [server name] -dbpath [path for .db] -libpath [path for .lib]"  << endl;
        exit(0);
    }
    if (dbLibs.size() == 0 || lefFiles.size() == 0 || libLibs.size() == 0) {
        cout << "Error: .db or .lef or .lib files are not specified!!" << endl;
        exit(0);
    }
    ptServerCmdTcl = homeDir + "/PT/ptserver_cmds.tcl";
    gtServerCmdTcl = homeDir + "/PT/gtserver_cmds.tcl";
    soceConfScript = homeDir + "/SOCE/default.conf";
}

void PowerOpt::readCmdFile(string cmdFileStr)
{
    ifstream file;
    file.open(cmdFileStr.c_str());
    if(!file) {
        cout << "Cannot open file :" << cmdFileStr << endl;
        exit(0);
    }

    string line, temp;

    //defalt values
    exeOp = 1;
    swapOp = 1;
    oaGenFlag = 1;
    sensFunc = 1;
    sensFuncTiming = 1;
    stopCond = 0;
    holdCheck = 1;
    maxTrCheck = 1;
    guardBand = 0.0;
    //oaGenFlag = 1;
    updateOp = 1;
    swapstep = 10000;
    circuitName = "";
    defFile = "";
    spefFile = "";
    verilogFile = "";
    clockName = "clk";
    reportFile = "result.rpt";
    verilogOutFile = "";
    defOutFile = "";
    spefOutFile = "";
    ptOption = "";
    vcdPath = "";
    ptReport = "";
    saifPath = "";
    initSwapFile = "";
    ptLogSave = false;
    varSave = false;
    leakPT = false;     // true: leakage from PT
    exeSOCEFlag = false;
    chkWNSFlag = false;
    totPWRFlag = false;
    mmmcOn = false;
    ptpxOff = false;
    useGT = false;
    useGTMT = false;
    noSensUp = false;
    noDEF = false;
    noSPEF = false;
    optEffort = "medium";
    testValue1 = 1;
    testValue2 = 1;
    parseCycle = -1;
    vcdStartCycle = 0;
    vcdStopCycle = -1;
    ignoreClockGates = 0;
    is_preprocess = false;
    is_postprocess = false;
    is_dump_uts = false;
    is_dump_units = false;
    is_dead_end_check = false;
    ignoreMultiplier = 0;
    vStart = 120;
    vEnd = 40;
    vStep = 1;
    targErrorRate = 0;
    // parameters for variation map
    varMapWidth = 10;
    varMapSeed = 1;
    //varMapStdDev = 0;
    varMapMatfile = "";
    varIteration = 1;
    varMapSigmafile = "";
    path_time = 0.0;
    num_sim_cycles = 0;

    while(std::getline(file, line))
    {
        if (line.find_first_of("#") == 0) continue;
        if (line.find("-mode ") != string::npos)
            exeOp = getTokenI(line,"-mode ");
        if (line.find("-op ") != string::npos)
            swapOp = getTokenI(line,"-op ");
        if (line.find("-c ") != string::npos) {
            circuitName = getTokenS(line,"-c ");
            libraryName = getTokenS(line,"-c ");
            layoutName = getTokenS(line,"-c ");
        }
        if (line.find("-dut") != string::npos) {
            dutName = getTokenS(line, "-dut");
        }
        if (line.find("-period") != string::npos) {
            clockPeriod = getTokenI(line, "-period");
        }
        if (line.find("-subsets") != string::npos) {
            swapOp = getTokenI(line, "-subsets");
        }

        if (line.find("-vmw ") != string::npos)
            varMapWidth = getTokenI(line, "-vmw ");
        if (line.find("-vms ") != string::npos)
            varMapSeed = getTokenI(line, "-vms ");
        //if (line.find("-vmsd ") != string::npos)
        //    varMapStdDev = getTokenF(line, "-vmsd ");
        if (line.find("-vmmfile ") != string::npos)
            varMapMatfile = getTokenS(line, "-vmmfile ");
        if (line.find("-vmsfile ") != string::npos)
            varMapSigmafile = getTokenS(line, "-vmsfile ");
        if (line.find("-vmitr ") != string::npos)
            varIteration = getTokenI(line, "-vmitr ");
        if (line.find("-untDumpFile") != string::npos)
            untDumpFile = getTokenS(line,"-untDumpFile");
        if (line.find("-utDumpFile") != string::npos)
            utDumpFile = getTokenS(line,"-utDumpFile");
        if (line.find("-clusterNamesFile") != string::npos)
            clusterNamesFile = getTokenS(line, "-clusterNamesFile");
        if (line.find("-simInitFile ") != string::npos)
            simInitFile = getTokenS(line, "-simInitFile ");
        if (line.find("-dmemInitFile ") != string::npos)
            dmemInitFile = getTokenS(line, "-dmemInitFile ");
        if (line.find("-dbgSelectGates ") != string::npos)
            dbgSelectGatesFile = getTokenS(line, "-dbgSelectGates ");
        if (line.find("-pmemFile ") != string::npos)
            pmem_file_name = getTokenS(line, "-pmemFile ");
        if (line.find("-def ") != string::npos)
            defFile = getTokenS(line, "-def ");
        if (line.find("-spef ") != string::npos)
            spefFile = getTokenS(line, "-spef ");
        if (line.find("-v ") != string::npos)
            verilogFile = getTokenS(line, "-v ");
        if (line.find("-sdc ") != string::npos)
            sdcFile = getTokenS(line, "-sdc ");
        if (line.find("-ck ") != string::npos)
            clockName = getTokenC(line,"-ck ");
        if (line.find("-n ") != string::npos)
            swapstep = getTokenI(line,"-n ");
        if (line.find("-sf ") != string::npos)
            sensFunc = getTokenI(line,"-sf ");
        if (line.find("-sft ") != string::npos)
            sensFuncTiming = getTokenI(line,"-sft ");
        if (line.find("-st ") != string::npos)
            stopCond = getTokenI(line,"-st ");
        if (line.find("-opt_effort ") != string::npos)
            optEffort = getTokenS(line,"-opt_effort ");
        if (line.find("-hold ") != string::npos)
            holdCheck = getTokenI(line,"-hold ");
        if (line.find("-maxTr ") != string::npos)
            maxTrCheck = getTokenI(line,"-maxTr ");
        if (line.find("-g ") != string::npos)
            guardBand = getTokenF(line,"-g ");
        if (line.find("-oa ") != string::npos)
            oaGenFlag = getTokenI(line,"-oa ");
        if (line.find("-rpt ") != string::npos)
            reportFile = getTokenS(line,"-rpt ");
        if (line.find("-vout ") != string::npos)
            verilogOutFile = getTokenS(line,"-vout ");
        if (line.find("-defout ") != string::npos)
            defOutFile = getTokenS(line,"-defout ");
        if (line.find("-spefout ") != string::npos)
            spefOutFile = getTokenS(line,"-spefout ");
        if (line.find("-ptOpt ") != string::npos)
            ptOption = getTokenS(line,"-ptOpt ");
        if (line.find("-ptLog") != string::npos)
            ptLogSave = true;
        if (line.find("-varLog") != string::npos)
            varSave = true;
        if (line.find("-leakPT") != string::npos)
            leakPT = true;
        if (line.find("-mmmc") != string::npos)
            mmmcOn = true;
        if (line.find("-noPTPX") != string::npos)
            ptpxOff = true;
        if (line.find("-useGT") != string::npos)
            useGT = true;
        if (line.find("-useGTMT") != string::npos)
            useGTMT = true;
        if (line.find("-noSensUp") != string::npos)
            noSensUp = true;
        if (line.find("-noSPEF") != string::npos)
            noSPEF = true;
        if (line.find("-noDEF") != string::npos)
            noDEF = true;
        if (line.find("-eco") != string::npos)
            exeSOCEFlag = true;
        if (line.find("-chkWNS") != string::npos)
            chkWNSFlag = true;
        if (line.find("-totPWR") != string::npos)
            totPWRFlag = true;
        if (line.find("-dont_touch_inst ") != string::npos)
            dontTouchInst.push_back(getTokenS(line, "-dont_touch_inst "));
        if (line.find("-dont_touch_cell ") != string::npos)
            dontTouchCell.push_back(getTokenS(line, "-dont_touch_cell "));
        if (line.find("-vcdpath ") != string::npos)
            vcdPath = getTokenS(line, "-vcdpath ");
        if (line.find("-ptReport ") != string::npos)
            ptReport = getTokenS(line, "-ptReport ");
        if (line.find("-initSwap ") != string::npos)
            initSwapFile = getTokenS(line, "-initSwap ");
        if (line.find("-vcd ") != string::npos)
            vcdFile.push_back(getTokenS(line, "-vcd "));
        if (line.find("-saifpath ") != string::npos)
            saifPath = getTokenS(line, "-saifpath ");
        if (line.find("-saif ") != string::npos)
            saifFile.push_back(getTokenS(line, "-saif "));
        if (line.find("-vcdCycle ") != string::npos)
            parseCycle = getTokenI(line,"-vcdCycle ");
        if (line.find("-vcdStartCycle ") != string::npos)
            vcdStartCycle = getTokenULL(line,"-vcdStartCycle");
        if (line.find("-vcdStopCycle ") != string::npos)
            vcdStopCycle = getTokenULL(line,"-vcdStopCycle");
        if (line.find("-vcdCheckStartCycle ") != string::npos)
            vcdCheckStartCycle = getTokenULL(line,"-vcdCheckStartCycle");
        if (line.find("-IMOI ") != string::npos)
            internal_module_of_interest = getTokenS(line,"-IMOI");
        if (line.find("-preprocess") != string::npos)
            is_preprocess = true;
        if (line.find("-postprocess") != string::npos)
            is_postprocess = true;
        if (line.find("-dump_uts") != string::npos)
            is_dump_uts = true;
        if (line.find("-dump_units") != string::npos)
          if (exeOp != 17){ cout << "NOT DUMPING UNITS SINCE EXEC_OP != 17" << endl; exit(0); }
          else  is_dump_units = true;
        if (line.find("-deadend") != string::npos)
            is_dead_end_check = true;
        if (line.find("-ignoreClockGates") != string::npos)
            ignoreClockGates = getTokenI(line,"-ignoreClockGates");
        if (line.find("-ignoreMultiplier") != string::npos)
            ignoreMultiplier = getTokenI(line,"-ignoreMultiplier");
        if (line.find("-vStart ") != string::npos)
            vStart = getTokenI(line,"-vStart ");
        if (line.find("-vEnd ") != string::npos)
            vEnd = getTokenI(line,"-vEnd ");
        if (line.find("-vStep ") != string::npos)
            vStep = getTokenI(line,"-vStep ");
        if (line.find("-targER ") != string::npos)
            targErrorRate = getTokenF(line,"-targER ");
        if (line.find("-test1 ") != string::npos)
            testValue1 = getTokenI(line,"-test1 ");
        if (line.find("-test2 ") != string::npos)
            testValue2 = getTokenI(line,"-test1 ");
        if (line.find("-num_sim_cycles ") != string::npos)
            num_sim_cycles = getTokenI(line,"-num_sim_cycles ");
    }

    if (swapstep == 0) swapstep = 10000;
    if (!mmmcOn) mmmcFile.clear();

    if (circuitName == "") {
        cout << "Error: design name (-c design) should be specified!!" << endl;
        exit(0);
    }
    if (defFile == "") defFile = circuitName + ".def";
    if (spefFile == "") spefFile = circuitName + ".spef";
    if (verilogFile == "") verilogFile = circuitName + ".v";
    if (sdcFile == "") sdcFile = circuitName + ".sdc";
    if (verilogOutFile == "") verilogOutFile = circuitName + "_eco.v";
    if (defOutFile == "") defOutFile = circuitName + "_eco.def";
    if (spefOutFile == "") spefOutFile = circuitName + "_eco.spef";
    if (stopCond == 0 && optEffort == "medium") stopCond = 50;
    else if (stopCond == 0 && optEffort == "high") stopCond = 100;
    else if (stopCond == 0 && optEffort == "low") stopCond = 25;
    if (useGT) ptpxOff = true;
    file.close();

  FILE * VCDfile;
  string VCDfilename = vcdPath + "/" + vcdFile[0];
  VCDfile = fopen(VCDfilename.c_str(), "r");
  if(VCDfile == NULL){
    printf("ERROR, could not open VCD file: %s\n",VCDfilename.c_str());
    exit(1);
  }
  fclose(VCDfile);
}

void PowerOpt::openFiles()
{
  DIR* dir = opendir("./PowerOpt");
  if (dir) closedir(dir);
  else system("mkdir PowerOpt");
  if (exeOp == 15 || exeOp == 16 || exeOp == 17)
  {
    toggle_info_file.open             ( "PowerOpt/toggle_info");
    toggled_nets_file.open            ( "PowerOpt/toggled_nets");
    unique_not_toggle_gate_sets.open  ( "PowerOpt/unique_not_toggle_gate_sets");
    unique_toggle_gate_sets.open      ( "PowerOpt/unique_toggle_gate_sets");
    units_file.open                   ( "PowerOpt/UNITS_INFO");
    slack_profile_file.open           ( "PowerOpt/slack_profile");
    net_gate_maps.open                ( "PowerOpt/net_gate_maps");
    net_pin_maps.open                 ( "PowerOpt/net_pin_maps" );
    toggle_counts_file.open           ( "PowerOpt/toggle_counts");
    histogram_toggle_counts_file.open ( "PowerOpt/histogram_toggle_counts_file");
    toggle_profile_file.open          ( "PowerOpt/toggle_profile_file");
    missed_nets.open                  ( "PowerOpt/Missed_nets", fstream::app);
  }
  debug_file.open("PowerOpt/debug_file");
  pmem_request_file.open("PowerOpt/pmem_request_file");
  dmem_request_file.open("PowerOpt/dmem_request_file");
  fanins_file.open("PowerOpt/fanins_file");
  processor_state_profile_file.open("PowerOpt/processor_state_profile");
  dmem_contents_file.open("PowerOpt/dmem_contents_file");
}

void PowerOpt::closeFiles()
{
  if (exeOp == 15 || exeOp == 16 || exeOp == 17)
  {
    toggle_info_file.close();
    toggled_nets_file.close();
    unique_not_toggle_gate_sets.close();
    unique_toggle_gate_sets.close();
    units_file.close();
    slack_profile_file.close();
    net_gate_maps.close();
    net_pin_maps.close();
    toggle_counts_file.close();
    histogram_toggle_counts_file.close();
    toggle_profile_file.close();
  }
  debug_file.close();
  pmem_request_file.close();
  dmem_request_file.close();
  fanins_file.close();
  processor_state_profile_file.close();
  dmem_contents_file.close();
}


void PowerOpt::makeOA()
{
    ofstream fout;
    fout.open(":genOADB");
    for (int i = 0; i < lefFiles.size(); i++) {
        fout << "lef2oa -lib " << libraryName << " -libPath ./";
        fout << circuitName << " -lef " << lefFiles[i];
        fout << " -view " << layoutName << " -overwrite" << endl;
        fout << "\\mv lef2oa.log " << libraryName << endl;
    }
    if (noDEF) {
        fout << "verilog2oa -lib " << libraryName << " -libPath ./";
        fout << circuitName << " -verilog " << verilogFile << " -overwrite ";
        fout << " -view " << layoutName << endl;
        fout << "\\mv verilog2oa.log " << libraryName << endl;
    } else {
        fout << "def2oa -lib " << libraryName << " -libPath ./";
        fout << circuitName << " -def " << defFile << " -overwrite -cell ";
        fout << circuitName << " -view " << layoutName;
        fout << " -masterViews " << layoutName << endl;
        fout << "\\mv def2oa.log " << libraryName << endl;
    }
    char Commands[250];
    sprintf(Commands, "\\touch %s", circuitName.c_str());
    system(Commands);
    sprintf(Commands, "\\rm -r %s", circuitName.c_str());
    system(Commands);
    sprintf(Commands, "\\mkdir %s", circuitName.c_str());
    system(Commands);
    sprintf(Commands, "source ./:genOADB");
    system(Commands);
    if (!ptLogSave) {
        sprintf(Commands, "rm ./:genOADB");
        system(Commands);
    }
    fout.close();
}

void PowerOpt::exePTServer(bool isPost)
{
    char socketFile[250];
    char Commands[250];
    bool flag;

    while (true) {
        ifstream file;
        string line;
        flag = false;
        if(ptPort > 9999) {
          cout << "Error: cannot open PT socket on port " << ptPort << endl;
          ptPort = 5000;
          cout << "Trying port " << ptPort << " instead." << endl;
          //exit(0);
        }
        if (useGT) exeGTServerOne(isPost);
        else exePTServerOne(isPost);
        bool flag2 = true;
        sprintf(socketFile, "socket-%d.tmp", ptPort );
        int tmp_cnt = 0;
        while (flag2) {
            ifstream fp;
            wait (5);
            if (tmp_cnt++ > 50) break;
            fp.open(socketFile);
            if (file) flag2 = false;
            fp.close();
        }
        file.open(socketFile);
        while(std::getline(file, line)) {
            if (line.find("Error") != string::npos) flag = true;
        }
        file.close();
        if (!flag) break;
        ptPort = ptPort + 10;
    }
    cout << "[PowerOpt] PT socket is opened at " << ptServerName << ":" << ptPort << endl;
    sprintf(Commands, "\\rm socket*.tmp" );
    system(Commands);
}

void PowerOpt::exePTServerOne(bool isPost)
{
    ofstream fout;
    string saif_filename;
    // make ptclient tcl
    fout.open(ptClientTcl.c_str());
    fout << "proc GetData {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  if {![eof $chan]} {" << endl;
    fout << "    set data [gets $chan]" << endl;
    fout << "    flush stdout" << endl;
    fout << "    return $data" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc SendData {data} {" << endl;
    fout << "  global chan" << endl;
    fout << "  puts $chan $data" << endl;
    fout << "  flush $chan" << endl;
    fout << "}" << endl;
    fout << "proc DoOnePtCommand {cmd} {" << endl;
    fout << "  SendData $cmd" << endl;
    //fout << "  echo \" Command is $cmd \" " << endl;
    fout << "  return [GetData]" << endl;
    fout << "}" << endl;
    fout << "proc InitClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  set server " << ptServerName << endl;
    fout << "  set chan [socket $server " << ptPort <<  "]" << endl;
    fout << "  GetData" << endl;
    fout << "  GetData" << endl;
    fout << "}" << endl;
    fout << "proc CloseClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  close $chan" << endl;
    fout << "}" << endl;
    fout.close();

    // make ptserver tcl
    fout.open(ptServerTcl.c_str());

    fout << "source " << ptServerCmdTcl << endl;
    fout << "set svcPort " << ptPort << endl;
    fout << "proc doService {sock msg} {" << endl;
    //fout << "    puts \"got command $msg on ptserver\"" << endl;
    fout << "    puts $sock [eval \"$msg\"]" << endl;
    fout << "    flush $sock" << endl;
    fout << "}" << endl;
    fout << "proc  svcHandler {sock} {" << endl;
    fout << "  set l [gets $sock]" << endl;
    fout << "  if {[eof $sock]} {" << endl;
    fout << "     close $sock " << endl;
    fout << "  } else {" << endl;
    fout << "    doService $sock $l" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc accept {sock addr port} {" << endl;
    fout << "  fileevent $sock readable [list svcHandler $sock]" << endl;
    fout << "  fconfigure $sock -buffering line -blocking 0" << endl;
    fout << "  puts $sock \"$addr:$port, You are connected to the pt server.\"" << endl;
    fout << "  puts $sock \"It is now [exec date]\"" << endl;
    fout << "  puts \"Accepted connection from $addr at [exec date]\"" << endl;
    fout << "}" << endl;
    fout << "set power_enable_analysis TRUE" << endl;
    fout << "set power_analysis_mode averaged" << endl;
    fout << "set design  \"" << circuitName <<"\"" << endl;
    fout << "set timing_remove_clock_reconvergence_pessimism true" << endl;
    fout << "set search_path \". " << dbLibPath << "\"" << endl;
    fout << "set link_path [list * ";
    for (int i = 0; i < dbLibs.size(); i++) fout << dbLibs[i] << " ";
    fout << "]" << endl;
    if (isPost) fout << "read_verilog " << verilogOutFile << endl;
    else fout << "read_verilog " << verilogFile << endl;
    fout << "current_design $design" << endl;
    fout << "link_design $design" << endl;
    fout << "read_sdc " << sdcFile << endl;
    fout << "set_propagated_clock " << clockName << endl;
    if (!noSPEF && isPost && exeSOCEFlag) fout << "read_parasitics " << spefOutFile << endl;
    else if (!noSPEF) fout << "read_parasitics " << spefFile << endl;
    fout << "set report_default_significant_digits 6" << endl;
    fout << "set timing_save_pin_arrival_and_slack true" << endl;
    fout << "set timing_report_always_use_valid_start_end_points false" << endl;
    fout << "check_timing" << endl;
    if (saifFile.size() == 1 ) {
        saif_filename = saifPath + "/" + saifFile[0];
        fout << "read_saif -strip_path $design " << saif_filename << endl;
    } else if (saifFile.size() != 0 ) {
        fout << "merge_saif -strip_path $design -input_list {";
        double weight = 100.0 / saifFile.size();
        for (int i = 0; i < saifFile.size() -1; i++) {
            saif_filename = saifPath + "/" + saifFile[i];
            fout << " -input " << saif_filename << " -weight " << weight;
        }
        weight = 100 - weight * (saifFile.size() - 1);
        saif_filename = saifPath + "/" + saifFile[saifFile.size()-1];
        fout << " -input " << saif_filename << " -weight " << weight << " }" << endl;
    }
    if (!ptpxOff) fout << "update_power" << endl;
    fout << "set sh_continue_on_error true" << endl;
    fout << "redirect socket-$svcPort.tmp { socket -server accept $svcPort }" << endl;
    fout << "set inFp [open socket-$svcPort.tmp]" << endl;
    fout << "while { [gets $inFp line] >=0 } {" << endl;
    fout << "  if { [regexp {^Error} $line] } {" << endl;
    fout << "    [exit]" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "vwait events" << endl;
    fout.close();


    //ofstream fout;
    fout.open("launch.cmd");
    if (ptLogSave && !isPost)
        fout << "pt_shell " << ptOption << " -f " << ptServerTcl << " |& tee PowerOpt/pt.log" << endl;
    else
        fout << "pt_shell " << ptOption << " -f " << ptServerTcl << endl;
    fout.close();

    /*
    fout.open(":exePT");
    fout << "#!/bin/csh -f" << endl;
    if (ptLogSave && !isPost)
        fout << "xterm -T PowerOptPT -iconic -e csh -f launch.cmd |& tee pt.out" << endl;
    else
        fout << "xterm -T PowerOptPT -iconic -e csh -f launch.cmd " << endl;
    fout.close();
    */

    // Execute PrimeTime
    char Commands[250];
    /*
    if (ptLogSave && !isPost)
        sprintf(Commands, "`pt_shell %s -f %s > pt.log` &", ptOption.c_str(), ptServerTcl.c_str());
    else if (!isPost) sprintf(Commands, "`pt_shell %s -f %s | grep \"Timing updates:\" > pt.log` &", ptOption.c_str(), ptServerTcl.c_str());
    else sprintf(Commands, "`pt_shell %s -f %s` &", ptOption.c_str(), ptServerTcl.c_str());
    */
    sprintf(Commands, "chmod +x launch.cmd");
    system(Commands);
    sprintf(Commands, "xterm -T PowerOptPT -iconic -e csh -f launch.cmd &");
    system(Commands);
}

void PowerOpt::exeGTServerOne(bool isPost)
{
    ofstream fout;
    // make ptclient tcl
    fout.open(ptClientTcl.c_str());
    fout << "proc GetData {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  if {![eof $chan]} {" << endl;
    fout << "    set data [gets $chan]" << endl;
    fout << "    flush stdout" << endl;
    fout << "    return $data" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc SendData {data} {" << endl;
    fout << "  global chan" << endl;
    fout << "  puts $chan $data" << endl;
    fout << "  flush $chan" << endl;
    fout << "}" << endl;
    fout << "proc DoOnePtCommand {cmd} {" << endl;
    fout << "  SendData $cmd" << endl;
    fout << "  return [GetData]" << endl;
    fout << "}" << endl;
    fout << "proc InitClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  set server " << ptServerName << endl;
    fout << "  set chan [socket $server " << ptPort <<  "]" << endl;
    fout << "  GetData" << endl;
    fout << "  GetData" << endl;
    fout << "}" << endl;
    fout << "proc CloseClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  close $chan" << endl;
    fout << "}" << endl;
    fout.close();

    // make ptserver tcl
    fout.open(ptServerTcl.c_str());

    fout << "source " << gtServerCmdTcl << endl;
    fout << "set svcPort " << ptPort << endl;
    fout << "proc doService {sock msg} {" << endl;
    //fout << "    puts \"got command $msg on ptserver\"" << endl;
    fout << "    puts $sock [eval \"$msg\"]" << endl;
    fout << "    flush $sock" << endl;
    fout << "}" << endl;
    fout << "proc  svcHandler {sock} {" << endl;
    fout << "  set l [gets $sock]" << endl;
    fout << "  if {[eof $sock]} {" << endl;
    fout << "     close $sock " << endl;
    fout << "  } else {" << endl;
    fout << "    doService $sock $l" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc accept {sock addr port} {" << endl;
    fout << "  fileevent $sock readable [list svcHandler $sock]" << endl;
    fout << "  fconfigure $sock -buffering line -blocking 0" << endl;
    fout << "  puts $sock \"$addr:$port, You are connected to the pt server.\"" << endl;
    fout << "  puts $sock \"It is now [exec date]\"" << endl;
    fout << "  puts \"Accepted connection from $addr at [exec date]\"" << endl;
    fout << "}" << endl;
    fout << "set design  \"" << circuitName <<"\"" << endl;
    if (useGTMT) fout << "settings TTimer/NumThreads 4" << endl;
    fout << "settings TTimer/SavePinArrivalAndSlack true" << endl;
    fout << "settings TTimer/ProgramMode high_performance" << endl;
    for (int i = 0; i < libLibs.size(); i++)
    fout << "read_lib " << libLibPath << "/" << libLibs[i] << endl;
    if (isPost) fout << "read_verilog " << verilogOutFile << endl;
    else fout << "read_verilog " << verilogFile << endl;
    fout << "flatten" << endl;
    fout << "source " << sdcFile << endl;
    if (!noSPEF && isPost && exeSOCEFlag) fout << "read_parasitics " << spefOutFile << endl;
    else if (!noSPEF) fout << "read_parasitics " << spefFile << endl;
    fout << "update_timing" << endl;
    fout << "redirect socket-$svcPort.tmp { socket -server accept $svcPort }" << endl;
    fout << "set inFp [open socket-$svcPort.tmp]" << endl;
    fout << "while { [gets $inFp line] >=0 } {" << endl;
    fout << "  if { [regexp {^Error} $line] } {" << endl;
    fout << "    [exit]" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "vwait events" << endl;
    fout.close();

    //ofstream fout;
    fout.open("launch.cmd");
    if (ptLogSave && !isPost)
        fout << "goldtime " << ptOption << " -f " << ptServerTcl << " |& tee gt.log" << endl;
    else
        fout << "goldtime " << ptOption << " -f " << ptServerTcl << endl;
    fout.close();

    // Execute PrimeTime
    char Commands[250];

    sprintf(Commands, "chmod +x launch.cmd");
    system(Commands);
    sprintf(Commands, "xterm -T PowerOptGT -iconic -e csh -f launch.cmd &");
    system(Commands);
}

void PowerOpt::restorePTLib(int volt, designTiming *T)
{
    ofstream fout;
    string saif_filename;
    // make ptclient tcl
    fout.open(ptRestoreTcl.c_str());
    fout << "remove_design -all" << endl;
    fout << "remove_lib -all" << endl;
    fout << "set design  \"" << circuitName <<"\"" << endl;
    fout << "set power_enable_analysis TRUE" << endl;
    fout << "set power_analysis_mode averaged" << endl;
    fout << "set timing_remove_clock_reconvergence_pessimism true" << endl;
    fout << "set search_path \". " << dbLibPath << "\"" << endl;
    fout << "global link_path" << endl;
    fout << "set link_path [list * ";
    // new db printing that considers all libraries
    for (int i = 0; i < dbLibs.size(); i++){
      // extract the portion up to the last underscore
      string db_basename;
      size_t locale;
      locale = (dbLibs[i]).rfind("_");
      // NOTE: THIS ASSUMES THAT THE NAMING CONVENTION IS db_basename_volt.db
      //if (locale != string::npos){
      db_basename = (dbLibs[i]).substr(0,locale+1);
      //} else{}
      // print the part of the name up to and including the underscore
      fout << db_basename;
      // print the volt with a width of 3, padded with 0
      fout.fill('0');
      fout.width(3);
      fout << volt;
      // this last part is in a separate fout statement so width operation is only for printing volt
      fout << ".db ";
    }
    fout << "]" << endl;

    fout << "read_verilog " << verilogFile << endl;
    fout << "current_design $design" << endl;
    fout << "link_design $design" << endl;
    fout << "read_sdc " << sdcFile << endl;
    fout << "set_propagated_clock " << clockName << endl;
    fout << "read_parasitics ./$design.spef" << endl;
    if (!noSPEF) fout << "read_parasitics " << spefFile << endl;
    fout << "set report_default_significant_digits 6" << endl;
    fout << "set timing_save_pin_arrival_and_slack true" << endl;
    fout << "set timing_report_always_use_valid_start_end_points false" << endl;
    fout << "check_timing" << endl;
    if (saifFile.size() == 1 ) {
        saif_filename = saifPath + "/" + saifFile[0];
        fout << "read_saif -strip_path $design " << saif_filename << endl;
    } else if (saifFile.size() != 0 ) {
        fout << "merge_saif -strip_path $design -input_list {";
        double weight = 100.0 / saifFile.size();
        for (int i = 0; i < saifFile.size() -1; i++) {
            saif_filename = saifPath + "/" + saifFile[i];
            fout << " -input " << saif_filename << " -weight " << weight;
        }
        weight = 100 - weight * (saifFile.size() - 1);
        saif_filename = saifPath + "/" + saifFile[saifFile.size()-1];
        fout << " -input " << saif_filename << " -weight " << weight << " }" << endl;
    }
    if (!ptpxOff) fout << "update_power" << endl;
    fout.close();
    T->readTcl(ptRestoreTcl);
    //char Commands[250];
    //sprintf(Commands, "rm ./%s", ptRestoreTcl.c_str());
    //system(Commands);
}

void PowerOpt::exeMMMC()
{
    for (int i = 0; i < mmmcFile.size(); i++) {
        if (useGT) exeGTServerMMMC(i);
        else exePTServerMMMC(i);
    }
}

void PowerOpt::exePTServerMMMC(int index)
{
    ifstream file;
    file.open(mmmcFile[index].c_str());
    if(!file) {
        cout << "Cannot open file:" << mmmcFile[index] << endl;
        exit(0);
    }

    string line;
    string dbLibPath_mmmc = dbLibPath;
    int    ptPort_mmmc =  ptPort + index + 1;
    vector<string> dbLibs_mmmc;
    string index_str;
    stringstream out_str;
    string sdcFile_mmmc;
    out_str << index;
    index_str = out_str.str();      // integer to string
    string name_mmmc = "mmmc" + index_str;

    while(std::getline(file, line))
    {
        if (line.find("-name ") != string::npos)
            name_mmmc = getTokenS(line, "-name ");
        if (line.find("-port ") != string::npos)
            ptPort_mmmc = getTokenI(line, "-port ");
        if (line.find("-dbpath ") != string::npos)
            dbLibPath_mmmc = getTokenS(line, "-dbpath ");
        if (line.find("-db ") != string::npos)
            dbLibs_mmmc.push_back(getTokenS(line, "-db "));
        if (line.find("-sdc ") != string::npos)
            sdcFile_mmmc = getTokenS(line, "-sdc ");
    }
    file.close();

    string ptClientTcl_mmmc = ptClientTcl + "-" + name_mmmc;
    string ptServerTcl_mmmc = ptServerTcl + "-" + name_mmmc;

    ofstream fout;
    // make ptclient tcl
    fout.open(ptClientTcl_mmmc.c_str());

    fout << "proc GetData {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  if {![eof $chan]} {" << endl;
    fout << "    set data [gets $chan]" << endl;
    fout << "    flush stdout" << endl;
    fout << "    return $data" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc SendData {data} {" << endl;
    fout << "  global chan" << endl;
    fout << "  puts $chan $data" << endl;
    fout << "  flush $chan" << endl;
    fout << "}" << endl;
    fout << "proc DoOnePtCommand {cmd} {" << endl;
    fout << "  SendData $cmd" << endl;
    fout << "  return [GetData]" << endl;
    fout << "}" << endl;
    fout << "proc InitClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  set server " << ptServerName << endl;
    fout << "  set chan [socket $server " << ptPort_mmmc <<  "]" << endl;
    fout << "  GetData" << endl;
    fout << "  GetData" << endl;
    fout << "}" << endl;
    fout << "proc CloseClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  close $chan" << endl;
    fout << "}" << endl;
    fout.close();

    // make ptserver tcl
    fout.open(ptServerTcl_mmmc.c_str());

    fout << "source " << ptServerCmdTcl << endl;
    fout << "set svcPort " << ptPort_mmmc << endl;
    fout << "proc doService {sock msg} {" << endl;
    //fout << "    puts \"got command $msg on ptserver\"" << endl;
    fout << "    puts $sock [eval \"$msg\"]" << endl;
    fout << "    flush $sock" << endl;
    fout << "}" << endl;
    fout << "proc  svcHandler {sock} {" << endl;
    fout << "  set l [gets $sock]" << endl;
    fout << "  if {[eof $sock]} {" << endl;
    fout << "     close $sock " << endl;
    fout << "  } else {" << endl;
    fout << "    doService $sock $l" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc accept {sock addr port} {" << endl;
    fout << "  fileevent $sock readable [list svcHandler $sock]" << endl;
    fout << "  fconfigure $sock -buffering line -blocking 0" << endl;
    fout << "  puts $sock \"$addr:$port, You are connected to the pt server.\"" << endl;
    fout << "  puts $sock \"It is now [exec date]\"" << endl;
    fout << "  puts \"Accepted connection from $addr at [exec date]\"" << endl;
    fout << "}" << endl;
    fout << "set power_enable_analysis TRUE" << endl;
    fout << "set power_analysis_mode averaged" << endl;
    fout << "set design  \"" << circuitName <<"\"" << endl;
    fout << "set timing_remove_clock_reconvergence_pessimism true" << endl;
    fout << "set search_path \". " << dbLibPath_mmmc << "\"" << endl;
    fout << "set link_path [list * ";
    for (int i = 0; i < dbLibs_mmmc.size(); i++) fout << dbLibs_mmmc[i] << " ";
    fout << "]" << endl;
    fout << "read_verilog " << verilogFile << endl;
    fout << "current_design $design" << endl;
    fout << "link_design $design" << endl;
    fout << "read_sdc " << sdcFile_mmmc << endl;
    fout << "set_propagated_clock " << clockName << endl;
    if (!noSPEF) fout << "read_parasitics " << spefFile << endl;
    fout << "set report_default_significant_digits 6" << endl;
    fout << "set timing_save_pin_arrival_and_slack true" << endl;
    fout << "set timing_report_always_use_valid_start_end_points false" << endl;
    fout << "check_timing" << endl;
    //if (saifFile != "") fout << "read_saif -strip_path $design " << saifFile << endl;
    if (!ptpxOff) fout << "update_power" << endl;
    fout << "set sh_continue_on_error true" << endl;
    fout << "redirect socket-$svcPort.tmp { socket -server accept $svcPort }" << endl;
    fout << "set inFp [open socket-$svcPort.tmp]" << endl;
    fout << "while { [gets $inFp line] >=0 } {" << endl;
    fout << "  if { [regexp {^Error} $line] } {" << endl;
    fout << "    [exit]" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "vwait events" << endl;
    fout.close();

    //ofstream fout;
    fout.open("launch.cmd");
    fout << "#!/bin/csh -f" << endl;
    if (ptLogSave) fout << "pt_shell " << ptOption << " -f " << ptServerTcl_mmmc << " |& tee pt-" << name_mmmc << ".log" << endl;
    else fout << "pt_shell " << ptOption << " -f " << ptServerTcl_mmmc << endl;
    fout.close();

    // Execute PrimeTime
    char Commands[250];
    /*
    if (ptLogSave)
        sprintf(Commands, "`pt_shell %s -f %s > pt.log-%s ` &", ptOption.c_str(), ptServerTcl_mmmc.c_str(), name_mmmc.c_str() );
    else sprintf(Commands, "`pt_shell %s -f %s` &", ptOption.c_str(), ptServerTcl_mmmc.c_str());
    */
    sprintf(Commands, "chmod +x launch.cmd");
    system(Commands);
    sprintf(Commands, "xterm -T PowerOptPT -iconic -e csh -f launch.cmd &");
    system(Commands);
    //sprintf(Commands, "\\rm -f :exePT");
    //system(Commands);

    designTiming * T_ptr;
    cout<<"[PowerOpt] PrimeTime server contact ... " << name_mmmc <<endl;
    T_ptr = new designTiming();
    while (true) {
        T_ptr->initializeServerContact(ptClientTcl_mmmc);
        wait (5);
        if (T_ptr->checkPT()) break;
    }
    cout<<"[PowerOpt] PrimeTime server contact ... done "<<endl;
    T_mmmc.push_back(T_ptr);
}

void PowerOpt::exeGTServerMMMC(int index)
{
    ifstream file;
    file.open(mmmcFile[index].c_str());
    if(!file) {
        cout << "Cannot open file:" << mmmcFile[index] << endl;
        exit(0);
    }

    string line;
    string libLibPath_mmmc = libLibPath;
    int    ptPort_mmmc =  ptPort + index + 1;
    vector<string> libLibs_mmmc;
    string index_str;
    stringstream out_str;
    string sdcFile_mmmc;
    out_str << index;
    index_str = out_str.str();      // integer to string
    string name_mmmc = "mmmc" + index_str;

    while(std::getline(file, line))
    {
        if (line.find("-name ") != string::npos)
            name_mmmc = getTokenS(line, "-name ");
        if (line.find("-port ") != string::npos)
            ptPort_mmmc = getTokenI(line, "-port ");
        if (line.find("-libpath ") != string::npos)
            libLibPath_mmmc = getTokenS(line, "-libpath ");
        if (line.find("-lib ") != string::npos)
            libLibs_mmmc.push_back(getTokenS(line, "-lib "));
        if (line.find("-sdc ") != string::npos)
            sdcFile_mmmc = getTokenS(line, "-sdc ");
    }
    file.close();

    string ptClientTcl_mmmc = ptClientTcl + "-" + name_mmmc;
    string ptServerTcl_mmmc = ptServerTcl + "-" + name_mmmc;

    ofstream fout;
    // make ptclient tcl
    fout.open(ptClientTcl_mmmc.c_str());

    fout << "proc GetData {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  if {![eof $chan]} {" << endl;
    fout << "    set data [gets $chan]" << endl;
    fout << "    flush stdout" << endl;
    fout << "    return $data" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc SendData {data} {" << endl;
    fout << "  global chan" << endl;
    fout << "  puts $chan $data" << endl;
    fout << "  flush $chan" << endl;
    fout << "}" << endl;
    fout << "proc DoOnePtCommand {cmd} {" << endl;
    fout << "  SendData $cmd" << endl;
    fout << "  return [GetData]" << endl;
    fout << "}" << endl;
    fout << "proc InitClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  set server " << ptServerName << endl;
    fout << "  set chan [socket $server " << ptPort_mmmc <<  "]" << endl;
    fout << "  GetData" << endl;
    fout << "  GetData" << endl;
    fout << "}" << endl;
    fout << "proc CloseClient {} {" << endl;
    fout << "  global chan" << endl;
    fout << "  close $chan" << endl;
    fout << "}" << endl;
    fout.close();

    // make ptserver tcl
    fout.open(ptServerTcl_mmmc.c_str());

    fout << "source " << gtServerCmdTcl << endl;
    fout << "set svcPort " << ptPort_mmmc << endl;
    fout << "proc doService {sock msg} {" << endl;
    //fout << "    puts \"got command $msg on ptserver\"" << endl;
    fout << "    puts $sock [eval \"$msg\"]" << endl;
    fout << "    flush $sock" << endl;
    fout << "}" << endl;
    fout << "proc  svcHandler {sock} {" << endl;
    fout << "  set l [gets $sock]" << endl;
    fout << "  if {[eof $sock]} {" << endl;
    fout << "     close $sock " << endl;
    fout << "  } else {" << endl;
    fout << "    doService $sock $l" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "proc accept {sock addr port} {" << endl;
    fout << "  fileevent $sock readable [list svcHandler $sock]" << endl;
    fout << "  fconfigure $sock -buffering line -blocking 0" << endl;
    fout << "  puts $sock \"$addr:$port, You are connected to the pt server.\"" << endl;
    fout << "  puts $sock \"It is now [exec date]\"" << endl;
    fout << "  puts \"Accepted connection from $addr at [exec date]\"" << endl;
    fout << "}" << endl;
    fout << "set design  \"" << circuitName <<"\"" << endl;
    if (useGTMT) fout << "settings TTimer/NumThreads 4" << endl;
    fout << "settings TTimer/SavePinArrivalAndSlack true" << endl;
    fout << "settings TTimer/ProgramMode high_performance" << endl;
    for (int i = 0; i < libLibs_mmmc.size(); i++)
    fout << "read_lib " << libLibPath_mmmc << "/" << libLibs_mmmc[i] << endl;
    fout << "read_verilog " << verilogFile << endl;
    fout << "flatten" << endl;
    fout << "source " << sdcFile_mmmc << endl;
    if (!noSPEF) fout << "read_parasitics " << spefFile << endl;
    fout << "update_timing" << endl;
    fout << "redirect socket-$svcPort.tmp { socket -server accept $svcPort }" << endl;
    fout << "set inFp [open socket-$svcPort.tmp]" << endl;
    fout << "while { [gets $inFp line] >=0 } {" << endl;
    fout << "  if { [regexp {^Error} $line] } {" << endl;
    fout << "    [exit]" << endl;
    fout << "  }" << endl;
    fout << "}" << endl;
    fout << "vwait events" << endl;
    fout.close();

    //ofstream fout;
    fout.open("launch.cmd");
    fout << "#!/bin/csh -f" << endl;
    if (ptLogSave) fout << "goldtime " << ptOption << " -f " << ptServerTcl_mmmc << " |& tee gt-" << name_mmmc << ".log" << endl;
    else fout << "goldtime " << ptOption << " -f " << ptServerTcl_mmmc << endl;
    fout.close();

    // Execute PrimeTime
    char Commands[250];
    sprintf(Commands, "chmod +x launch.cmd");
    system(Commands);
    sprintf(Commands, "xterm -T PowerOptGT -iconic -e csh -f launch.cmd &");
    system(Commands);

    designTiming * T_ptr;
    cout<<"[PowerOpt] GoldTime server contact ... " << name_mmmc <<endl;
    T_ptr = new designTiming();
    while (true) {
        T_ptr->initializeServerContact(ptClientTcl_mmmc);
        wait (5);
        if (T_ptr->checkPT()) break;
    }
    cout<<"[PowerOpt] GoldTime server contact ... done "<<endl;
    T_mmmc.push_back(T_ptr);
}

void PowerOpt::exeSOCE()
{
    ofstream fout;
    fout.open(soceConfFile.c_str());
    fout << "set design \"" << circuitName << "\"" << endl;
    fout << "set netlist \"" << verilogFile <<"\"" << endl;
    fout << "set sdc \"" << sdcFile << "\"" << endl;
    fout << "set libdir \"" << libLibPath << "\"" << endl;
    fout << "set libworst \"";
    for (int i = 0; i < libLibs.size(); i++)
    fout << "$libdir/" << libLibs[i] << " ";
    fout << "\"" << endl;
    fout << "set libbest \"\"" << endl;
    fout << "set lef \"";
    for (int i = 0; i < lefFiles.size(); i++)
    fout << lefFiles[i] << " ";
    fout << "\"" << endl;
    fout << "set captblworst \"" << soceCapTableWorst << "\"" << endl;
    fout << "set captblbest \"\"" << endl;
    fout.close();
    char Commands[250];
    sprintf(Commands, "cat %s >> %s", soceConfScript.c_str(), soceConfFile.c_str());
    system(Commands);

    fout.open(soceTclFile.c_str());
    fout << "loadConfig ./soce.conf 0" << endl;
    fout << "commitConfig" << endl;
    fout << "readCapTable -worst " << soceCapTableWorst << " -best " << soceCapTableBest << endl;
    fout << "defineRCCorner -late {worst} 125 -early {best} -40" << endl;
    //fout << "setRCFactor -defcap 1.0 -detcap 1.0 -xcap 1.0 -res 1.0" << endl;
    fout << "setCheckMode -mgrid off" << endl;
    fout << "defIn " << defFile << endl;
    fout << "set_propagated_clock " << clockName << endl;
    fout << "setextractrcmode -extended true" << endl;
    fout << "extractRC" << endl;
    fout << "source eco.tcl" << endl;
    fout << "setNanoRouteMode -quiet -routeWithEco true" << endl;
    fout << "setNanoRouteMode -quiet -routeBottomRoutingLayer 1" << endl;
    fout << "setNanoRouteMode -quiet -routeTopRoutingLayer 8" << endl;
    fout << "setNanoRouteMode -quiet -drouteEndIteration default" << endl;
    fout << "setNanoRouteMode -quiet -drouteFixAntenna false" << endl;
    fout << "setNanoRouteMode -quiet -droutePostRouteWidenWireRule NA" << endl;
    fout << "setNanoRouteMode -quiet -drouteStartIteration default" << endl;
    fout << "setNanoRouteMode -quiet -drouteEndIteration default" << endl;
    fout << "setNanoRouteMode -quiet -routeWithTimingDriven false" << endl;
    fout << "setNanoRouteMode -quiet -routeWithSiDriven false" << endl;
    fout << "setNanoRouteMode -quiet -envNumberProcessor 2" << endl;
    fout << "routeDesign -globalDetail" << endl;
    fout << "setExtractRCMode -detail -noReduce -extendedCapTbl" << endl;
    fout << "extractRC" << endl;
    fout << "rcOut -spef " << spefOutFile << endl;
    fout << "saveDesign eco.enc" << endl;
    fout << "report_timing -max_paths 250 > post_route.setup.timing.rpt" << endl;
    fout << "defOut -placement -routing " << defOutFile << endl;
    fout << "saveNetlist " << verilogOutFile << endl;
    fout << "exit" << endl;
    fout.close();

    sprintf(Commands, "encounter -nowin -replay ./soce.tcl");
    system(Commands);
    //exeSOCEFlag = true;
}

int PowerOpt::getTokenI(string line, string option)
{
    uint64_t token;
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    sscanf(arg.c_str(), "%d", &token);
    return token;
}


uint64_t PowerOpt::getTokenULL(string line, string option)
{
    uint64_t token;
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    token = strtoull(arg.c_str(), NULL, 10);
    //sscanf(arg.c_str(), "%d", &token);
    return token;
}


void PowerOpt::updateCellDelay(designTiming *T, int voltage)
{
    Gate *g;
    string cellInst;
    double delay;

    double stdDev,stdDevHi,stdDevLo;
    int stdDevIndex,stdDevMix;

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if (g->getFFFlag()) delay = T->getFFQDelay(cellInst);
        else delay = T->getCellDelay(cellInst);
        g->setDelay (delay);

        // update the random delay variation
        stdDevIndex = (120 - voltage)/10;   // index of the stdDev closer to the 120 side of the spectrum
        stdDevMix = voltage % 10;  // interpolate between the two stdDev values
        stdDevLo = (stdDevMap[g->getCellName()])[stdDevIndex];
        if(stdDevIndex < 8){
          stdDevHi = (stdDevMap[g->getCellName()])[stdDevIndex+1];
          stdDev = stdDevHi * ((double) stdDevMix)/10.0 + stdDevLo * ((double) (10 - stdDevMix))/10.0;
        }else{
          stdDev = stdDevLo;
        }

        //g->setDelayVariation(g->getGateDelay() * stdDev * g->getVariation());
        g->setDelayVariation(delay * stdDev * g->getVariation());
        // SHK-TEST
        g->setChannelLength(stdDev);
    }
}

void PowerOpt::updateBiasCell()
{
    Gate *g;
    string cellInst, master, bias ;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        master = g->getCellName();
        bias = g->getBiasCellName();
        if (master != bias) {
            g->setBiasCellName(master);
            g->setSwapped(true);
        }
    }
}

// Execute oa2def and oa2verilog
void PowerOpt::writeOut(designTiming *T)
{
    char Commands[250];
    sprintf(Commands, "oa2def  -def %s -lib %s -cell %s -view %s", defOutFile.c_str(), libraryName.c_str(), circuitName.c_str(), layoutName.c_str() );
    system(Commands);
    sprintf(Commands, "\\mv oa2def.log %s", libraryName.c_str() );
    system(Commands);
    cout << "[PowerOpt] Output DEF: " << defOutFile << " generated" << endl;
    /*
    sprintf(Commands, "oa2verilog  -verilog %s -lib %s -cell %s -view %s -recursive -excludeLeaf -tieHigh 1\'b1 -tieLow 1\'b0", verilogOutFile.c_str(), libraryName.c_str(), circuitName.c_str(), layoutName.c_str() );
    system(Commands);
    sprintf(Commands, "\\mv oa2verilog.log %s", libraryName.c_str() );
    system(Commands);
    */
    T->writeVerilog(verilogOutFile);
    cout << "[PowerOpt] Output netlist: " << verilogOutFile << " generated" << endl;

}

/* ???
void PowerOpt::readDivFile(string divFileStr)
{
    ifstream file;
    file.open(divFileStr.c_str());
    if(!file) cout << "Cannot open file :" << divFileStr << endl;

    Gate *g;
    string cellInst;
    map<string,bool> gate_dictionary;
    //map<string,bool>::iterator gate_it;

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        gate_dictionary[cellInst]=false;
    }

    while(std::getline(file, cellInst))
    {
        gate_dictionary[cellInst]=true;
    }

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if (gate_dictionary.find(cellInst)->second)
            g->setExclude(false);
        else
            g->setExclude(true);
    }
    file.close();
}
*/

void PowerOpt::setDontTouch()
{
    string cellInst, master;
    // dont_touch cell
    for (int i = 0; i < getGateNum(); i++) {
        master = m_gates[i]->getCellName();
        for (int j = 0; j < dontTouchCell.size(); j++) {
            if (cmpString(dontTouchCell[j], master))
                m_gates[i]->setExclude(true);
        }
    }
    // dont_touch inst
    for (int i = 0; i < getGateNum(); i++) {
        cellInst = m_gates[i]->getName();
        for (int j = 0; j < dontTouchInst.size(); j++) {
            if (cmpString(dontTouchInst[j], cellInst))
                m_gates[i]->setExclude(true);
        }
    }
    /* for (int i = 0; i < getGateNum(); i++) {
        if (m_gates[i]->isExclude())
        cout << m_gates[i]->getName() << " "
             << m_gates[i]->getCellName() << endl;
    } */
}

void PowerOpt::findLeakage(designTiming *T)
{

    int id_index = 0;
    LibcellVector blank_vect;
    //cout << " In find Leakage" << endl;

    for (int i = 0; i < dbLibs.size(); i++) {
        string target_lib = libLibPath + "/" + libLibs[i];
        T->makeLeakList(target_lib, leakListFile);

        ifstream file;
        file.open(leakListFile.c_str());
        if(!file) cout << "[PowerOpt] Cannot open library file ..." << endl;
        string  line, lib_str, leakage_str, footprint_str, funcId_str;
        float   leakage_val;
        char    line_in[1024];
        char    *tmp_str;
        while(std::getline(file, line))
        {
            sprintf(line_in, "%s", line.c_str());
            tmp_str = strtok(line_in, " \t");
            lib_str = tmp_str;
            tmp_str = strtok(NULL, " \t");
            leakage_str = tmp_str;
            tmp_str = strtok(NULL, " \t");
            footprint_str = tmp_str;
            sscanf(leakage_str.c_str(), "%f", &leakage_val);
            libLeakageMap[lib_str] = (double) leakage_val;
            libFootprintMap[lib_str] = footprint_str;

            if (useGT) {
                funcId_str = footprint_str;
                if (swapOp == 1) {
                    funcId_str = findBaseName(lib_str);
                } else if (swapOp == 2) {
                    funcId_str = funcId_str + libLibs[i];
                }
                Libcell *l = new Libcell(lib_str);
                l->setCellFuncId(funcId_str);
                l->setCellDelay((double) 1/leakage_val); // TEMP for GT
                l->setCellLeak((double) leakage_val);
                m_libCells.push_back(l);
                if (libNameIdMap.count(funcId_str) == 0) {
                    libNameIdMap[funcId_str] = id_index++;
                    cellLibrary.push_back(blank_vect);
                }
            }
        }
        file.close();
    }
    char Commands[250];
    if ( !ptLogSave ) {
        sprintf(Commands, "\\rm %s*", leakListFile.c_str());
        system(Commands);
    }

}

void PowerOpt::findLeakagePT(designTiming *T)
{
    cout<<"[PowerOpt] Find cell power from PT ... " <<endl;
    Gate *g;
    string cellInst, master, swap_cell;
    double cellLeak, cellDelay, cellSlack;
    for (int i = 0; i < libSuffix.size(); i++)
    {
        cout<<"[PowerOpt] Library : " << i <<endl;
        for (int j = 0; j < getGateNum(); j++)
        {
            g = m_gates[j];
            cellInst = g->getName();
            master = g->getCellName();
            if (cellInst.find(getClockName()) != string::npos)
            {
                g->setCTSFlag(true);
                continue;
            }
            swap_cell = findBaseName(master)+libSuffix[i];
            if (master.compare(swap_cell) != 0)
            {
                T->sizeCell(cellInst, swap_cell);
                g->setCellName(swap_cell);
            }
        }
        T->putCommand("update_power");
        for (int j = 0; j < getGateNum(); j++)
        {
            g = m_gates[j];
            cellInst = g->getName();
            master = g->getCellName();
            if (g->getCTSFlag()) continue;
            if (totPWRFlag) cellLeak = T->getCellPower(cellInst);
            else cellLeak = T->getCellLeak(cellInst);
            cellDelay = T->getCellDelay(cellInst);
            cellSlack = T->getCellSlack(cellInst);
            g->setLeakList(cellLeak);
            g->setDelayList(cellDelay);
            g->setSlackList(cellSlack);
            g->setCellList(master);
        }
    }

    restoreNet(T);
}

double PowerOpt::getLeakPower()
{

    Gate *g;
    string master;
    double total_leakage = 0;

    for (int j = 0; j < getGateNum(); j++) {
        g = m_gates[j];
        master = g->getCellName();
        total_leakage += libLeakageMap[master];
    }
    total_leakage = total_leakage*0.000000001; // reporting in Watts (accounting for the nano offset in the library values)
    return total_leakage;

}

void PowerOpt::testCellPowerDelay(string cellName, designTiming *T)
{
    Gate *g;
    string cellInst, master, swap_cell;
    double cellLeak, cellDelay;
    string fileName = cellName + ".rpt";
    ofstream fout;
    fout.open(fileName.c_str());

    for (int j = 0; j < getGateNum(); j++) {
        g = m_gates[j];
        master = g->getCellName();
        if (g->getCTSFlag()) continue;
        if (findBaseName(master).compare(cellName) == 0) break;
    }
    for (int i = 0; i < libSuffix.size(); i++)
    {
        swap_cell = findBaseName(master)+libSuffix[i];
        master = g->getCellName();
        cellInst = g->getName();
        if (master.compare(swap_cell) != 0)
        {
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
        }
        cellLeak = T->getCellLeak(cellInst);
        cellDelay = T->getCellDelay(cellInst);
        fout << g->getCellName() << "\t" << cellLeak << "\t"
             << cellDelay << endl;
    }

    fout.close();
    restoreNet(T);
}
void PowerOpt::testCellSize()
{
    string master;
    cout << "[PowerOpt-TEST] find down-sized swap cell" << endl;
    for (int i = 0; i < getGateNum(); i++) {
        master = m_gates[i]->getCellName();
        if (isDownsizable(m_gates[i]))
            cout << master << "\t\t" << findDownCell(master) << endl;
        else
            cout << master << "\t\t" << "CANNOT DOWNSIZE" << endl;
    }
    cout << "[PowerOpt-TEST] find up-sized swap cell" << endl;
    for (int i = 0; i < getGateNum(); i++) {
        master = m_gates[i]->getCellName();
        if (isUpsizable(m_gates[i]))
            cout << master << "\t\t" << findUpCell(master) << endl;
        else
            cout << master << "\t\t" << "CANNOT UPSIZE" << endl;
    }
    cout << "[PowerOpt-TEST] find cell power from PT" << endl;
    for (int i = 0; i < getGateNum(); i++) {
        if (m_gates[i]->getCTSFlag()) continue;
        cout << m_gates[i]->getName() << "\t"
             << m_gates[i]->getCellName() << "\t";
        for (int j = 0; j < libSuffix.size(); j++)
            cout << m_gates[i]->getCellLeakTest(j) << "\t";
        cout << endl;
    }

}

void PowerOpt::testSlack()
{
    Gate *g;
    list<Gate *> gate_list;
    list<Gate *>::iterator  it;
    string cellInst, master, swap_cell;


    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if(!g->getCTSFlag() ) {
            g->setChecked(true);
            g->setSlack(T_main->getCellSlack(cellInst));
            g->setSensitivity(g->getSlack());
            gate_list.push_back(g);
        }
    }

    gate_list.sort(GateSensitivityS2L());

    ofstream fout;
    fout.open("slack.rpt");

    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        double slack = g->getSlack();
        fout << g->getName() << "\t" << slack << endl;
    }
    fout.close();
}

void PowerOpt::testRunTime()
{
    int num_swap = testValue1;
    int num_ISTA = testValue2;

    Gate *g;
    list<Gate *> gate_list;
    list<Gate *>::iterator  it;
    string cellInst, master, swap_cell;

    srand(0);

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (g->isExclude()) continue;
        if(isDownsizable(g) && !g->getCTSFlag() ) {
            g->setChecked(true);
            g->setSensitivity((double)rand());
            gate_list.push_back(g);
        }
    }

    gate_list.sort(GateSensitivityL2S());

    while(!gate_list.empty()){
    list<Gate *> gate_list_sub;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        if (g->isHolded()) continue;
        gate_list_sub.push_back(g);
        g->setHolded(true);
        it = gate_list.erase(it);
        it --;
        if (gate_list_sub.size() == num_swap) break;
    }
    for ( it = gate_list_sub.begin(); it != gate_list_sub.end(); it++)
    {
        g = *it;
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = findDownCell(master);
        T_main->sizeCell(cellInst, swap_cell);
        cout << "SIZE: " << cellInst << endl;
    }
    gate_list_sub.clear();
    if (useGT) T_main->updateTiming();
    cout << "SLACK: " << T_main->getWorstSlack(getClockName()) << endl;;
    num_ISTA --;
    if (num_ISTA == 0) break;
    }
}

void PowerOpt::restoreNet(designTiming *T)
{
    Gate *g;
    string cellInst, master, swap_cell ;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        if (g->isExclude()) continue;
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = g->getBiasCellName();
        if (master != swap_cell) {
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
        }
        //cout << cellInst << " : " << swap_cell << " : " << g->getLeakHVT() << " : " << g->getLeakNVT() << " : " << g->getLeakLVT() << endl;
    }
}

double PowerOpt::getCellSlack(Gate *g, designTiming *T)
{
    double worstSlack = T->getCellSlack(g->getName());
    double tempSlack;
    Gate *fanin;
    Gate *fanout;
    for (int i=0; i < g->getFaninGateNum(); ++i) {
        fanin = g->getFaninGate(i);
        tempSlack = T->getCellSlack(fanin->getName());
        if (tempSlack < worstSlack) worstSlack = tempSlack;
    }
    for (int i=0; i < g->getFanoutGateNum(); ++i) {
        fanout = g->getFanoutGate(i);
        tempSlack = T->getCellSlack(fanout->getName());
        if (tempSlack < worstSlack) worstSlack = tempSlack;
    }
    return worstSlack;
}

double PowerOpt::computeTimingSens(Gate *g, designTiming *T)
{
    double sensitivity;
    if ( sensFuncTiming == 1 )
        sensitivity = computeTimingSF1(g, T);
    else if ( sensFuncTiming == 2 )
        sensitivity = computeTimingSF2(g, T, (updateOp == 1));
    else if ( sensFuncTiming == 3 )
        sensitivity = computeTimingSF3(g, T, (updateOp == 1));
    else
        sensitivity = computeTimingSF1(g, T);
    return sensitivity;
}

double PowerOpt::computeTimingSF1(Gate *g, designTiming *T)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    //double slack, slack_prime;
    //double delta_slack;
    cellInst = g->getName();
    master = g->getCellName();

    g->setSlack(T->getCellSlack(cellInst));
    sensitivity = - g->getSlack();

    return sensitivity;
}

double PowerOpt::computeTimingSF2(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack, slack_prime;
    double delta_slack;
    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findUpCell(master);
    slack = T->getCellSlack(cellInst);
    g->setSlack(slack);

    if (change) {
        T->sizeCell(cellInst, swap_cell);   // downsize
        slack_prime = T->getCellSlack(cellInst);
        T->sizeCell(cellInst, master);      // restore
        delta_slack = slack_prime - slack;
        g->setDeltaSlack(delta_slack);
    }
    if (slack > 0 || g->getDeltaSlack() < 0) sensitivity = 0;
    else sensitivity = g->getDeltaSlack()/(0.000001 + slack - getCurWNS());

    return sensitivity;
}

double PowerOpt::computeTimingSF3(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack, slack_prime;
    double delta_slack;
    double power, power_prime;
    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findUpCell(master);

    slack = T->getCellSlack(cellInst);
    g->setSlack(slack);
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    g->setCellName(swap_cell);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    g->setCellName(master);
    g->setDeltaLeak(power_prime - power);
    if (change) {
        T->sizeCell(cellInst, swap_cell);   // downsize
        slack_prime = T->getCellSlack(cellInst);
        T->sizeCell(cellInst, master);      // restore
        delta_slack = slack_prime - slack;
        g->setDeltaSlack(delta_slack);
    }
    if (slack > 0 || g->getDeltaSlack() < 0) sensitivity = 0;
    else sensitivity = g->getDeltaSlack()/(0.000001 + slack - getCurWNS())/g->getDeltaLeak();

    return sensitivity;
}

double PowerOpt::computeSF1(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MIN;
    string cellInst, master, swap_cell;
    double delay, delay_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    delay = change ? T->getCellDelay(cellInst) : g->getCellDelay();
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    //power = libLeakageMap[g->getCellName()];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    delay_prime = change ? T->getCellDelay(cellInst) : g->getCellDelay();
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(delay_prime-delay);
    sensitivity = g->getDeltaLeak()/g->getDeltaDelay();
    return sensitivity;
}

double PowerOpt::computeSF3(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack, slack_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    slack = change ? T->getCellSlack(cellInst) : g->getCellSlack();
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    slack_prime = change ? T->getCellSlack(cellInst) : g->getCellSlack();
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(slack - slack_prime);
    sensitivity = g->getDeltaLeak()/g->getDeltaDelay();
    return sensitivity;
}
// SF2 with no cell change
double PowerOpt::computeSF2(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_curr;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    slack_curr = T->getCellSlack(cellInst) - (double) getSensVar();
    // apply delta slack which will  be changed after real swap
    //else slack_curr = T->getCellSlack(cellInst) + slack_prime - slack;
    //cout << T->getCellSlack(cellInst) <<"\t"<< slack_curr;
    //cout <<"\t"<< slack <<"\t"<< slack_prime << endl;
    if(change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setSlack(slack_curr);
    g->setDeltaLeak(power - power_prime);
    if (slack_curr > 0 ) sensitivity = g->getDeltaLeak() * slack_curr;
    else sensitivity = slack_curr / g->getDeltaLeak();

    return sensitivity;
}

double PowerOpt::computeSF4(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack, slack_prime;
    double power, power_prime;
    double tmp_slack;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    slack = 0;
    slack_prime = 0;
    for (int l=0; l < g->getFaninGateNum(); ++l)
    {
        Gate *fanin = g->getFaninGate(l);
        tmp_slack = change ? T->getCellSlack(fanin->getName())
                           : fanin->getCellSlack();
        slack = slack + tmp_slack;
    }
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    for (int l=0; l < g->getFaninGateNum(); ++l)
    {
        Gate *fanin = g->getFaninGate(l);
        tmp_slack = change ? T->getCellSlack(fanin->getName())
                           : fanin->getCellSlack();
        slack_prime = slack_prime + tmp_slack;
    }
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(slack - slack_prime);
    sensitivity = g->getDeltaLeak()/g->getDeltaDelay();
    return sensitivity;
}

double PowerOpt::computeSF5(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_prime;
    double delay, delay_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);

    delay = change ? T->getCellDelay(cellInst) : g->getCellDelay();
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    slack_prime = T->getCellSlack(cellInst);
    delay_prime = change ? T->getCellDelay(cellInst) : g->getCellDelay();
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    if (change) g->setDeltaDelay(delay_prime - delay);
    sensitivity = g->getDeltaLeak() * slack_prime /g->getDeltaDelay();

    return sensitivity;
}

double PowerOpt::computeSF6(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_prime;
    double power, power_prime;
    int fifo_num;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);

    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    slack_prime = T->getCellSlack(cellInst);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    fifo_num = g->getFaninGateNum() + g->getFanoutGateNum();
    sensitivity = g->getDeltaLeak() * slack_prime / (double)fifo_num;

    return sensitivity;
}

double PowerOpt::computeSF7(Gate *g, designTiming *T, bool change)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_prime;
    double power, power_prime;
    double fifo, fifo_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);

    fifo = 0;
    fifo_prime = 0;
    for (int l=0; l < g->getFaninGateNum(); ++l)
    {
        Gate *fanin = g->getFaninGate(l);
        fifo = fifo + T->getCellSlack(fanin->getName());
    }
    for (int k=0; k < g->getFanoutGateNum(); ++k)
    {
        Gate *fanout = g->getFanoutGate(k);
        fifo = fifo + T->getCellSlack(fanout->getName());
    }
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    if (change) T->sizeCell(cellInst, swap_cell);  // downsize
    g->setCellName(swap_cell);
    slack_prime = T->getCellSlack(cellInst);
    for (int l=0; l < g->getFaninGateNum(); ++l)
    {
        Gate *fanin = g->getFaninGate(l);
        fifo_prime = fifo_prime + T->getCellSlack(fanin->getName());
    }
    for (int k=0; k < g->getFanoutGateNum(); ++k)
    {
        Gate *fanout = g->getFanoutGate(k);
        fifo_prime = fifo_prime + T->getCellSlack(fanout->getName());
    }
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    if (change) T->sizeCell(cellInst, master); // restore
    g->setCellName(master);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(fifo - fifo_prime);
    sensitivity = g->getDeltaLeak() * slack_prime / g->getDeltaDelay();

    return sensitivity;
}

double PowerOpt::computeSF8(Gate *g, designTiming *T)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_curr;
    double delay, delay_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    delay = g->getCellDelay();
    g->setCellName(swap_cell);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    delay_prime = g->getCellDelay();
    slack_curr = T->getCellSlack(cellInst) - (double) getSensVar();
    g->setCellName(master);
    g->setSlack(slack_curr);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(delay_prime - delay);
    if (slack_curr - g->getDeltaDelay() > 0 )
        sensitivity = g->getDeltaLeak() * (slack_curr - g->getDeltaDelay()) ;
    else sensitivity =  (slack_curr - g->getDeltaDelay()) / g->getDeltaLeak();

    return sensitivity;
}

double PowerOpt::computeSF9(Gate *g, designTiming *T)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_curr;
    double delay, delay_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    delay = g->getCellDelay();
    g->setCellName(swap_cell);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    delay_prime = g->getCellDelay();
    slack_curr = T->getCellSlack(cellInst) - (double) getSensVar();
    g->setCellName(master);
    g->setSlack(slack_curr);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(delay_prime - delay);
    if (slack_curr > 0 )
        sensitivity = g->getDeltaLeak() / g->getDeltaDelay() * slack_curr;
    else sensitivity = slack_curr / g->getDeltaLeak() * g->getDeltaDelay();

    return sensitivity;
}

double PowerOpt::computeSF10(Gate *g, designTiming *T)
{
    double sensitivity = DBL_MAX;
    string cellInst, master, swap_cell;
    double slack_curr;
    double delay, delay_prime;
    double power, power_prime;

    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);
    if (leakPT) power = g->getCellLeak();
    else power = libLeakageMap[master];
    delay = g->getCellDelay();
    g->setCellName(swap_cell);
    if (leakPT) power_prime = g->getCellLeak();
    else power_prime = libLeakageMap[swap_cell];
    delay_prime = g->getCellDelay();
    slack_curr = T->getCellSlack(cellInst) - (double) getSensVar();
    g->setCellName(master);
    g->setSlack(slack_curr);
    g->setDeltaLeak(power - power_prime);
    g->setDeltaDelay(delay_prime - delay);
    if (slack_curr > 0 )
        sensitivity = g->getDeltaLeak() / g->getDeltaDelay() * slack_curr * slack_curr;
    else sensitivity = slack_curr * slack_curr / g->getDeltaLeak() * g->getDeltaDelay() * (-1);

    return sensitivity;
}

double PowerOpt::computeSensitivity(Gate *g, designTiming *T, bool change)
{
    double sensitivity;
    if ( getSensFunc() == 1 )
        sensitivity = computeSF2(g, T, false);
    else if ( getSensFunc() == 2 )
        sensitivity = computeSF1(g, T, change);
        //sensitivity = computeSF1(g, T, true);
    else if ( getSensFunc() == 3 )
        sensitivity = computeSF9(g, T);
    else if ( getSensFunc() == 4 )
        sensitivity = computeSF10(g, T);
    //else if ( getSensFunc() == 8 && getFlag() == false )
    //    sensitivity = computeSF2(g, T, change);
    //else if ( getSensFunc() == 8 && getFlag() == true )
    //    sensitivity = computeSF5(g, T, true);
    else
        sensitivity = computeSF2(g, T, false);
    return sensitivity;
}

int PowerOpt::downsizeTest(designTiming *T)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;
    double targetWNS, currWNS;
    if (getSwapstep() == 5)
        targetWNS = -0.010;
    else if (getSwapstep() == 6)
        targetWNS = -0.020;
    else if (getSwapstep() == 7)
        targetWNS = -0.050;
    else if (getSwapstep() == 8)
        targetWNS = -0.100;
    else
        targetWNS = -0.010;

    int downsizeCnt = 0;
    int downCell;
    while (true) {
        downCell = rand() % (getGateNum()-1);
        g = m_gates[downCell];
        cellInst = g->getName();
        master = g->getCellName();
        if (cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (isDownsizable(g) && !g->getCTSFlag()) {
            swap_cell = findDownCell(master);
            T->sizeCell(cellInst, swap_cell);
            if (T->getCellSlack(cellInst) < 0) {
                g->setCellName(swap_cell);
                downsizeCnt ++;
                cout << downsizeCnt <<"\t"<< cellInst <<"\t"<< master <<"\t";
                cout << swap_cell << endl;
            } else {
                T->sizeCell(cellInst, master);
            }
        }
        currWNS = T->getWorstSlack(getClockName());
        if (currWNS < targetWNS ) break;
    }
    return downsizeCnt;
}

int PowerOpt::downsizeAll(designTiming *T)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;

    int downsizeNum;
    if (getSwapstep() == 1)
        downsizeNum = (int) (getGateNum() * 0.005);
    else if (getSwapstep() == 2)
        downsizeNum = (int) (getGateNum() * 0.01);
    else if (getSwapstep() == 3)
        downsizeNum = (int) (getGateNum() * 0.05);
    else if (getSwapstep() == 4)
        downsizeNum = (int) (getGateNum() * 0.10);
    else
        downsizeNum = 10;

    int downsizeCnt = 0;
    int downCell;
    while (true) {
        downCell = rand() % (getGateNum()-1);
        g = m_gates[downCell];
        cellInst = g->getName();
        master = g->getCellName();
        if (cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (isDownsizable(g) && !g->getCTSFlag()) { swap_cell = findDownCell(master);
            T->sizeCell(cellInst, swap_cell);
            //if (T->getCellSlack(cellInst) < 0) {
                g->setCellName(swap_cell);
                downsizeCnt ++;
                cout << downsizeCnt <<"\t"<< cellInst <<"\t"<< master <<"\t";
                cout << swap_cell << endl;
            //} else {
            //    T->sizeCell(cellInst, master);
            //}
        }
        if (downsizeCnt == downsizeNum ) break;
    }
    return downsizeCnt;
}

int PowerOpt::kick_move(designTiming *T, double kick_val)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;

    int downsizeNum = (int) (getGateNum() * kick_val);
    int downsizeCnt = 0;
    int downCell;
    while (true) {
        downCell = rand() % (getGateNum()-1);
        g = m_gates[downCell];
        cellInst = g->getName();
        master = g->getCellName();
        if (cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (isDownsizable(g) && !g->getCTSFlag()) {
            swap_cell = findDownCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
            downsizeCnt ++;
            cout << downsizeCnt <<"\t"<< cellInst <<"\t"<< master <<"\t";
            cout << swap_cell << endl;
        }
        if (downsizeCnt == downsizeNum ) break;
    }
    return downsizeCnt;
}

void PowerOpt::updateCellLib(designTiming *T)
{
    Gate *g;
    cout << "=======Update cell lib from PT "<< endl;

    for (int i = 0; i < getGateNum(); i ++)
    {
        g = m_gates[i];
        g->setCellName(T->getLibCell(g->getName()));
    }
}

void PowerOpt::setBestLib(designTiming *T)
{
    Gate *g;
    for (int i = 0; i < getGateNum(); i ++)
    {
        g = m_gates[i];
        g->setBestCellName(g->getCellName());
    }
}

void PowerOpt::restoreLib(designTiming *T)
{
    Gate *g;
    for (int i = 0; i < getGateNum(); i ++)
    {
        g = m_gates[i];
        if (g->getCellName() != g->getBestCellName()){
            g->setCellName(g->getBestCellName());
            T->sizeCell(g->getName(), g->getBestCellName());
        }
    }
}

void PowerOpt::checkCTSCells()
{
    Gate *g;
    string gateName, cellName;
    for (int i = 0; i < getGateNum(); i ++)
    {
        g = m_gates[i];
        gateName = g->getName();
        cellName = g->getCellName();

        if (g->getFFFlag()) continue;
        {
            if(gateName.find(getClockName())  != string::npos)
            {
                g->setCTSFlag(true);
                //cout << "CTS gate : " << gateName << endl;
            }
        }
    }
}

void PowerOpt::setBaseName()
{
    for (int i = 0; i < getGateNum(); i ++)
        m_gates[i]->setBaseName(findBaseName(m_gates[i]->getCellName()));
}

string PowerOpt::findBaseName(string master)
{
    string base_name = master;
    size_t pos_t, pos_n;
    // Find base name
    for (int i = 0; i < libSuffix.size(); i++){
        pos_t = base_name.find(libSuffix[i]);
        pos_n = libSuffix[i].size();
        if (pos_n == 0) continue;
        if (pos_t != string::npos) {
            base_name = base_name.erase(pos_t, pos_n);
            break;
        }
    }
    return base_name;
}

string PowerOpt::findSuffixName(string master)
{
    string suffix_name = "";
    size_t pos_t, pos_n;
    // Find Suffix name
    for (int i = 0; i < libSuffix.size(); i++){
        pos_t = master.find(libSuffix[i]);
        pos_n = libSuffix[i].size();
        if (pos_n == 0) continue;
        if (pos_t != string::npos) {
            suffix_name = libSuffix[i];
            break;
        }
    }
    return suffix_name;
}

string PowerOpt::findDownCell(string master)
{
    int lib_idx = libNameMap[master];
    if (m_libCells[lib_idx]->getDnCell() == NULL) {
        cout << "[PowerOpt] Fatal error: findDownCell()" << endl;
        return master;
    } else {
        return m_libCells[lib_idx]->getDnCell()->getCellName();
    }
}

string PowerOpt::findUpCell(string master)
{
    int lib_idx = libNameMap[master];
    if (m_libCells[lib_idx]->getUpCell() == NULL) {
        cout << "[PowerOpt] Fatal error: findUpCell()" << endl;
        return master;
    } else {
        return m_libCells[lib_idx]->getUpCell()->getCellName();
    }
}

/* OLD
string PowerOpt::findDownCell(string master)
{
    string down_cell;
    if (swapOp == 2) down_cell = findDownSize(master);
    else down_cell = findDownSwap(master);
    if (down_cell.size() == 0) {
        cout << "[PowerOpt] Fatal error: findDownCell()" << endl;
        return master;
    } else {
        return down_cell;
    }
}

string PowerOpt::findUpCell(string master)
{
    string up_cell;
    if (swapOp == 2) up_cell = findUpSize(master);
    else up_cell = findUpSwap(master);
    if (up_cell.size() == 0) {
        cout << "[PowerOpt] Fatal error: findUpCell()" << endl;
        return master;
    } else {
        return up_cell;
    }
}
*/
string PowerOpt::findDownSwap(string master)
{
    for (int i = 0; i < libSuffix.size(); i++)
        if (master.compare(findBaseName(master)+libSuffix[i]) == 0)
            if (i != libSuffix.size()-1)
                return findBaseName(master)+libSuffix[i+1];
    return "";
}

string PowerOpt::findUpSwap(string master)
{
    for (int i = 0; i < libSuffix.size(); i++)
        if (master.compare(findBaseName(master)+libSuffix[i]) == 0)
            if (i != 0)
                return findBaseName(master)+libSuffix[i-1];
    return "";
}

string PowerOpt::findDownSize(string master)
{
    string base_name = findBaseName(master);
    int idx, id_cur, id_next;
    for (int i = 0; i < libNameVect.size(); i++) {
        if (base_name.compare(libNameVect[i]) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == 0) return "";
    id_cur = libNameIdMap[libNameVect[idx]];
    id_next = libNameIdMap[libNameVect[idx - 1]];
    if (id_cur != id_next) return "";
    return libNameVect[idx - 1] + findSuffixName(master);
}

string PowerOpt::findUpSize(string master)
{
    string base_name = findBaseName(master);
    int idx, id_cur, id_next;
    for (int i = 0; i < libNameVect.size(); i++)
        if (base_name.compare(libNameVect[i]) == 0) {
            idx = i;
            break;
        }
    if (idx == libNameVect.size() -1 ) return "";
    id_cur = libNameIdMap[libNameVect[idx]];
    id_next = libNameIdMap[libNameVect[idx + 1]];
    if (id_cur != id_next) return "";
    return libNameVect[idx + 1] + findSuffixName(master);
}

bool PowerOpt::isDownsizable(Gate *g)
{
    string master = g->getCellName();
    int lib_idx = libNameMap[master];
    return (m_libCells[lib_idx]->getDnCell() != NULL);
}

bool PowerOpt::isUpsizable(Gate *g)
{
    string master = g->getCellName();
    int lib_idx = libNameMap[master];
    return (m_libCells[lib_idx]->getUpCell() != NULL);
}

/* OLD
bool PowerOpt::isDownsizable(Gate *g)
{
    string master = g->getCellName();
    if (swapOp == 2) {
        if (findDownSize(master).size() == 0) return false;
        else return true;
    } else {
        string smallest = findBaseName(master)+libSuffix[libSuffix.size()-1];
        if (master.compare(smallest)!=0) return true;
        else return false;
    }
}

bool PowerOpt::isUpsizable(Gate *g)
{
    string master = g->getCellName();
    if (swapOp == 2) {
        if (findUpSize(master).size() == 0) return false;
        else return true;
    } else {
        string largest = findBaseName(master)+libSuffix[0];
        if (master.compare(largest)!=0) return true;
        else return false;
    }
}
*/

void PowerOpt::sensitivityUpdateAll(list<Gate *> gate_list, designTiming *T)
{
    Gate *g;
    double cell_sensitivity;
    list<Gate *>::iterator  it;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        if (getUpdateOp() == 1)
            cell_sensitivity = computeSensitivity(g, T, false);
        else
            cell_sensitivity = computeSensitivity(g, T, true);
        g->setSensitivity(cell_sensitivity);
    }
    //cout << "Sensivities are updated for all cell in gate_list" << endl;
}

void PowerOpt::sensitivityUpdateTimingAll(list<Gate *> gate_list, designTiming *T)
{
    Gate *g;
    double cell_sensitivity;
    list<Gate *>::iterator  it;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        cell_sensitivity = computeTimingSens(g, T);
        g->setSensitivity(cell_sensitivity);
    }
    cout << "[PowerOpt] Sensivities are updated for all cell in gate_list" << endl;
}

//sensitivityUpdate : update sensitivity of the gate and its
//fanin/fanout cells
void PowerOpt::sensitivityUpdate(Gate *g, designTiming *T)
{
    Gate *g_tmp;
    Gate *g_fanin = NULL;
    Gate *g_fanout = NULL;
    Gate *g_next = NULL;
    double cell_sensitivity;
    GateVector check_gate_L1; // neighbor cell
    GateVector check_gate_L2; // on-path cell
    check_gate_L1.clear();
    check_gate_L2.clear();

    bool update = (getUpdateOp() == 1);

    if(g->isChecked()) {
        if (getCurOp() == 2)
            cell_sensitivity = computeTimingSens(g, T);
        else
            cell_sensitivity = computeSensitivity(g, T, update);
        g->setSensitivity(cell_sensitivity);
    }

    double minSlack = DBL_MAX;
    for (int j = 0; j < g->getFaninGateNum(); ++j){
        g_tmp = g->getFaninGate(j);
        if (g_tmp->getSlack() < minSlack) {
            minSlack = g_tmp->getSlack();
            g_fanin = g_tmp;
        }
        check_gate_L1.push_back(g_tmp);
    }
    minSlack = DBL_MAX;
    for (int k = 0; k < g->getFanoutGateNum(); ++k){
        g_tmp = g->getFanoutGate(k);
        if (g_tmp->getSlack() < minSlack) {
            minSlack = g_tmp->getSlack();
            if (g->getFFFlag()) g_fanout = NULL;
            else g_fanout = g_tmp;
        }
        check_gate_L1.push_back(g_tmp);
    }
    //Find gates in the critical path to update sensitivity
    //Fanin side
    while (true)
    {
        g_next = NULL;
        minSlack = DBL_MAX;
        if (g_fanin == NULL || g_fanin->getFFFlag()) break;
        for (int i = 0; i < g_fanin->getFaninGateNum(); ++i) {
            g_tmp = g_fanin->getFaninGate(i);
            if (g_tmp->getSlack() < minSlack) {
                minSlack = g_tmp->getSlack();
                g_next = g_tmp;
            }
        }
        if (g_next != NULL) {
            //if(g_next->isChecked())
            check_gate_L2.push_back(g_next);
            g_fanin = g_next;
            //cout << "FI added : " << g_next->getSlack() << endl;
        }
        else break;
    }
    //Fanout side
    while (true)
    {
        g_next = NULL;
        minSlack = DBL_MAX;
        if (g_fanout == NULL || g_fanout->getFFFlag()) break;
        for (int i = 0; i < g_fanout->getFanoutGateNum(); ++i) {
            g_tmp = g_fanout->getFanoutGate(i);
            if (g_tmp->getSlack() < minSlack) {
                minSlack = g_tmp->getSlack();
                g_next = g_tmp;
            }
        }
        if (g_next != NULL && !g_next->getFFFlag()) {
            check_gate_L2.push_back(g_next);
            g_fanout = g_next;
            //cout << "FO added : " << g_next->getSlack() << endl;
        }
        else break;
    }

    for (int i = 0; i < check_gate_L1.size(); ++i) {
        string cellInst = check_gate_L1[i]->getName();
        check_gate_L1[i]->setSlack(T->getCellSlack(cellInst));
        if(check_gate_L1[i]->isChecked()) {
            if (getCurOp() == 2)
                cell_sensitivity = computeTimingSens(check_gate_L1[i], T);
            else
                cell_sensitivity = computeSensitivity(check_gate_L1[i], T, update);
            check_gate_L1[i]->setSensitivity(cell_sensitivity);
        }
    }
    check_gate_L1.clear();

    for (int i = 0; i < check_gate_L2.size(); ++i) {
        string cellInst = check_gate_L2[i]->getName();
        check_gate_L2[i]->setSlack(T->getCellSlack(cellInst));
        if(check_gate_L2[i]->isChecked()) {
            if (getCurOp() == 2)
                cell_sensitivity = computeTimingSens(check_gate_L2[i], T);
            else
                cell_sensitivity = computeSensitivity(check_gate_L2[i], T, update);
            check_gate_L2[i]->setSensitivity(cell_sensitivity);
        }
    }
    check_gate_L2.clear();

}

//sensitivityUpdatetiming : update sensitivity of the gate and its
//fanin/fanout cells
void PowerOpt::sensitivityUpdateTiming(Gate *g, designTiming *T)
{
    GateVector gv;
    gv.clear();
    gv.push_back(g);
    double sensitivity;

    addGateFiDFS (g, &gv);
    addGateFoDFS (g, &gv);

    for (int i = 0; i < gv.size(); ++i) {
        sensitivity = computeTimingSens(gv[i], T);
        //cout << "SensUpdate: " << gv[i]->getSensitivity() << " -> " << sensitivity << endl;
        gv[i]->setSensitivity(computeTimingSens(gv[i], T));
    }
    //cout << "SensUpdate Done" << endl;
    gv.clear();
}

bool PowerOpt::addGateFiDFS(Gate *g, GateVector* gv)
{
    Gate *g_fi;
    for (int i = 0; i < g->getFaninGateNum(); i++) {
        g_fi = g->getFaninGate(i);
        if (g_fi->getCTSFlag()) continue;
        gv->push_back(g_fi);
        if (!g_fi->getFFFlag()) addGateFiDFS (g_fi, gv);
    }
}

bool PowerOpt::addGateFoDFS(Gate *g, GateVector* gv)
{
    Gate *g_fo;
    for (int i = 0; i < g->getFanoutGateNum(); i++) {
        g_fo = g->getFanoutGate(i);
        if (g_fo->getCTSFlag()) continue;
        if (!g_fo->getFFFlag()) {
            gv->push_back(g_fo);
            addGateFiDFS (g_fo, gv);
        }
    }
}

// Sensitivity-based Downsizing for Leakage Optimization
void PowerOpt::reduceLeak(designTiming *T)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;
    ofstream fout;
    fout.open("reducLeak.log");

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (g->isExclude()) continue;
        if(isDownsizable(g) && !g->getCTSFlag()) {
            g->setChecked(true);
            gate_list.push_back(g);
        }
    }
    cout << "[PowerOpt] Sensitivity calculation ... " << endl;
    sensitivityUpdateAll(gate_list, T);
    cout << "[PowerOpt] Sensitivity calculation ... done " << endl;
    gate_list.sort(GateSensitivityL2S());

#ifdef CHECK
    cout << "initial sensitivity" << endl;
    list<Gate *>::iterator  it;
    int cnt_tmp = 0;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        cout << cnt_tmp <<"\t"<< g->getName() <<"\t";
        cout << g->getCellName() <<"\t"<< libLeakageMap[g->getCellName()] <<"\t";
        cout << g->getSlack() <<"\t"<< g->getSensitivity() << endl;
        ++ cnt_tmp;
    }
#endif

    setSwapcnt(0);
    setSwaptry(0);

    double power, power_prime;
    double prev_leak = getCurLeak();
    double imp;
    int lastSwapcnt = 0;
    int deltaSwapcnt = 0;
    int totalListSize = (int) gate_list.size();
    double progCheck = 0;
    cout << "[PowerOpt] Swap cells ... " << endl;
    cout << ">0%--------------------50%-------------------100%<" << endl;

    while(!gate_list.empty()){
        g = gate_list.front();
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = findDownCell(master);
        if (swap_cell == "ERROR") {
            gate_list.pop_front();
            continue;
        }
        //power = g->getCellLeak();
        power = libLeakageMap[master] * 0.000000001;
        T->sizeCell(cellInst, swap_cell);
        if (useGT) T->updateTiming();
        for (int i = 0; i < mmmcFile.size(); i++) {
            if (mmmcOn) {
                T_mmmc[i]->sizeCell(cellInst, swap_cell);
                if (useGT) T_mmmc[i]->updateTiming();
            }
        }
        incSwaptry();

        if ( checkViolation(cellInst, T) ) { //restore
            T->sizeCell(cellInst, master);
            for (int i = 0; i < mmmcFile.size(); i++) {
                if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, master);
            }
            gate_list.pop_front();
            g->setChecked(false); // not in the gate_list
        } else {  // accept swap
            g->setCellName(swap_cell);
            g->setSwapped(true);
            incSwapcnt();
            //power_prime = g->getCellLeak();
            power_prime = libLeakageMap[swap_cell] * 0.000000001;
            setCurLeak ( getCurLeak() - power + power_prime);

            if(!isDownsizable(g)) {
                gate_list.pop_front();
                g->setChecked(false); // not in the gate_list
            }
            if(!noSensUp) {
                sensitivityUpdate(g,T);
                gate_list.sort(GateSensitivityL2S());
            }
        }
        if (getSwaptry() % 10 == 0) fout << "CUR_LEAK\t" << getCurLeak() << endl;

        if (getSwaptry() % 100 == 0) {
            imp = prev_leak - getCurLeak();
            deltaSwapcnt = getSwapcnt() - lastSwapcnt;
            int currentListSize = (int) gate_list.size();
            double progress = (double) (totalListSize - currentListSize) / (double) totalListSize * (double) 100;
            //cout.precision(2);
            fout << "IMP_LEAK\t" << imp << "\taccept\t" << deltaSwapcnt  << "\tprogress\t" << (int)progress << endl;
            //if ( deltaSwapcnt < getStopCond() || (int) progress > getStopCond2()) {
            while ((int)progress > (int)progCheck ) {
                cout << "." << flush;
                progCheck = progCheck + ((double)getStopCond()/50);
            }
            if ((int)progress > getStopCond()) {
                fout << "Optimization Terminated!!" << endl;
                break;
            }
            prev_leak = getCurLeak();
            lastSwapcnt = getSwapcnt();
        }
    }
    gate_list.clear();
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        g->setChecked(false);
    }
    cout << endl << "[PowerOpt] Leakage optimization ... done " << endl;
    fout.close();
}
// Sensitivity-based Downsizing for Leakage Optimization
void PowerOpt::reduceLeakMultiSwap(designTiming *T)
{
    Gate *g;
    list<Gate *> gate_list;
    list<Gate *>::iterator  it;
    string cellInst, master, swap_cell;

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true);
        if (g->isExclude()) continue;
        if(isDownsizable(g) && !g->getCTSFlag() ) {
            g->setChecked(true);
            gate_list.push_back(g);
        }
    }

    cout << "[PowerOpt] Sensitivity calculation ... " << endl;
    sensitivityUpdateAll(gate_list, T);
    gate_list.sort(GateSensitivityL2S());
#ifdef CHECK
    cout << "initial sensitivity" << endl;
    int cnt_tmp = 0;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        cout << cnt_tmp <<"\t"<< g->getName() <<"\t";
        cout << g->getCellName() <<"\t"<< libLeakageMap[g->getCellName()] <<"\t";
        cout << g->getSlack() <<"\t"<< g->getSensitivity() << endl;
        ++ cnt_tmp;
    }
#endif
    cout << "[PowerOpt] Sensitivity calculation ... done " << endl;
    setSwapcnt(0);
    setSwaptry(0);

    int totalListSize = (int) gate_list.size();

    double progCheck = 0;

    cout << "[PowerOpt] Swap cells ... " << endl;
    cout << ">0%--------------------50%-------------------100%<" << endl;
    while(!gate_list.empty()){

    list<Gate *> gate_list_sub;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        if (g->isHolded()) continue;
        gate_list_sub.push_back(g);
        g->setHolded(true);
        checkGateFanin(g);
        checkGateFanout(g);
        it = gate_list.erase(it);
        it --;
        if (gate_list_sub.size() == getSwapstep()) break;
    }
    clearHolded();
    // Swap cells
    for ( it = gate_list_sub.begin(); it != gate_list_sub.end(); it++)
    {
        g = *it;
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = findDownCell(master);
        if (swap_cell == "ERROR") {
            it = gate_list_sub.erase(it);
            it --;
            continue;
        }
        T->sizeCell(cellInst, swap_cell);
        for (int i = 0; i < mmmcFile.size(); i++) {
            if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, swap_cell);
        }
        incSwaptry();
    }
    //updateTiming for GT
    if (useGT) T->updateTiming();
    if (mmmcOn && useGT) {
        for (int i = 0; i < mmmcFile.size(); i++)
            T_mmmc[i]->updateTiming();
    }

    // Accept or Deny
    for ( it = gate_list_sub.begin(); it != gate_list_sub.end(); it++)
    {
        g = *it;
        cellInst = g->getName();
        if ( checkViolation(cellInst, T) )  //restore
            g->setHolded(true);
        else {
            g->setHolded(false);
            incSwapcnt();
        }
    }
    // restore or swap
    for ( it = gate_list_sub.begin(); it != gate_list_sub.end(); it++)
    {
        g = *it;
        cellInst = g->getName();
        master = g->getCellName();
        if (g->isHolded()) { // restore
            T->sizeCell(cellInst, master);
            for (int i = 0; i < mmmcFile.size(); i++) {
                if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, master);
            }
            //cout << g->getName() << " : resotred" << endl;
        }
        else  { // swap
            swap_cell = findDownCell(master);
            g->setCellName (swap_cell);
            g->setSwapped(true);
            //cout << g->getName() << " : swapped" << endl;
            //incSwapcnt();
        }
    }
    // Update Sensitivity
    for ( it = gate_list_sub.begin(); it != gate_list_sub.end(); it++)
    {
        g = *it;
        if (g->isHolded()) { // restore
            g->setHolded(false);
            g->setChecked(false);
        } else {
            if (!isDownsizable(g))
                g->setChecked(false);
            else {
                gate_list.push_back(g);
            }
            sensitivityUpdate(g,T);
        }
    }
    //int num_multiswap = gate_list_sub.size();
    gate_list_sub.clear();
    gate_list.sort(GateSensitivityL2S());
    int currentListSize = (int) gate_list.size();
    double progress = (double)(totalListSize - currentListSize)/
                      (double) totalListSize * (double) 100;
    while ((int) progress > (int) progCheck ) {
        cout << "." << flush;
        progCheck = progCheck + ((double)getStopCond()/50);
    }
    if ( (int) progress > getStopCond()) {
        //cout << "Optimization Terminated!!" << endl;
        break;
    }
    //cout << "SWAP : " << num_multiswap <<
    //     << "\tProgress : " << progress << endl;
    } // while loop
    gate_list.clear();
    cout << endl << "[PowerOpt] Leakage optimization ... done " << endl;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        g->setChecked(false);
    }
}
void PowerOpt::checkGateFanin(Gate *g)
{
    Gate *fanin;
    for (int i = 0; i < g->getFaninGateNum(); i++) {
        fanin = g-> getFaninGate(i);
        if (fanin->getCTSFlag() || fanin->isHolded()) continue;
        fanin->setHolded( true );
        if (!fanin->getFFFlag()) checkGateFanin (fanin);
    }
}

void PowerOpt::checkGateFanout(Gate *g)
{
    Gate *fanout;
    for (int i = 0; i < g->getFanoutGateNum(); i++) {
        fanout = g-> getFanoutGate(i);
        if (fanout->getCTSFlag() || fanout->isHolded()) continue;
        fanout->setHolded( true );
        if (!fanout->getFFFlag()) checkGateFanout (fanout);
    }
}

void PowerOpt::optTiming(designTiming *T)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;
    double worstSlack = T->getWorstSlack(getClockName());
    setCurWNS(worstSlack);

    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName()) != string::npos) {
            g->setCTSFlag(true);
            continue;
        }
        if (g->isExclude()) continue;
        g->setSlack(T->getCellSlack(cellInst));
        if (g->getSlack() > 0) continue;
        if(isUpsizable(g)) {
            g->setChecked(true);
            gate_list.push_back(g);
        }
    }

    sensitivityUpdateTimingAll(gate_list, T);
    gate_list.sort(GateSensitivityL2S());

#ifdef CHECK
    cout << "initial sensitivity" << endl;
    list<Gate *>::iterator  it;
    int cnt_tmp = 0;
    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        cout << cnt_tmp <<"\t"<< g->getName() <<"\t";
        cout << g->getSensitivity() <<"\t"<< g->getSlack() << endl;
        if (++cnt_tmp == 20) break;
    }
#endif
    setSwapcnt(0);
    setSwaptry(0);

    setCurWNS (T->getWorstSlack(getClockName()));
    double prev_WNS = getCurWNS();
    //double saved_WNS = 0;

    while(!gate_list.empty()){
        g = gate_list.front();
        if (prev_WNS >= 0)
            break;
        if (g->getSensitivity() < 0) {
            break;
            //sensitivityUpdateAll(gate_list, T);
            //gate_list.sort(GateSensitivityL2S());
            //g = gate_list.front();
        }
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = findUpCell(master);
        T->sizeCell(cellInst, swap_cell);
        for (int i = 0; i < mmmcFile.size(); i++) {
            if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, swap_cell);
        }
        incSwaptry();
        setCurWNS (T->getWorstSlack(getClockName()));
        if (prev_WNS >= getCurWNS()) { // restore
            T->sizeCell(cellInst, master);
            for (int i = 0; i < mmmcFile.size(); i++) {
                if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, master);
            }
            gate_list.pop_front();
        } else {
            g->setCellName(swap_cell);
            g->setSwapped(true);
            prev_WNS = getCurWNS();
            incSwapcnt();
            if(!isUpsizable(g)) {
                gate_list.pop_front();
                g->setChecked(false); // not in the gate_list
            }
            sensitivityUpdateTiming(g,T);
            gate_list.sort(GateSensitivityL2S());
        }
        if (getSwaptry() % 100 == 0)
            cout << "TRY_SWAP_WNS\t" << getSwaptry() << "\t"
                 << getSwapcnt() << "\t" << getCurWNS() << endl;
        /*if (getSwaptry() % 200 == 0) {
            if (saved_WNS == prev_WNS) {
                cout << "Optimization Terminated!!" << endl;
                break;
            }
            saved_WNS = prev_WNS;
        }*/
#ifdef CHECK
        cout << "Updated sensitivity" << endl;
        cnt_tmp = 0;
        for ( it = gate_list.begin(); it != gate_list.end(); it++) {
            g = *it;
            cout << cnt_tmp <<"\t"<< g->getName() <<"\t";
            cout << g->getSensitivity() <<"\t"<< g->getSlack() << endl;
            if (++cnt_tmp == 20) break;
        }
#endif
    }
    //set clear
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        g->setChecked(false);
    }
}

void PowerOpt::PTEvaluation(designTiming *T)
{
    Gate *g;
    string cellInst, master, swap_cell;
    int testCycle = m_gates.size();
    int repeatCycle = 5;
    time_t t_start, t_end;
    double avg_time;
    double worstSlack;

    cout << "---1.Slack access start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            worstSlack = T->getCellSlack(cellInst);
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Slack access. Time:\t" << avg_time << endl;

    cout << "---2.WNS access start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            worstSlack = T->getWorstSlack(getClockName());
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---WNS access. Time:\t" << avg_time << endl;

    cout << "---3.Leak access start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            worstSlack = T->getCellLeak(cellInst);
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Leak access. Time:\t" << avg_time << endl;

    cout << "---4.Cell swap start" << endl;
    t_start = time(NULL);
    cout << "---Cell swap start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            master = g->getCellName();
            if (isDownsizable(g))
                swap_cell = findDownCell(master);
            else
                swap_cell = findUpCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Cell swap. Time:\t" << avg_time << endl;

    repeatCycle = 1;

    cout << "---5.Cell swap+slack start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            master = g->getCellName();
            if (isDownsizable(g))
                swap_cell = findDownCell(master);
            else
                swap_cell = findUpCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
            worstSlack = T->getCellSlack(cellInst);
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Cell swap+slack. Time:\t" << avg_time << endl;

    cout << "---6.Cell swap+WNS start" << endl;
    t_start = time(NULL);
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            master = g->getCellName();
            if (isDownsizable(g))
                swap_cell = findDownCell(master);
            else
                swap_cell = findUpCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
            worstSlack = T->getWorstSlack(getClockName());
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Cell swap+WNS. Time:\t" << avg_time << endl;

    cout << "---7.Cell swap+Leak start" << endl;
    t_start = time(NULL);
    testCycle = 50;
    for (int j = 0; j < repeatCycle; j++) {
        for (int i = 0; i < testCycle; i ++) {
            g = m_gates[i];
            cellInst = g->getName();
            master = g->getCellName();
            if (isDownsizable(g))
                swap_cell = findDownCell(master);
            else
                swap_cell = findUpCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
            worstSlack = T->getCellLeak(cellInst);
        }
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)(testCycle*repeatCycle);
    cout << "---Cell swap+Leak. Time:\t" << avg_time << endl;
}

void PowerOpt::PTEvaluation(designTiming *T, int freq, int test)
{
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;
    double worstSlack;
    GateVector check_gate;
    time_t t_start, t_end;
    double avg_time;
    int testCnt;
    //test = 1 : slack
    //test = 2 : WNS
    //test = 3 : leakage
    if (test == 3) testCnt = 1000;
    else testCnt = getGateNum();

    for (int i = 0; i < testCnt; i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  == string::npos)
            gate_list.push_back(g);
    }

    int swapCount = 0;
    int itrCount = 0;
    check_gate.clear();

    t_start = time(NULL);
    while(!gate_list.empty()){
        if (swapCount < freq) {
            g = gate_list.front();
            cellInst = g->getName();
            master = g->getCellName();
            if (isDownsizable(g))
                swap_cell = findDownCell(master);
            else
                swap_cell = findUpCell(master);
            T->sizeCell(cellInst, swap_cell);
            g->setCellName(swap_cell);
            gate_list.pop_front();
            check_gate.push_back(g);
            swapCount++;
            if (!gate_list.empty()) continue;
        }
        if (test == 2) {
            worstSlack = T->getWorstSlack(getClockName());
        } else {
            for (int i = 0; i < check_gate.size(); ++i) {
                cellInst = check_gate[i]->getName();
                if (test == 1)
                    worstSlack = T->getCellSlack(cellInst);
                else
                    worstSlack = T->getCellLeak(cellInst);
            }
        }
        check_gate.clear();
        swapCount=0;
        itrCount ++;
    }
    t_end = time(NULL);
    avg_time = (double)(t_end - t_start)/(double)itrCount;
    if (test == 2) {
        cout << "WNS_access_Time_with " << freq << "swap : ";
        cout << avg_time << endl;
    }
    else if (test == 1) {
        cout << "Slack_access_Time_with " << freq << "swap : ";
        cout << avg_time << endl;
    }
    else {
        cout << "Leakage_access_Time_with " << freq << "swap : ";
        cout << avg_time << endl;
    }
}

void PowerOpt::compBiasCellName()
{
    for (int i = 0; i < m_gates.size(); i ++)
    {
        Gate *g = m_gates[i];
        if (g->getFFFlag())
            continue;
            string cellName = g->getCellName();
            string biasCellName = cellName + "_P10P10";
            g->setBiasCellName(biasCellName);
    }
}

double PowerOpt::convertToDouble(const string& s)
{
    std::istringstream i(s);
    double x;
    i >> x;
    return x;
}

void PowerOpt::makeLibList()
{
    ofstream fout;
    fout.open(libListTcl.c_str());

    fout << "#!/usr/bin/tclsh" << endl;
    fout << "set inputFileName   [lindex $argv 0]" << endl;
    fout << "set outputFileName  [lindex $argv 1]" << endl;
    fout << "set inFp            [open $inputFileName]" << endl;
    fout << "set outFp           [open $outputFileName w]" << endl;
    fout << "while { [gets $inFp line] >=0 } {" << endl;
    fout << "    if { [regexp {^cell \\(} $line] } {" << endl;
    fout << "        regsub -all \"{|}\" $line \"\" temp" << endl;
    fout << "        regsub -all {\\(|\\)} $temp \"\" temp " << endl;
    fout << "        set cell [lindex $temp 1]" << endl;
    fout << "    } elseif { [regexp {cell_footprint} $line]} {" << endl;
    fout << "        regsub -all \";|\\\"\" $line \"\" temp" << endl;
    fout << "        set foot [lindex $temp 2]" << endl;
    fout << "    } elseif { [regexp {cell_leakage} $line]} {" << endl;
    fout << "        regsub -all \";\" $line \"\" temp" << endl;
    fout << "        set leakage [lindex $temp 2]" << endl;
    fout << "        puts $outFp \"$cell $foot $leakage\"" << endl;
    fout << "    } " << endl;
    fout << "}" << endl;
    fout << "close $inFp" << endl;
    fout << "close $outFp" << endl;
    fout.close();

    char Commands[250];
    string target_lib = libLibPath + "/" + libLibs[0];
    sprintf(Commands, "chmod +x %s", libListTcl.c_str());
    system (Commands);
    sprintf(Commands, "%s %s libList.tmp", libListTcl.c_str(),
            target_lib.c_str());
    system (Commands);

    ifstream file;
    file.open("libList.tmp");
    string  line, lib_str, foot_str, leakage_str;
    //float leakage_val;
    size_t pos_t1, pos_t2;
    while(std::getline(file, line))
    {
        pos_t1 = line.find_first_of(" ");
        pos_t2 = line.find_last_of(" ");
        lib_str = line.substr(0, pos_t1);
        pos_t1++;
        foot_str = line.substr(pos_t1, pos_t2 - pos_t1);
        pos_t2++;
        leakage_str = line.substr(pos_t2, line.size() - pos_t2);
        // INCOMPLETE
    }
}

void PowerOpt::readLibCells(designTiming *T)
{
    string libNameStr = T->getLibNames();
    cout << "LIB: " << libNameStr << endl;
    char strIn[1024];
    char Commands[250];
    sprintf(strIn, "%s", libNameStr.c_str());
    string libListName;

    char* pch;

    pch = strtok (strIn, " ,\t");
    while (pch != NULL)
    {
        string strTmp = pch;
        libNames.push_back(strTmp);
        pch = strtok (NULL, " ,\t");
    }
    //
    int id_index = 0;
    LibcellVector blank_vect;

    for (int i = 0; i < libNames.size(); i++) {
        libListName = libNames[i] + ".list";
        T->makeCellList(libNames[i], libListName);
        ifstream file;
        file.open(libListName.c_str());
        if(!file) cout << "Cannot open cell library file" << endl;
        string  lineread;
        char    *name_str;
        char    *funcId_str;
        char    *delay_str;
        char    line_in[1024];
        float   delay_val;
        string  name_string, funcId_string;

        int lineCnt = 0;
        while(std::getline(file, lineread)) {
            stringstream ss;
            ss << lineCnt++;
            sprintf(line_in, "%s", lineread.c_str());
            funcId_str = strtok(line_in, " \t");
            name_str = strtok(NULL, " \t");
            delay_str = strtok(NULL, " \t");
            name_string = name_str;
            sscanf(delay_str, "%f", &delay_val);
            /*
            funcId_string = funcId_str;
            if (funcId_string.compare("unknown") == 0)
                funcId_string = funcId_string + ss.str();
            */
            funcId_string =libFootprintMap[name_string];
            if (swapOp == 1) {
                funcId_string = findBaseName(name_string);
            } else if (swapOp == 2) {
                funcId_string = funcId_string + libNames[i];
            }

            Libcell *l = new Libcell(name_string);
            l->setCellFuncId(funcId_string);
            l->setCellDelay((double) delay_val);
            l->setCellLeak(libLeakageMap[name_string]);
            m_libCells.push_back(l);
            if (libNameIdMap.count(funcId_string) == 0) {
                libNameIdMap[funcId_string] = id_index++;
                cellLibrary.push_back(blank_vect);
            }
        }
        if ( !ptLogSave ) {
            sprintf(Commands, "\\rm -f temp.sensOpt");
            system(Commands);
            sprintf(Commands, "\\rm %s*", libListName.c_str());
            system(Commands);
        }
    }
}
/*
void PowerOpt::readLibCellsFile()
{
    ifstream file;
    file.open(libListFile.c_str());
    if(!file){
        cout << "Cannot open cell library file" << endl;
    }
    string  lineread;
    char    *libstr;
    char    *cellstr;
    string  libstring, cellstring, libstrtmp;
    int     libIndex = 0;
    char    line_in[1024];

    string drive_strength;
    vector<string> blank_vect;
    cellLibrary.push_back(blank_vect);

    while(std::getline(file, lineread))
    {
        sprintf(line_in, "%s", lineread.c_str());
        libstr  = strtok(line_in, " \t");
        cellstr = strtok(NULL, " \t");
        libstrtmp = libstr;
        cellstring = cellstr;
        libNameVect.push_back(cellstr);

        if (libstring == libstrtmp) {
            libNameIdMap[cellstring] = libIndex;
        }
        else {
            // next type of cell
            libNameIdMap[cellstring] = ++libIndex;
            libstring = libstrtmp;

            // add another row to the cell library
            cellLibrary.push_back(blank_vect);
        }

          cellLibrary[libIndex].push_back(cellstring);

    }
    cout<<"Number of Lib Cell : " << libNameVect.size() << endl;
    cout<<"========Library Cell Read Finish======= "<<endl;
}
*/
void PowerOpt::updateLibCells()
{
    Libcell *l;
    int id_idx;
    string name_str;
    string funcId_str;
    for (int i = 0; i < m_libCells.size(); i++) {
        l = m_libCells[i];
        id_idx = libNameIdMap[l->getCellFuncId()];
        cellLibrary[id_idx].push_back(l);
        libNameMap[l->getCellName()] = i;
    }

    for (int i = 0; i < cellLibrary.size(); i++) {
        if (exeOp == 1 && !useGT)
            sort (cellLibrary[i].begin(), cellLibrary[i].end(), LibcellDelayL2S());
        else
            sort (cellLibrary[i].begin(), cellLibrary[i].end(), LibcellLeakS2L());
        for (int j = 0; j < cellLibrary[i].size(); j++) {
            if (j == 0)
                cellLibrary[i][j]->setDnCell(NULL);
            else
                cellLibrary[i][j]->setDnCell(cellLibrary[i][j-1]);
            if (j == (cellLibrary[i].size() - 1))
                cellLibrary[i][j]->setUpCell(NULL);
            else
                cellLibrary[i][j]->setUpCell(cellLibrary[i][j+1]);
        }
    }
    /*
    for (int i = 0; i < m_libCells.size(); i++) {
        cout << m_libCells[i]->getCellName() << "\t";
        cout << m_libCells[i]->getCellFuncId() << "\t";
        cout << m_libCells[i]->getCellDelay() << "\t";
        cout << m_libCells[i]->getCellLeak() << "\t";
        if (m_libCells[i]->getUpCell() != NULL)
        cout << m_libCells[i]->getUpCell()->getCellName() << "\t";
        else cout << "NULL" << "\t";
        if (m_libCells[i]->getDnCell() != NULL)
        cout << m_libCells[i]->getDnCell()->getCellName() << "\t";
        else cout << "NULL" << "\t";
        cout << endl;
    }
    */
}

bool PowerOpt::netPartition(designTiming *T)
{
    Gate *g;
    string cellInst;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName())  != string::npos)
            g->setCTSFlag(true); // CTS cell
        if(g->getFFFlag()) {
            ff_des_list.push_back(g);
        }
    }
    cout << "Number of FF in the list :\t" << ff_des_list.size() << endl;

    DivnetVector allDivnet;

    int net_cnt =  0;
    list<Gate *>::iterator it;

    Divnet *d;

    for ( it = ff_des_list.begin(); it != ff_des_list.end(); it++) {
        g = *it;
        //if (g->isCheckDes()) continue;
        d = new Divnet(net_cnt++);

        Terminal *oTerm;
        Gate *fanin = NULL;
        // find data fan-in of flip flip (g)
        for (int i = 0; i < g->getInputSubnetNum(); ++i) {
            Subnet *s = g->getInputSubnet(i);
            if (s->inputIsPad()) continue;
            oTerm = s->getOutputTerminal();
            if (oTerm->getName() == "D") {
                fanin = s->getInputTerminal()->getGate();
                break;
            }
        }
        if ( fanin == NULL ) continue;
        cout << g->getName() <<" "<< fanin->getName() << endl;

        // DFS search from the 'fanin' cell
        if (!fanin->getFFFlag() && !fanin->getCTSFlag()) {
            if ( fanin->getDiv() == NULL) {
                d = new Divnet(net_cnt++);
                d->add_gate(fanin); fanin->setDiv(d);
                addGateDFS (fanin, d);
            } else
                continue;
        } else
            continue;
        allDivnet.push_back(d);

        //for ( int i = 0; i < d->getGateNum(); i++ ) {
        //    cout << (d->getGate(i))->getName() << endl;
        //}
    }
    d = new Divnet(net_cnt++);
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        if ( !g->getCTSFlag() && g->getDiv() == NULL )
            d->add_gate(g); g->setDiv(d);
    }
    cout << "#SUBNET\t" << allDivnet.size() << endl;
    sort (allDivnet.begin(), allDivnet.end(), DivnetSizeL2S());
    // assign partitions
    //
    int partition[getDivNum()];
    for (int i = 0; i < getDivNum(); i++)
        partition[i] = 0;

    for (int i = 0; i < allDivnet.size(); i++) {
        d = allDivnet[i];
        cout << "SUBNET\t" << i <<"\t"<<d->getGateNum()<< endl;
        int minNum = INT_MAX;
        int minPart = 0;
        for (int j=0; j < getDivNum(); j++) {
            if ( partition[j] < minNum ) {
                minNum = partition[j];
                minPart = j;
            }
        }
        partition[minPart] += d->getGateNum();
        d->setPartNum(minPart);
    }
    for (int i = 0; i < allDivnet.size(); i++) {
        d = allDivnet[i];
        cout << "SUBNET\t" << i <<"\t"<<d->getGateNum()
             << "\t" << d->getPartNum() << endl;
    }
    char fileName [128];
    ofstream fout;
    for (int i = 0; i < getDivNum(); i++) {
        cout << "# partition " << i << " : " << partition[i] << endl;
        sprintf(fileName, "partition-%d", i);
        fout.open (fileName);
        for (int j = 0; j < allDivnet.size(); j++) {
            d = allDivnet[j];
            if ( d->getPartNum() != i ) continue;
            for ( int k = 0; k < d->getGateNum(); k++)
                fout << (d->getGate(k))->getName() << endl;
        }
        fout.close();
    }
    return true;
}


bool PowerOpt::addGateDFS(Gate *g, Divnet *d)
{
    //search to Fanin
    Gate *fanin;
    for (int i = 0; i < g->getFaninGateNum(); i++) {
        fanin = g-> getFaninGate(i);
        if (fanin->getDiv() != NULL) continue;
        if (fanin->getCTSFlag()) continue;
        d->add_gate(fanin);
        fanin->setDiv(d);
        if (!fanin->getFFFlag()) addGateDFS (fanin, d);
    }

    Gate *fanout;
    for (int i = 0; i < g->getFanoutGateNum(); i++) {
        fanout = g-> getFanoutGate(i);
        if (fanout->getDiv() != NULL) continue;
        if (fanout->getCTSFlag()) continue;
        if (fanout->getFFFlag()) {
            fanout->setCheckDes(true);
            continue;
        }
        d->add_gate(fanout);
        fanout->setDiv(d);
        addGateDFS (fanout, d);
    }

    return true;
}


void PowerOpt::netPartitionPT(designTiming *T)
{
    Gate *g;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        if(g->getFFFlag()) {
            ff_src_list.push_back(g);
            ff_des_list.push_back(g);
        }
    }
    cout << "Number of FF in the list :\t" << ff_src_list.size() << endl;

    DivnetVector allDivnet;
    int net_cnt =  0;
    list<Gate *>::iterator it_src;

    while (true) {
        for ( it_src = ff_src_list.begin(); it_src != ff_src_list.end(); it_src++) {
            g = *it_src;
            if ( g->getDiv_src() == NULL && g->getDiv_des() == NULL) break;
        }
        if ( g->getDiv_src() != NULL || g->getDiv_des() != NULL ) break;
        Divnet *d = new Divnet(net_cnt);
        d->addFF_src(g);
        g->setDiv_src(d);
        //cout << d->getFF_srcName() <<"\t"<< d->getFF_srcNum() << endl;
        //cout << d->getFF_desName() <<"\t"<< d->getFF_desNum() << endl;
        while (true){
            int des_cnt = checkDES(d, T);
            int src_cnt = checkSRC(d, T);
            cout << "DES_CNT :\t" << des_cnt << endl;
            cout << "SRC_CNT :\t" << src_cnt << endl;
            if (des_cnt == 0 && src_cnt == 0) break;
        }


        net_cnt++;
        //cout << "Number of SRC FF :\t" << d->getFF_srcNum() << endl;
        for (int i = 0; i < d->getFF_srcNum(); i++)
            cout << (d->getFF_src(i))->getName() << "\t";
        cout << endl;
        //cout << "Number of DES FF :\t" << d->getFF_desNum() << endl;
        for (int i = 0; i < d->getFF_desNum(); i++)
            cout << (d->getFF_des(i))->getName() << "\t";
        cout << endl;
        allDivnet.push_back(d);
    }
    Divnet *d_tmp;
    cout << "#SUBNET\t" << allDivnet.size() << endl;
    for ( int i = 0; i < allDivnet.size(); i++ ) {
        d_tmp = allDivnet[i];
        cout << "SUBNET\t" << i <<"\t"<<d_tmp->getFF_desNum()<<"\t"
             << d_tmp->getFF_srcNum() << endl;
    }
}

int PowerOpt::checkDES( Divnet *d, designTiming *T ) {

    Gate *g_src, *g_des;
    list<Gate *>::iterator it_des;
    // make src FF union
    string srcName, desName;

    srcName  = "\\\"";
    int cnt_src = 0;
    for (int i = 0; i < d->getFF_srcNum(); i++) {
        g_src = d->getFF_src(i);
        if (g_src->isCheckSrc()) continue;
        g_src->setCheckSrc(true);
        srcName = srcName + " " + g_src->getName();
        cnt_src++;
    }
    srcName = srcName + "\\\"";
    //cout << d->getFF_srcName() << "\t Num :" << cnt_src << endl;
    if (cnt_src == 0) return cnt_src;

    int cnt_des = 0;
    for ( it_des = ff_des_list.begin(); it_des != ff_des_list.end(); it_des++) {
        g_des = *it_des;
        if (g_des->getDiv_des() != NULL || g_des->getDiv_src() == d)
            continue;
        desName  = "\\\"" + g_des->getName() + "\\\"";
        if (T->checkDependFF(srcName, desName) ) {
            d->addFF_des(g_des);
            g_des->setDiv_des(d);
            cnt_des++;
        }
    }

    return cnt_des;
}

int PowerOpt::checkSRC( Divnet *d, designTiming *T ) {

    Gate *g_src, *g_des;
    list<Gate *>::iterator it_src;
    string srcName, desName;

    desName = "\\\"";
    int cnt_des = 0;
    for (int i = 0; i < d->getFF_desNum(); i++) {
        g_des = d->getFF_des(i);
        if (g_des->isCheckDes()) continue;
        g_des->setCheckDes(true);
        desName = desName + " " + g_des->getName();
        cnt_des++;
    }
    desName = desName + "\\\"";
    //cout << d->getFF_desName() << "\t Num :" << cnt_des << endl;
    if (cnt_des == 0) return cnt_des;

    int cnt_src = 0;
    for ( it_src = ff_src_list.begin(); it_src != ff_src_list.end(); it_src++) {
        g_src = *it_src;
        if (g_src->getDiv_src() != NULL || g_src->getDiv_des() == d)
            continue;
        srcName  = "\\\"" + g_src->getName() + "\\\"";
        if (T->checkDependFF (srcName, desName)) {
            //cout << "Dependent\t" << g_src->getName() <<"\t"
            //     <<  d->getFF_desName() << endl;
            d->addFF_src(g_src);
            g_src->setDiv_src(d);
            cnt_src++;
        }
    }
    return cnt_src;
}

// Check design
int PowerOpt::checkMaxTr( designTiming *T ) {
    Gate *g;
    string cellInst, master;
    int vioCnt = 0;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        master = g->getCellName();
        if (!T->checkMaxTran(cellInst)) {
           cout << "MaxTr Violation :\t" << cellInst << "\t" << master << endl;
           vioCnt++;
        }
    }
    return vioCnt;
}

int PowerOpt::fixMaxTr( designTiming *T ) {
    Gate *g;
    list<Gate *> gate_list;
    string cellInst, master, swap_cell;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        if(cellInst.find(getClockName()) != string::npos) {
            g->setCTSFlag(true);
            continue;
        }
        if (g->isExclude()) continue;
        if (T->checkMaxTran(cellInst)) continue;
        if(isUpsizable(g)) {
            g->setChecked(true);
            gate_list.push_back(g);
        }
    }

    setSwapcnt(0);
    while(!gate_list.empty()){
        g = gate_list.front();
        cellInst = g->getName();
        master = g->getCellName();
        swap_cell = findUpCell(master);
        T->sizeCell(cellInst, swap_cell);
        //for (int i = 0; i < mmmcFile.size(); i++) {
        //    if (mmmcOn) T_mmmc[i]->sizeCell(cellInst, swap_cell);
        //}
        g->setCellName(swap_cell);
        g->setSwapped(true);
        incSwapcnt();
        if(!isUpsizable(g) || T->checkMaxTran(cellInst)) {
            gate_list.pop_front();
            g->setChecked(false); // not in the gate_list
        }
    }
    int vioCnt = 0;
    for (int i = 0; i < getGateNum(); i++) {
        g = m_gates[i];
        cellInst = g->getName();
        master = g->getCellName();
        if (!T->checkMaxTran(cellInst)) {
           cout << "MaxTr Violation :\t" << cellInst << "\t" << master << endl;
           vioCnt++;
        }
    }
    return vioCnt;
}


void PowerOpt::optimizeTargER(designTiming *T) {
    int num_steps;
    vector<int> volt;

    int curVoltage = vStart;
    while(curVoltage >= vEnd){
        volt.push_back(curVoltage);
        curVoltage -= vStep;
    }
    num_steps = volt.size();
    cout << "[PowerOpt] voltage scaling steps:" << num_steps << endl;
    vector<double>  power(num_steps, DBL_MAX);
    vector<double>  leak(num_steps, DBL_MAX);
    vector<double>  final_er(num_steps, DBL_MAX);
    vector<int>     num_er_ff(num_steps, 0);

    int swap_cnt = 0;
    bool change_flag = false;

    checkCTSCells();
    double error_rate;
    //char Commands[250];

    // voltage scaling
    int criticalPathsNum;

    for (int i = 0; i < num_steps; i++) {
        restorePTLib(volt[i], T);
        if (change_flag) {
            T->readTcl("resize.tcl");
        }
        // Extract Critical Paths from STA
        extCriticalPaths(T);
        sort(criticalPaths.begin(), criticalPaths.end(),PathTgL2S());
        clearCheck();
        criticalPathsNum = getCriticalPathsNum();
        if (criticalPathsNum == 0) continue;
        cout << "[PowerOpt] Optimize Paths for ";
        cout << criticalPathsNum << " paths: ";
        swap_cnt = resizePaths(T);
        cout << swap_cnt << " cells swapped" << endl;
        if (swap_cnt != 0) change_flag = true;
        T->writeChange("change.tcl");
        string libNameStr = T->getLibNames();
        T->makeResizeTcl(libNameStr, "resize");

        // Extract Critical Paths after optimizePath()
        // Calculate ErrorRate after optimizePath()
        extCriticalPaths(T);
        // JMS-SHK begin
        // assign the endpoints of criticalPaths to be RZ-FF if they aren't already
        // @@ fix the swap_razor_flops function to swap in RZ-FF where needed
        //swap_razor_flops(T); -> will not chage it expricitely
        //
        // assign RZ FF to registers which have negative slack
        /* RZ assignment based on FF cell slack - incorrect
        for (int j = 0; j < getGateNum(); j++) {
            if (m_gates[j]->getFFFlag()) {
                numFF ++;
                // slack was updated or not ???
                // if update, it can be just m_gates[i]->getSlack();
                double slack_FF = T->getCellSlack(m_gates[j]->getName());
                if (slack_FF < 0) {
                    numRZ++;
                    m_gates[j]->setRZFlag(true);
                } else {
                    m_gates[j]->setRZFlag(false);
                }
            }
        }
        */

        // JMS-SHK end
        error_rate = calErrorRate(totalSimCycle, T);

        cout << "[PowerOpt] Error_rate_at_V" << volt[i] << " : " << error_rate;
        cout << "\t critical_paths : " << getCriticalPathsNum();
        //if (getExeOp() == 7) continue;
        if ( error_rate > (double) targErrorRate) break;
    }
}

// Cell resizing with PT Tcl socket
int PowerOpt::resizePaths(designTiming *T) {
    int swap_cnt = 0;
    string PathString;
    for (int i = 0; i < criticalPaths.size(); i++) {
        Path *p = criticalPaths[i];
        PathString = p->getPathStr();
        //cout << "[" << i << "] : ";
        //cout << criticalPaths[i]->getNumToggled() << " : ";
        //cout << p->getSlack() << " : ";

        if (p->isIgnore()) {
            //cout << endl;
            continue;
        }

        int GateNum = p->getGateNum();
        bool    No_Swap = false;
        int     PathItr = 1;
        while (!No_Swap) {
        int swap_cnt_tmp = 0;
        bool    changed = false;
        bool    swapped = false;

        // consider cells for swapping from front to back in a path
        for (int j = 0; j < GateNum; j++){
            Gate *g = p->getGate(j);
            if (g->isChecked()) continue;
            g->setPathStr(PathString);
            if ( T->getPathSlack(PathString) > 0.00) continue;
            if ( !g->getFFFlag() && !g->getCTSFlag() ) {
                if ( g->getBiasCellName() == g->getCellName()) changed = false;
                else    changed = true;
                if ( resizePathCell(T,g,PathString,PathItr)) swapped = true;
                else    swapped = false;
                if (swapped) swap_cnt_tmp++;
                if (swapped && !changed) swap_cnt++;
            }
        } // for (path p)

        // @NOTE: we may want to change maximum iterations from 3 for Experiment 3
        if ((swap_cnt_tmp==0)||(PathItr > 3)) No_Swap = true;
        PathItr++;
        } // path iteration (critical path)
        for (int k = 0; k < GateNum; k++) p->getGate(k)->setChecked(true); // IS THIS IN THE RIGHT PLACE?
            //cout <<  T->getPathSlack(p->getPathStr()) << "\t[" << PathItr << "]" << "\t[" << swap_cnt << "]" << endl;
        //if (swap_cnt > getGateNum() * getChglimit()) break;
    }
    return swap_cnt;
}

bool PowerOpt::resizePathCell(designTiming *T, Gate *g, string PathString, int PathItr)
{
    bool    swapped = false;
    bool    restore = false;
    int     inst_idx, lib_idx;
    double  CurSlack, NewSlack, ChangeSlack, DeltaSlack, TmpSlack;
    bool    check = false;
    string  cur_inst, swap_cell;
    Gate*   fan_gate = NULL;
    vector<Gate *> check_gates;  // checked fanin and fanout cells for which slack should not decrease

    lib_idx = libNameMap[g->getCellName()];
    inst_idx = libNameIdMap[m_libCells[lib_idx]->getCellFuncId()];
    CurSlack = T->getPathSlack(PathString);
    for (int i = 0; i < g->getFaninGateNum(); i++){
        fan_gate = g->getFaninGate(i);
        if (fan_gate->isChecked()) {
            TmpSlack = T->getPathSlack(fan_gate->getPathStr());
            fan_gate->setSlack(TmpSlack);
            // record this gate as on to check slack for later
            check_gates.push_back(fan_gate);
        }
    }

    for (int j = 0; j < g->getFanoutGateNum(); j++){
        fan_gate = g->getFanoutGate(j);
        if (fan_gate->isChecked()) {
            TmpSlack = T->getPathSlack(fan_gate->getPathStr());
            fan_gate->setSlack(TmpSlack);
            // record this gate as on to check slack for later
            check_gates.push_back(fan_gate);
        }
    }
    //cout << g->getName() << " " << g->getCellName() << endl;
    // try all alternate cell sizes
    for (int k = 0; k < cellLibrary[inst_idx].size(); k++) {
        swap_cell = cellLibrary[inst_idx][k]->getCellName();
        cur_inst = g->getCellName();
        if ( cur_inst == swap_cell ) continue;
        //cout << cur_inst << "->" << swap_cell << endl;
        //RESIZE
        if (T->sizeCell(g->getName(),cellLibrary[inst_idx][k]->getCellName()) == false) {
            cout <<"Resize Fail 1: "<< g->getName()<<" : "<<cur_inst<<" : ";
            cout << cellLibrary[inst_idx][k]->getCellName() << endl;
        }
        NewSlack = T->getPathSlack(PathString);
        DeltaSlack = NewSlack - CurSlack;
        if (check) cout << CurSlack << " -> " << NewSlack << endl;
        if (NewSlack > CurSlack) {
            restore = false;
            // Fain/out check
            for (int l = 0; l < check_gates.size(); l++){
                fan_gate = check_gates[l];
                TmpSlack = T->getPathSlack(fan_gate->getPathStr());
                ChangeSlack = fan_gate->getSlack() - TmpSlack;
                if ( ChangeSlack > 0 ){
                  restore = true;
                  break;
                }
            }
        }
        else { //restore previous cell
            restore = true;
        }

        if (restore) {
            if (T->sizeCell(g->getName(),cur_inst) == false) {
                cout <<"Resize Fail 2: "<<g->getName()<<" : "<< swap_cell <<" : ";
                cout << cur_inst << endl;
            }
        } else {
            CurSlack = NewSlack;
            g->setCellName(swap_cell);
            swapped = true;
            g->setSwapped(true);
        }
    }
    return swapped;
}

void PowerOpt::reducePower(designTiming *T) {
    int targ_errors = (int) targErrorRate * totalSimCycle;
    // initialize plusPaths and minusPaths (P_+ and P_-)
    // note that minusPaths is a vector, but plusPaths is a LIST (for efficient random access, deletion, sorting, etc.)
    // @NOTE: MAYBE THIS CAN BE DONE ELSEWHERE, like when criticalPaths is being formed (it can, but needn't be every time extCritical is called)
    plusPaths.clear();
    minusPaths.clear();

    Path *p;
    double path_slack;
    for (int i = 0; i < toggledPaths.size(); i++)
    {
        p = toggledPaths[i];
        path_slack = p->getSlack();
        //path slack was saved at extractPaths()
        if (path_slack < 0.0) {
          minusPaths.push_back(p);
        }
        else {
          plusPaths.push_back(p);
        }
    }
    //TEST
    cout << plusPaths.size() << " paths in P+ before path select\n";
    cout << minusPaths.size() << " paths in P- before path select\n";

    // JMS-SHK begin
    // RZFF assignment based on path slack
    numFF = 0;
    for (int j = 0; j < getGateNum(); j++) {
        if (m_gates[j]->getFFFlag()) numFF++;
    }
    // reset # RZFF
    numRZ = 0;
    PathVector::iterator pitter;
    for (pitter = minusPaths.begin(); pitter != minusPaths.end(); pitter++) {
        p = *pitter;
        path_slack = p->getSlack();
        if (path_slack < 0) {
            Gate *g = p->getGate(p->getGateNum()-1);
            if (g->getFFFlag() && !(g->getRZFlag())){
                g->setRZFlag(true);
                numRZ++;
                cout << g->getName()
                     << " is assigned to RZFF. slack:"
                     << path_slack << endl;
            }
        }
    }

    // JMS-SHK end
    // initialize minus_cycles
    copy(error_cycles.begin(), error_cycles.begin()+numErrorCycles, minus_cycles.begin() );
    numMinusCycles = numErrorCycles;

  // sort paths by minimum incremental contribution to error rate
  // @NOTE: technically, after adding a path to P_-, the deltaER of each path can change if it shared unique toggle cycles with the added path (this is explored in experiment 5)
  // maybe we should try to add the set of paths with smallest union of toggle_cycles, not just individual paths greedily

  int  deltaER;
  list<Path *>::iterator jitter;
  //PathVector::iterator pitter;
  for (jitter = plusPaths.begin(); jitter != plusPaths.end(); jitter++) {
    deltaER = getDeltaErrors(*jitter);
    (*jitter)->setDeltaER(deltaER);
  }

  // sort plustPaths from minimum DeltaER
  plusPaths.sort(PathDeltaERSorterS2L());

  // now go through paths in sorted order of deltaER and add them to P_- if they fit in the error budget
  for(jitter = plusPaths.begin(); jitter != plusPaths.end(); jitter++){
    // if the path fits, add it to P_- and remove it from P_+
    if( (*jitter)->getDeltaER() + numMinusCycles < targ_errors ){
      addMinusPath(*jitter);
      // JMS-SHK begin
      // Note that adding the path to minus paths doesn't make it a negative slack path
      // It only becomes a negative slack path (maybe) after downsizing performed below
      // JMS-SHK end
      // remove this path from lists of P_+ paths for all gates in the path
    }else{
      // if this path's error contribution is too high, all the rest will also be too high (sorted order)
      // !! THIS IS NOT NECESSARILY TRUE -- IT IS ONLY TRUE IF WE RECOMPUTE deltaER AFTER ADDING A PATH !!
      // remove all the elements from plusPaths that were added to minusPaths
      break; // maybe we should comment out this break except in the version that recomputes deltaER, except the if condition prevents us from considering possible paths anyway
    }
  }

  // TEST
  cout << plusPaths.size() << " paths in P+ after path select\n";
  cout << minusPaths.size() << " paths in P- after path select\n";

  // Add plusPaths to each cell (all P_+ paths that the cell is in)
  for (jitter = plusPaths.begin(); jitter != plusPaths.end(); jitter++) {
    for (int j=0; j < (*jitter)->getGateNum(); j++) {
      Gate* g_tmp = (*jitter)->getGate(j);
      g_tmp->addPath(*jitter);
    }
  }

  // JMS-SHK begin
  // Add minusPaths to each cell (all P_- paths that the cell is in)
  // These paths will be checked later to see if we need to add any RZ-FFs
  for (pitter = minusPaths.begin(); pitter != minusPaths.end(); pitter++) {
    for (int j=0; j < (*pitter)->getGateNum(); j++) {
      Gate* g_tmp = (*pitter)->getGate(j);
      g_tmp->addMinusPath(*pitter);
    }
  }
  // JMS-SHK end

  // now, compute the sensitivity of all cells and insert downsizable cells into the set to be downsized
  Gate *g;
  double cell_sensitivity;
  list<Gate *> gate_list;
  //priority_queue<Gate *, vector<Gate *>, GateSensitivityL2S> gate_heap;  // @NOTE: need to test if this is popping the min sensitivity gate

  for (int i = 0; i < getGateNum(); i++) {
    g = m_gates[i];
    if( g->getFFFlag()||g->getCTSFlag()||!isDownsizable(g)) continue;
    if( g->getPathNum() != 0) continue;
    cell_sensitivity = computeSensitivity(g, T, false);
    g->setSensitivity(cell_sensitivity);
    gate_list.push_back(g);
  }

  //TEST
  cout << gate_list.size() << " downsizable gates\n";
  // sort the gates in order of sensitivity
  gate_list.sort(GateSensitivitySorterS2L());

  // now, continue to select the minimum sensitivity cell for downsizing and perform the downsize as long as conditions are met

  string cellInst, master, swap_cell;
  GateVector check_gates;
  bool    restore = false;
  Path *p_tmp;
  double tmpSlack;
  //double tmpSensitivity;
  //int new_lib_index;
  //TEST
  int numDownsizes=0;int numDownsizeFails=0;

  while(!gate_list.empty()){
    g = gate_list.front();

    // downsize by one size
    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);

    T->sizeCell(cellInst, swap_cell);
    // check slack of paths for this cell and fanin and fanout cells to make sure none from P_+ became negative

    restore = false;

    // Getting fanin and fanout of gate g.
    // g is gate with minimum sensitivity
    check_gates.clear();
    check_gates.push_back(g);
    for (int j = 0; j < g->getFaninGateNum(); ++j){
      check_gates.push_back(g->getFaninGate(j));
    }
    for (int k = 0; k < g->getFanoutGateNum(); ++k){
      check_gates.push_back(g->getFanoutGate(k));
    }

    //Check Timing
    for (int i = 0; i < check_gates.size(); i++) {
      for (int j = 0; j < check_gates[i]->getPathNum(); j++) {
        p_tmp = check_gates[i]->getPath(j);
        tmpSlack = T->getPathSlack(p_tmp->getPathStr());
        if ( tmpSlack < 0.0 ){
          restore = true;
          // @NOTE: we could record which paths prevent the most downsizing moves and add them to P- before adding paths based on deltaER -- this might permit more downsizing
          // if(preventingPaths.find(p_tmp) == NULLTHING?){preventingPaths[p_tmp] = 1;}
          // else{preventingPaths[p_tmp]++;}  -- a map of path to number of preventions
          break;
        }
      }
      // if restore is true, we don't have to check anymore
      if(restore){
        break;
      }
    }

    // JMS-SHK begin
    // check whether any minusPaths need a RZ-FF
    // only do this check if the cell swap has not been restored
    if(!restore){
      for (int i = 0; i < check_gates.size(); i++) {
        for (int j = 0; j < check_gates[i]->getMinusPathNum(); j++) {
          p_tmp = check_gates[i]->getMinusPath(j);
          tmpSlack = T->getPathSlack(p_tmp->getPathStr());
          if ( tmpSlack < 0.0 ){
            // change the endpoint to a RZ-FF if it isn't already
            // get the endpoint (FF) of the path
            // @@ write the code to get the endpoint of p_tmp;
            Gate * endpoint = p_tmp->getGate(p_tmp->getGateNum()-1);
            // if the endpoint is a FF but not a RZ-FF, make it a RZ-FF
            if( endpoint->getFFFlag() && !(endpoint->getRZFlag()) ){
              endpoint->setRZFlag(true);
              numRZ++;
              cout << endpoint->getName()
                   << " is assigned to RZFF. slack:"
                   << tmpSlack << endl;
            }
          }
        }
      }
    }
    // if the new RZ-FFs cause power to increase, then restore the changes
    // @@ correct this code
    // THIS CHECK IS IGNORED FOR NOW BECAUSE IT CAN MAKE RUNTIME HUGE
    //if(Power has increased because of RZ-FFs){
    //  restore = true;
    //}
    // JMS-SHK end

    if (restore) {
      //TEST
      numDownsizeFails++;
      // resotre cell swap
      T->sizeCell(cellInst, master);
      gate_list.pop_front();
    }
    else {
      //TEST -- must be removed later
      numDownsizes++;
      // check if any paths from P+ have unexpectedly entered P-

      // accept cell swap
      g->setCellName(swap_cell);
      // if the cell has been downsized to it's smallest instance, remove it from the list
      if(!isDownsizable(g)) gate_list.pop_front();

      // recompute sensitivity for the affected cells
      /*
      for (int i = 0; i < check_gates.size(); ++i) {
        if( (check_gates[i])->getFFFlag() || (check_gates[i])->getCTSFlag() ){
          continue;
        }
        tmpSensitivity = computeSensitivity(check_gates[i], T, false);
        check_gates[i]->setSensitivity(tmpSensitivity);
      }

      // re-sort in order of sensitivity
      gate_list.sort(GateSensitivitySorterS2L());
      //sort (gate_list.begin(), gate_list.end(), GateSensitivityS2L());
      */
    }
        check_gates.clear();

    }

    T->writeChange("change.tcl");
    string libNameStr = T->getLibNames();
    T->makeResizeTcl(libNameStr, "swaplist");

    cout << numDownsizes << " successful downsizes\n";
    cout << numDownsizeFails << " downsize fails\n";

    double ratioRZFF = (double) numRZ / (double) numFF;

    cout << numFF << " FFs\n";
    cout << numRZ << " RZ-FFs\n";
    cout << ratioRZFF << " Ratio of RZ-FFs\n";

}

//reducePower for SlackOptimizer
void PowerOpt::reducePowerSO(designTiming *T) {
  plusPaths.clear();

  Path *p;
  //double path_slack;
  for (int i = 0; i < toggledPaths.size(); i++)
  {
    p = toggledPaths[i];
    plusPaths.push_back(p);
  }

  // TEST
  cout << plusPaths.size() << " paths in P+ after path select\n";

  list<Path *>::iterator jitter;
  // Add plusPaths to each cell (all P_+ paths that the cell is in)
  for (jitter = plusPaths.begin(); jitter != plusPaths.end(); jitter++) {
    for (int j=0; j < (*jitter)->getGateNum(); j++) {
      Gate* g_tmp = (*jitter)->getGate(j);
      g_tmp->addPath(*jitter);
    }
  }

  // now, compute the sensitivity of all cells and insert downsizable cells into the set to be downsized
  Gate *g;
  double cell_sensitivity;
  list<Gate *> gate_list;
  //priority_queue<Gate *, vector<Gate *>, GateSensitivityL2S> gate_heap;  // @NOTE: need to test if this is popping the min sensitivity gate

  for (int i = 0; i < getGateNum(); i++) {
    g = m_gates[i];
    if( g->getFFFlag()||g->getCTSFlag()||!isDownsizable(g)) continue;
    if( g->getPathNum() != 0) continue;
    cell_sensitivity = computeSensitivity(g, T, false);
    g->setSensitivity(cell_sensitivity);
    gate_list.push_back(g);
  }

  //TEST
  cout << gate_list.size() << " downsizable gates\n";
  // sort the gates in order of sensitivity
  gate_list.sort(GateSensitivitySorterS2L());

  // now, continue to select the minimum sensitivity cell for downsizing and perform the downsize as long as conditions are met

  string cellInst, master, swap_cell;
  GateVector check_gates;
  bool    restore = false;
  Path *p_tmp;
  double tmpSlack;
  //double tmpSensitivity;
  //int new_lib_index;
  //TEST
  int numDownsizes=0;int numDownsizeFails=0;

  while(!gate_list.empty()){
    g = gate_list.front();

    // downsize by one size
    cellInst = g->getName();
    master = g->getCellName();
    swap_cell = findDownCell(master);

    T->sizeCell(cellInst, swap_cell);
    // check slack of paths for this cell and fanin and fanout cells to make sure none from P_+ became negative

    restore = false;

    // Getting fanin and fanout of gate g.
    // g is gate with minimum sensitivity
    check_gates.clear();
    check_gates.push_back(g);
    for (int j = 0; j < g->getFaninGateNum(); ++j){
      check_gates.push_back(g->getFaninGate(j));
    }
    for (int k = 0; k < g->getFanoutGateNum(); ++k){
      check_gates.push_back(g->getFanoutGate(k));
    }

    //Check Timing
    for (int i = 0; i < check_gates.size(); i++) {
      for (int j = 0; j < check_gates[i]->getPathNum(); j++) {
        p_tmp = check_gates[i]->getPath(j);
        tmpSlack = T->getPathSlack(p_tmp->getPathStr());
        if ( tmpSlack < 0.0 ){
          restore = true;
          break;
        }
      }
      // if restore is true, we don't have to check anymore
      if(restore){
        break;
      }
    }

    if (restore) {
      //TEST
      numDownsizeFails++;
      // resotre cell swap
      T->sizeCell(cellInst, master);
      gate_list.pop_front();
    }
    else {
      //TEST -- must be removed later
      numDownsizes++;
      // check if any paths from P+ have unexpectedly entered P-

      // accept cell swap
      g->setCellName(swap_cell);
      // if the cell has been downsized to it's smallest instance, remove it from the list
      if(!isDownsizable(g)) gate_list.pop_front();

    }
        check_gates.clear();

    }

    T->writeChange("change.tcl");
    string libNameStr = T->getLibNames();
    T->makeResizeTcl(libNameStr, "swaplist");

    cout << numDownsizes << " successful downsizes\n";
    cout << numDownsizeFails << " downsize fails\n";
}

int PowerOpt::getDeltaErrors(Path *p) {
     // take the set difference of the toggle cycles for the path p and the set of minus cycles
     vector<int>::iterator unique_end;
     unique_end = set_difference(p->getToggleCycles(), (p->getToggleCycles())+(p->getNumToggled()), minus_cycles.begin(), minus_cycles.begin()+numMinusCycles, delta_cycles.begin() );
     // return the number of elements in the unique set
     return int(unique_end - delta_cycles.begin());
}

void PowerOpt::addMinusPath(Path *p) {
    // append the path to the set
    minusPaths.push_back(p);
    // if this path has unique error cycles, add them to minus_cycles
    if(p->getDeltaER() > 0){
        vector<int>::iterator minus_end;
        // make a copy of minus_cycles so the union result can be placed in minus_cycles (use delta_cycles as temporary storage)
        copy(minus_cycles.begin(), minus_cycles.begin()+numMinusCycles, delta_cycles.begin() );

        // add the path's unique toggle cycles to the set of minus_cycles
        minus_end = set_union(delta_cycles.begin(), delta_cycles.begin()+numMinusCycles, p->getToggleCycles(), (p->getToggleCycles())+(p->getNumToggled()), minus_cycles.begin() );

        // update the number of minus_cycles
        numMinusCycles = int(minus_end - minus_cycles.begin());
    }
}

void PowerOpt::calER(designTiming *T) {
    int num_steps;
    vector<int> volt;

    int curVoltage = vStart;
    while(curVoltage >= vEnd){
        volt.push_back(curVoltage);
        curVoltage -= vStep;
    }
    num_steps = volt.size();
    cout << "[PowerOpt] voltage scaling steps:" << num_steps << endl;
    vector<double>  power(num_steps, DBL_MAX);
    vector<double>  leak(num_steps, DBL_MAX);
    vector<double>  razor_oh(num_steps, DBL_MAX);  // RZ overhead
    vector<double>  razor_rt(num_steps, DBL_MAX);  // RZ ratio
    vector<double>  razor_area(num_steps, DBL_MAX);  // RZ overhead
    vector<double>  hold_oh(num_steps, DBL_MAX);  // Hold buffer overhead
    vector<double>  hold_area(num_steps, DBL_MAX);  // Hold buffer overhead
    vector<double>  final_er(num_steps, DBL_MAX);
    vector<int>     num_er_ff(num_steps, 0);

    int num_targ = 8;
    double target[8] = {0, 0.00125, 0.0025, 0.005,
                        0.01, 0.02, 0.04, 0.08};
    int cur_targ = 0;
    vector<double>  power_targ(num_targ, 0);
    vector<double>  leak_targ(num_targ, 0);
    vector<double>  razor_oh_targ(num_targ, 0);  // RZ overhead
    vector<double>  razor_rt_targ(num_targ, 0);  // RZ ratio
    vector<double>  razor_area_targ(num_targ, 0);  // RZ overhead
    vector<double>  hold_oh_targ(num_targ, 0);  // Hold overhead
    vector<double>  hold_area_targ(num_targ, 0);  // Hold overhead
    vector<double>  er_targ(num_targ, 0);
    vector<int>     volt_targ(num_targ, 0);

    double error_rate = 0.0;

    numFF = 0;
    for (int j = 0; j < getGateNum(); j++) {
        if (m_gates[j]->getFFFlag()) numFF++;
    }
    double area = T->getArea();

    // voltage scaling
    for (int i = 0; i < num_steps; i++) {
        restorePTLib(volt[i], T);
        if (initSwapFile != "") T->readTcl(initSwapFile);

        ////////////////////////////////////
        // Temporarily canceling error rate calculation when error rate is already high,
        // since extCriticalPaths takes SO LONG for large number of toggled paths
        // Rationale is that when error rate is greater than 50% it's too high anyway
        // GET RID OF THIS if STATEMENT LATER!!!
        //@if(error_rate < 0.5){
        /////////////////////////////////


        // Extract Critical Paths from STA
        extCriticalPaths(T);
        error_rate = calErrorRate(totalSimCycle, T);

        // Assign RZ_FF
        numRZ = 0;
        areaRZ = 0;
        Path *p;
        for (int j = 0; j < toggledPaths.size(); j++) {
            p = toggledPaths[j];
            double path_slack = p->getSlack();
            if (path_slack < 0) {
                Gate *g = p->getGate(p->getGateNum()-1);
                if (g->getFFFlag()){
                    g->setRZFlag(true);
                }
            }

        }
        for (int j = 0; j < getGateNum(); j++) {
            if (m_gates[j]->getFFFlag() && m_gates[j]->getRZFlag()){
                numRZ++;
                areaRZ += T->getCellArea(m_gates[j]->getName()) * (double) rz_area;
            }
        }

        double ratioRZFF = (double) numRZ / (double) numFF;
        double areaRZFF = areaRZ / area;

        ///////////////////////////////////
        //@} else{
        //@  cout << "LAZY_";
        //@}
        //////////////////////////////////////

        cout << "ER_at_V" << volt[i] << " : " << error_rate;
        cout << "\t ER_Paths : " << getCriticalPathsNum() ;
        cout << "\t RZ_ratio : " << ratioRZFF << endl;


        // removed target finding code from here

        power[i] = T->getTotalPower();
        leak[i] = T->getLeakPower();
        final_er[i] = error_rate;
        razor_oh[i] = reportRZOH(T);
        razor_area[i] = areaRZFF;
        razor_rt[i] = ratioRZFF;
        hold_oh[i] = reportHBOH(T);
        hold_area[i] = areaHB / area;

        //cout << "Power at V" << volt[i] << " : " << power[i] << endl;
        ///////////////////////////////////
        // Temporarily commenting the number of error FF calculation to save time
        num_er_ff[i] = checkNumErFF();
        ///////////////////////////////////
        //cout << "# Error F/Fs at V" << volt[i] << " : " << num_er_ff[i] << endl;
    }

      for(cur_targ = 0; cur_targ < num_targ; cur_targ++){
        for (int j = 0; j < num_steps-1; j++) {
          if ( (final_er[j] <= target[cur_targ]) && (final_er[j+1] > target[cur_targ]) ){
            power_targ[cur_targ] = power[j];
            razor_oh_targ[cur_targ] = razor_oh[j];
            razor_rt_targ[cur_targ] = razor_rt[j];
            razor_area_targ[cur_targ] = razor_area[j];
            hold_oh_targ[cur_targ] = hold_oh[j];
            hold_area_targ[cur_targ] = hold_area[j];
            leak_targ[cur_targ] = leak[j];
            er_targ[cur_targ] = final_er[j];
            volt_targ[cur_targ] = volt[j];
          }
        }
      }

    ofstream fout;
    fout.open("ErrorRate.rpt");
    int min_power_volt = 0;
    double min_power = DBL_MAX;

    for (int j = 0; j < num_steps; j++) {
        if (power[j] < min_power) {
            min_power_volt = j;
            min_power = power[j];
        }
    }
    fout << "Area is : " << area << endl;
    fout << "Minimum power is : " << min_power << endl;
    fout << "V_opt with minimum power is : " << volt[min_power_volt]
         << " mV" << endl;
    fout << "Final Error Rate : " << final_er[min_power_volt] << endl;
    fout << "Number of ER FF : " << num_er_ff[min_power_volt] << endl;

    fout << "============================================" << endl;
    fout << "VOLTAGE";
    for (int i = 0; i < num_steps; i++) fout << "\t" << volt[i];
    fout << endl;
    fout << "ERROR_RATE";
    for (int i = 0; i < num_steps; i++) fout << "\t" << final_er[i];
    fout << endl;
    fout << "POWER";
    for (int i = 0; i < num_steps; i++) fout << "\t" << power[i];
    fout << endl;
    fout << "LEAK";
    for (int i = 0; i < num_steps; i++) fout << "\t" << leak[i];
    fout << endl;
    fout << "RZRT";
    for (int i = 0; i < num_steps; i++) fout << "\t" << razor_rt[i];
    fout << endl;
    fout << "RZOH";
    for (int i = 0; i < num_steps; i++) fout << "\t" << razor_oh[i];
    fout << endl;
    fout << "RZAREA";
    for (int i = 0; i < num_steps; i++) fout << "\t" << razor_area[i];
    fout << "HDOH";
    for (int i = 0; i < num_steps; i++) fout << "\t" << hold_oh[i];
    fout << endl;
    fout << "HDAREA";
    for (int i = 0; i < num_steps; i++) fout << "\t" << hold_area[i];
    fout << endl;
    fout << "TARGET";
    for (int j = 0; j < num_targ; j++) fout << "\t" << target[j];
    fout << endl;
    fout << "MINIMUM_POWER";
    for (int j = 0; j < num_targ; j++) fout << "\t" << power_targ[j];
    fout << endl;
    fout << "MINIMUM_LEAK";
    for (int j = 0; j < num_targ; j++) fout << "\t" << leak_targ[j];
    fout << endl;
    fout << "OPERATING_VOLTAGE";
    for (int j = 0; j < num_targ; j++) fout << "\t" << volt_targ[j];
    fout << endl;
    fout << "ACTUAL_ERROR_RATE";
    for (int j = 0; j < num_targ; j++) fout << "\t" << er_targ[j];
    fout << endl;
    fout << "RZRT";
    for (int j = 0; j < num_targ; j++) fout << "\t" << razor_rt_targ[j];
    fout << endl;
    fout << "RZOH";
    for (int j = 0; j < num_targ; j++) fout << "\t" << razor_oh_targ[j];
    fout << endl;
    fout << "RZAREA";
    for (int j = 0; j < num_targ; j++) fout << "\t" << razor_area_targ[j];
    fout << endl;
    fout << "HBOH";
    for (int j = 0; j < num_targ; j++) fout << "\t" << hold_oh_targ[j];
    fout << endl;
    fout << "HBAREA";
    for (int j = 0; j < num_targ; j++) fout << "\t" << hold_area_targ[j];
    fout << endl;
    fout.close();
}

// calER w/ N iterations for SSTA
void PowerOpt::calER_SSTA(designTiming *T) {

    int num_steps;
    vector<int> volt;

    int curVoltage = vStart;
    while(curVoltage >= vEnd){
        volt.push_back(curVoltage);
        curVoltage -= vStep;
    }
    num_steps = volt.size();
    cout << "[PowerOpt] voltage scaling steps:" << num_steps << endl;
    vector<double>  power(num_steps, DBL_MAX);
    vector<double>  leak(num_steps, DBL_MAX);
    vector<double>  final_er(num_steps, DBL_MAX);
    vector<int>     num_er_ff(num_steps, 0);

    int num_targ = 8;
    double target[8] = {0, 0.00125, 0.0025, 0.005,
                        0.01, 0.02, 0.04, 0.08};
    int cur_targ = 0;
    vector<double>  power_targ(num_targ, 0);
    vector<double>  leak_targ(num_targ, 0);
    vector<double>  er_targ(num_targ, 0);
    vector<int>     volt_targ(num_targ, 0);

    vector<double>  min_power_targ(num_targ, DBL_MAX);
    vector<double>  min_leak_targ(num_targ, DBL_MAX);
    vector<int>     min_volt_targ(num_targ, 120);

    vector<double>  max_power_targ(num_targ, 0);
    vector<double>  max_leak_targ(num_targ, 0);
    vector<int>     max_volt_targ(num_targ, 40);

    vector<double>  sum_power_targ(num_targ, 0);
    vector<double>  sum_leak_targ(num_targ, 0);
    vector<int>     sum_volt_targ(num_targ, 0);

    vector<double>  avg_power_targ(num_targ, 0);
    vector<double>  avg_leak_targ(num_targ, 0);
    vector<double>  avg_volt_targ(num_targ, 0);

    double error_rate = 0.0;

    // N-iteration (varIteration)
    for (int n = 0; n < varIteration; n++) {
        cout << "[PowerOpt] initVariations :" << n << endl;
        initVariations(n);
        // voltage scaling

        for (int i = 0; i < num_steps; i++) {
            restorePTLib(volt[i], T);
            if (initSwapFile != "") T->readTcl(initSwapFile);
            // Cell delay update
            updateCellDelay(T, volt[i]);

            if (varSave) {
                ofstream fout;
                char filename[250];
                sprintf(filename, "delayVar-%d-%d.rpt", n, volt[i] ) ;
                fout.open(filename);
                Gate *g;
                for (int k = 0; k < getGateNum(); k ++){
                    g = m_gates[k];
                    fout << g->getName() << "\t" << g->getCellName() << "\t" << "\t" << g->getGateDelay() << "\t" << g->getVariation() << "\t" << g->getChannelLength() << "\t" << g->getDelayVariation() << endl;

                }
                fout.close();
            }
            // Extract Critical Paths from STA
            extCriticalPaths(T);
            error_rate = calErrorRate(totalSimCycle, T);

            cout << "ER_at_V" << volt[i] << " : " << error_rate;
            cout << "\t ER_Paths : " << getCriticalPathsNum() << endl;

            power[i] = T->getTotalPower();
            leak[i] = T->getLeakPower();
            final_er[i] = error_rate;
            num_er_ff[i] = checkNumErFF();
        }

        for(cur_targ = 0; cur_targ < num_targ; cur_targ++){
            for (int j = 0; j < num_steps-1; j++) {
                if ( (final_er[j] <= target[cur_targ]) && (final_er[j+1] > target[cur_targ]) ){
                    power_targ[cur_targ] = power[j];
                    leak_targ[cur_targ] = leak[j];
                    er_targ[cur_targ] = final_er[j];
                    volt_targ[cur_targ] = volt[j];
                } else if ( (final_er[j] <= target[cur_targ]) && (j == num_steps-2) ) {
                  // additional case for when error rate at minimum voltage is not greater than target rate
                    power_targ[cur_targ] = power[j+1];
                    leak_targ[cur_targ] = leak[j+1];
                    er_targ[cur_targ] = final_er[j+1];
                    volt_targ[cur_targ] = volt[j+1];
                }
            }
        }
        for(cur_targ = 0; cur_targ < num_targ; cur_targ++){
            if (min_power_targ[cur_targ] > power_targ[cur_targ])
                min_power_targ[cur_targ] = power_targ[cur_targ];
            if (min_leak_targ[cur_targ] > leak_targ[cur_targ])
                min_leak_targ[cur_targ] = leak_targ[cur_targ];
            if (min_volt_targ[cur_targ] > volt_targ[cur_targ])
                min_volt_targ[cur_targ] = volt_targ[cur_targ];
            if (max_power_targ[cur_targ] < power_targ[cur_targ])
                max_power_targ[cur_targ] = power_targ[cur_targ];
            if (max_leak_targ[cur_targ] < leak_targ[cur_targ])
                max_leak_targ[cur_targ] = leak_targ[cur_targ];
            if (max_volt_targ[cur_targ] < volt_targ[cur_targ])
                max_volt_targ[cur_targ] = volt_targ[cur_targ];
            sum_power_targ[cur_targ] += power_targ[cur_targ];
            sum_leak_targ[cur_targ] += leak_targ[cur_targ];
            sum_volt_targ[cur_targ] += volt_targ[cur_targ];
        }
    }
    for(cur_targ = 0; cur_targ < num_targ; cur_targ++){
        avg_power_targ[cur_targ] = sum_power_targ[cur_targ] / (double) varIteration;
        avg_leak_targ[cur_targ] = sum_leak_targ[cur_targ] / (double) varIteration;
        avg_volt_targ[cur_targ] = (double) sum_volt_targ[cur_targ] / (double) varIteration;
    }
    ofstream fout;
    fout.open("ErrorRate.rpt");
    //fout << "Minimum power is : " << min_power << endl;
    //fout << "V_opt with minimum power is : " << volt[min_power_volt]
    //     << " mV" << endl;
    //fout << "Final Error Rate : " << final_er[min_power_volt] << endl;
    //fout << "Number of ER FF : " << num_er_ff[min_power_volt] << endl;

    //fout << "============================================" << endl;
    //fout << "VOLTAGE";
    //for (int i = 0; i < num_steps; i++) fout << "\t" << volt[i];
    //fout << endl;
    //fout << "ERROR_RATE";
    //for (int i = 0; i < num_steps; i++) fout << "\t" << final_er[i];
    //fout << endl;
    //fout << "POWER";
    //for (int i = 0; i < num_steps; i++) fout << "\t" << power[i];
    //fout << endl;
    //fout << "LEAK";
    //for (int i = 0; i < num_steps; i++) fout << "\t" << leak[i];
    //fout << endl;
    fout << "TARGET";
    for (int j = 0; j < num_targ; j++) fout << "\t" << target[j];
    fout << endl;
    fout << "MINIMUM_POWER_MIN";
    for (int j = 0; j < num_targ; j++) fout << "\t" << min_power_targ[j];
    fout << endl;
    fout << "MINIMUM_POWER_MAX";
    for (int j = 0; j < num_targ; j++) fout << "\t" << max_power_targ[j];
    fout << endl;
    fout << "MINIMUM_POWER_AVG";
    for (int j = 0; j < num_targ; j++) fout << "\t" << avg_power_targ[j];
    fout << endl;
    fout << "MINIMUM_LEAK_MIN";
    for (int j = 0; j < num_targ; j++) fout << "\t" << min_leak_targ[j];
    fout << endl;
    fout << "MINIMUM_LEAK_MAX";
    for (int j = 0; j < num_targ; j++) fout << "\t" << max_leak_targ[j];
    fout << endl;
    fout << "MINIMUM_LEAK_AVG";
    for (int j = 0; j < num_targ; j++) fout << "\t" << avg_leak_targ[j];
    fout << endl;
    fout << "OPERATING_VOLTAGE_MIN";
    for (int j = 0; j < num_targ; j++) fout << "\t" << min_volt_targ[j];
    fout << endl;
    fout << "OPERATING_VOLTAGE_MAX";
    for (int j = 0; j < num_targ; j++) fout << "\t" << max_volt_targ[j];
    fout << endl;
    fout << "OPERATING_VOLTAGE_AVG";
    for (int j = 0; j < num_targ; j++) fout << "\t" << avg_volt_targ[j];
    fout << endl;

    fout.close();
}

// JMS-SHK begin
// report entire RZ overhead in power
double PowerOpt::reportRZOH(designTiming *T){
    double RZ_dyn_oh = rz_dynamic;
    double RZ_sta_oh = rz_static;
    double sum_dyn_oh = 0;
    double sum_sta_oh = 0;
    //double sum_hold_oh = 0;
    string cellInst;
    double cellDyn, cellLeak;

    for (int i = 0; i < getGateNum(); i++) {
        if (m_gates[i]->getRZFlag()) {
            cellInst = m_gates[i]->getName();
            cellLeak = T->getCellLeak(cellInst);
            cellDyn = T->getCellPower(cellInst) - cellLeak;
            sum_sta_oh += cellLeak * RZ_sta_oh;
            sum_dyn_oh += cellDyn * RZ_dyn_oh;
        }
    }
    //cout << "RZ " << sum_sta_oh + sum_dyn_oh << endl;
    return (sum_sta_oh + sum_dyn_oh);
}

double PowerOpt::reportHBOH(designTiming *T){ // Hold buffer OH
    // find hold buffer delay and power
    double hold_delay_sum = 0;
    double hold_power_sum = 0;
    int hold_num = 0;
    areaHB = 0;
    string cellInst, master;

    for (int i = 0; i < getGateNum(); i++) {
        master = m_gates[i]->getCellName();
        cellInst = m_gates[i]->getName();
        if (master == "BUFFD0"){
            hold_delay_sum += T->getCellDelay(cellInst);
            hold_power_sum += T->getCellPower(cellInst);
            hold_num ++;
        }
    }
    double hold_delay = hold_delay_sum / (double) hold_num;
    double hold_power = hold_power_sum / (double) hold_num;

    double hold_overhead = 0;

    for (int i = 0; i < getGateNum(); i++) {
        if (m_gates[i]->getRZFlag()) {
            cellInst = m_gates[i]->getName();
            double slack = T->getFFMinSlack(cellInst);
            if (slack < half_clk) {
                double diff = (double) half_clk - slack;
                hold_overhead += diff / hold_delay * hold_power;
                areaHB += diff / hold_delay * (double) buffer_area;
            }
        }
    }
    //cout << "hold delay " << hold_delay << " hold_power " << hold_power << " HDOH " << hold_overhead << endl;
    return hold_overhead;
}
// JMS-SHK end

void PowerOpt::printSlackDist(designTiming *T) {
    Path *p;
    double path_slack;
    ofstream slackfile;
    slackfile.open("slackDist.rpt");

    slackfile << "path_slack" << "\t" << "toggle_rate" << endl;
    double t0 = 0.0;
    TICK();
    for (int i = 0; i < toggledPaths.size(); i++)
    {
        p = toggledPaths[i];
        path_slack = T->getPathSlack(p->getPathStr());
        p->setSlack(path_slack);
        slackfile << path_slack << "\t" << (double) p->getNumToggled() / (double) totalSimCycle << "\t" << p->getPathStr() << endl;
    }
    TOCK(path_time);
    slackfile.close();
}

void PowerOpt::print_processor_state_profile(int cycle_num)
{
  processor_state_profile_file <<  "PRINTING PROFILE FOR CYCLE " << cycle_num << endl;
  for (int i = 0; i < terms.size(); i++)
  {
    Terminal * term = terms[i];
    processor_state_profile_file << term->getFullName() << ":" << term->getSimValue() << endl ;
  }
  for (int i = 0; i < m_pads.size(); i++)
  {
    Pad *pad  = m_pads[i];
    processor_state_profile_file << pad->getName() << ":" << pad->getSimValue() << endl ;
  }
//  list<GNode*>::iterator it = nodes_topo.begin();
//  for (it = nodes_topo.begin(); it != nodes_topo.end(); it ++)
//  {
//     GNode* node = *it;
//     processor_state_profile_file << node->getName() << " : " << node->getSimValue() << endl;
//  }

}

void PowerOpt::print_dmem_contents(int cycle_num)
{
  dmem_contents_file <<  "PRINTING DMEM CONTENTS AT CYCLE " << cycle_num << endl;
  map<int, bitset<16> >::iterator it;
  for (it = DMemory.begin(); it != DMemory.end(); it ++)
  {
     int address = it->first;
     bitset<16> value = it->second;
     dmem_contents_file << address << ":" << value << endl;
  }

}

void PowerOpt::print_fanin_cone()
{
  for (int i = 0; i < getGateNum(); i ++)
  {
    Gate* g = m_gates[i];
    //cout << i << "(" << g->getName() << ")" << endl;
    //if (g->getFaninGateNum() < g->getFaninTerminalNum())
    {
       fanins_file << i << "(" << g->getName() << ")  (" << g->getCellName() << ") " << endl;
       for (int l = 0; l < g->getInputSubnetNum(); ++l)
       {
          Subnet * subnet = g->getInputSubnet(l);
          Terminal * op_terminal = subnet->getOutputTerminal();
          if (!subnet->inputIsPad()) {
             Terminal * inp_terminal = subnet->getInputTerminal();
             fanins_file << " . \t  Gate is -->" << inp_terminal->getGate()->getName() << " (" << op_terminal->getName()  << ") " <<  endl;
          }
          else {
             fanins_file <<  " . \t  Input Pad is --> " << subnet->getInputPad()->getName() << " (" << op_terminal->getName() << ") " << endl;
          }
       }
    }
//    for (int l = 0; l < g->getFaninGateNum(); ++l)
//    {
//      cout << "\t-->" << g->getFaninGate(l)->getId() << endl;
//    }
  }

/*  for (int i = 0; i < getTerminalNum(); i ++)
  {
     Terminal* term = getTerminal(i);
     cout << " Terminal :  "   << term->getName() << endl;

  }*/

/*  for(int i =0; i < getPadNum(); i ++)
  {
//    if (getPad(i)->getType() == PrimiaryInput)
//    cout << "Pad: " << getPad(i)->getName() << endl;
      //if (getPad(i)->getName() == "aclk")
      {
        cout << getPad(i)->getName();
        for(int j =0;  j < getPad(i)->getNetNum(); j++)
        {
          cout << " --> " <<  getPad(i)->getNet(j)->getName();
        }
        cout << endl;
      }
  }*/
/*  for(int i =0; i < getGateNum(); i ++)
  {
//    if (getPad(i)->getType() == PrimiaryInput)
//    cout << "Pad: " << getPad(i)->getName() << endl;
      //if (getPad(i)->getName() == "aclk")
      {
        cout << getGate(i)->getName() << " ( " << getGate(i)->getFanoutTerminalNum() << " ) ";
        for(int j =0;  j < getGate(i)->getFanoutTerminalNum(); j++)
        {
          Terminal* term = getGate(i)->getFanoutTerminal(j);
            for (int k = 0 ; k < term->getNetNum(); k++)
            {
                cout << " --> " << term->getNet(k)->getName();
            }
          //cout << " --> " <<  getGate(i)->getFanoutTerminal(j)->getName();
        }
        cout << endl;
      }
  }*/
}

void PowerOpt::populateGraphDatabase(Graph* graph)
{
    for (int i = 0; i < getPadNum(); i++) // PADS
    {
       Pad* pad = getPad(i);
       GNode* node  = (GNode*) malloc(sizeof(GNode));
       node->setIsPad(true);
       node->setIsGate(false);
       node->setVisited(false);
       if (pad->getType() == PrimiaryInput)
       {
         node->setPad(pad);
         node->setIsSource(true);
         graph->addSource(node);
         graph->addWfNode(node);
         graph->addNode(node);
       }
       else if (pad->getType() == PrimiaryOutput)
       {
         node->setPad(pad);
         node->setIsSink(true);
         graph->addSink(node);
         //graph->addWfNode(node);
         graph->addNode(node);
       }
       else if (pad->getType() == InputOutput)
       {
         node->setPad(pad);
         node->setIsSource(true);
         node->setIsSink(true);
         graph->addSource(node);
         graph->addWfNode(node);
         graph->addSink(node);
         graph->addNode(node);
       }
       else { assert(0) ;}
       pad->setGNode(node);
    }
    for (int i = 0; i < getGateNum(); i ++) // GATES
    {
      Gate* gate = m_gates[i];
      //if (gate->getIsClkTree()) continue;
      GNode * node = (GNode*) malloc(sizeof(GNode));
      node->setIsGate(true);
      node->setIsPad(false);
      node->setVisited(false);
      if (gate->getFFFlag())  // A FF is a source and sink
      {
        node->setGate(gate);
        node->setIsSource(true);
        node->setIsSink(true);
        graph->addSource(node);
        graph->addWfNode(node);
        graph->addSink(node);
        graph->addNode(node);
      }
      else  // Other gates are nodes
      {
        node->setGate(gate);
        node->setIsSink(false);
        node->setIsSource(false);
        //node->etIsNode(true);
        graph->addNode(node);
      }
      gate->setGNode(node);
    }

}

void PowerOpt::computeNetExpressions()
{
  list<GNode*>::iterator it = nodes_topo.begin();
  for (it = nodes_topo.begin(); it != nodes_topo.end(); it ++)
  {
    GNode* node = *it;
    cout << " Current Node is : " << node->getName() << endl;
    if (node->getIsPad() == true)
    {
        Pad* pad = node->getPad();
        pad->setExpr();
    }
    else //if (node->getIsFF() == true)
    {
        Gate* gate = node->getGate();
        gate->computeNetExpr();
    }
  }
/*  cout << " Computing Expression" << endl;
  for (int i = 0; i < m_pads.size(); i++)
  {
    Pad* pad = m_pads[i];
    pad->setExpr();
  }

  for (int i = 0; i < m_gates.size(); i++)
  {
    Gate * gate = m_gates[i];
    if (gate->getFFFlag())
    {
       gate->computeNetExpr();
       //debug_file << "Gate is " << gate->getName() << endl;
    }
    else continue;
  }
  //debug_file << " **************************" << endl;

  cout << " Finished marking FF nets" << endl;

  //for (int i = m_gates_topo.size() -1; i >= 0; i--)
  for (int i = 0; i < m_gates_topo.size() ; i++)
  {
    Gate* gate = m_gates_topo[i];
    gate->computeNetExpr();
  }*/
}

void PowerOpt::readDmemInitFile()
{
  ifstream dmem_init_file;
  dmem_init_file.open(dmemInitFile.c_str());
  cout << "Reading dmem_init_file" << endl;
  if (dmem_init_file.is_open())
  {
    string line;
    while (getline(dmem_init_file, line))
    {
      trim(line);
      vector<string> tokens;
      tokenize(line, ':', tokens);
      assert(tokens.size() == 2);
      int addr = atoi(tokens[0].c_str());// term or pad name
      string value = tokens[1];
      bitset<16> value_bs(value);
      DMemory.insert(make_pair(addr, value_bs));
    }
  }

}

void PowerOpt::readSimInitFile()
{
  ifstream sim_init_file;
  sim_init_file.open(simInitFile.c_str());
  cout << "Reading sim_init_file" << endl;
  if (sim_init_file.is_open())
  {
    string line;
    while (getline(sim_init_file, line))
    {
      trim(line);
      vector<string> tokens;
      tokenize(line, ':', tokens);
      //assert(tokens.size() == 2);
      string name = tokens[0];// term or pad name
      string value;
      if (tokens.size() == 2) value = tokens[1];
      // apply to the terminal
      map<string, int>::iterator it = terminalNameIdMap.find(name);
      if (it != terminalNameIdMap.end())
      {
        Terminal* term = terms[it->second];
        term->setSimValue(value, sim_wf);
      }
      else //
      {
        it = padNameIdMap.find(name);
        Pad* pad = m_pads[it->second];
        pad->setSimValue(value, sim_wf);
      }
    }
  }
}

void PowerOpt::initialize_sim_wf()
{
  vector<GNode*> sources = graph->getSources();
  vector<GNode*>::iterator it = sources.begin();
  for( ; it!= sources.end(); it++)
  {
    GNode* source = *it;
    sim_wf.push(source);
  }
}

void PowerOpt::clearSimVisited()
{
  while(! sim_visited.empty())
  {
    GNode* node = sim_visited.top(); sim_visited.pop();
    node->setVisited(false);
  }
}

void PowerOpt::runSimulation(bool wavefront)
{
  static int cycle_num = 1;
  cout << "Simulating cycle " << cycle_num << endl;
  cycle_num ++;
  Pad* dco_clk_pad = m_pads[padNameIdMap["dco_clk"]];
  string dco_clk_value = dco_clk_pad->getSimValue();
  if (dco_clk_value == "1") dco_clk_pad-> setSimValue("0", sim_wf);
  else if (dco_clk_value == "0") dco_clk_pad-> setSimValue("1", sim_wf);
  else { cout << " dco_clk : " <<  dco_clk_value << endl; assert(0); }
  clearSimVisited();
  //initialize_sim_wf();
  bool once = false;
  if(wavefront)
  {
    while (!sim_wf.empty())
    {
      GNode* node = sim_wf.top(); sim_wf.pop();
/*      if (!once)
      {
        //cout << " * " << endl;
        once = true;
      }*/
      if ((node->getIsPad() == false) && (node->getIsSink() == false)) // Nothing needed for pads
      {
        // Deadcode
        if (node->getVisited() == true) continue;
        node->setVisited(true);
        // Deadcode
        sim_visited.push(node);
        Gate* gate = node->getGate();
        gate->computeVal(sim_wf); // here we push gates into sim_wf
        //debug_file << " Node : " << node->getName() << " : " <<  node->getTopoId() << endl;
      }
//      else 
//      {
//        debug_file << "Missed" << node->getName() << endl;
//      }
    }
  }
  else
  {
    list<GNode*>::iterator it = nodes_topo.begin();
    for (it = nodes_topo.begin(); it != nodes_topo.end(); it++)
    {
      GNode* node = *it;
      if (node->getIsGate() == true) // Nothing needed for pads
      {
        Gate* gate = node->getGate();
        gate->computeVal(sim_wf);
      }
    }
  }
}

void PowerOpt::printRegValues()
{
    list<Gate*>::iterator it;
    for(it = m_regs.begin(); it != m_regs.end(); it++)
    {
        Gate* ff_gate = *it;
        cout << ff_gate->getName() << ":" << ff_gate->getSimValue() << ", " ;
    }
    cout << endl;
}

void PowerOpt::readSelectGatesFile()
{
    ifstream select_gates_file;
    select_gates_file.open(dbgSelectGatesFile.c_str());
    if (select_gates_file.is_open())
    {
        cout << "Reading Select Gates File" << endl;
        string line;
        while(getline(select_gates_file, line))
        {
            trim(line);
            dbgSelectGates.push_back(line);
        }
    }

}

void PowerOpt::readConstantTerminals()
{
  ifstream one_pins_file, zero_pins_file; 
  one_pins_file.open("one_pins");
  zero_pins_file.open("zero_pins");
  string line;
  if (one_pins_file.is_open())
  {
    while (getline(one_pins_file, line))
    {
      trim(line);
      vector<string> tokens; 
      tokenize(line, ':', tokens);
      string gate_name = tokens[0];
      string term_name = tokens[1];

      int id = gateNameIdMap[gate_name];
      Gate * gate = m_gates[id];
      int num_terms = terms.size();

      Terminal* term = new Terminal(num_terms+1, term_name);
      Net *n = new Net(nets.size()+1, term_name); // set the net name to be same as term name

      term->setGate(gate);
      term->computeFullName();
      term->addNet(n);
      term->setSimValue("1");

      addTerminal(term);
      addNet(n);

      string name = term->getFullName();
      if (terminalNameIdMap.find(name) != terminalNameIdMap.end()) assert(0);
      gate->addFaninTerminal(term);
    }
  }
  if (zero_pins_file.is_open())
  {
    while (getline(zero_pins_file, line))
    {
      trim(line);
      vector<string> tokens; 
      tokenize(line, ':', tokens);
      string gate_name = tokens[0];
      string term_name = tokens[1];

      int id = gateNameIdMap[gate_name];
      Gate * gate = m_gates[id]; 
      int num_terms = terms.size();
      Terminal* term = new Terminal(num_terms+1, term_name);
      Net *n = new Net(nets.size()+1, term_name); // set the net name to be same as term name

      term->setGate(gate);
      term->computeFullName();
      term->addNet(n);
      term->setSimValue("0");

      addTerminal(term);
      addNet(n);

      string name = term->getFullName();
      if (terminalNameIdMap.find(name) != terminalNameIdMap.end()) assert(0);
      gate->addFaninTerminal(term);
    }
  }

}

void PowerOpt::printSelectGateValues()
{
    for (int i = 0; i < dbgSelectGates.size(); i++)
    {
       Gate* gate = m_gates[gateNameIdMap[dbgSelectGates[i]]];
       debug_file << gate->getName() << " : " << gate->getSimValue() << ", ";
    }
    debug_file << endl;
}

void PowerOpt::updateRegOutputs()
{
  Pad* dco_clk_pad = m_pads[padNameIdMap["dco_clk"]];
  string dco_clk_value = dco_clk_pad->getSimValue();
  if (dco_clk_value != "1") return;
  //cout << " Updating Reg Outputs " << endl;
  list<Gate*>::iterator it;
  for(it = m_regs.begin(); it != m_regs.end(); it++)
  {
    Gate* ff_gate = *it;
    ff_gate->transferDtoQ(sim_wf);
  }
}

void PowerOpt::readPmemFile()
{
  FILE * pmem_file = fopen(pmem_file_name.c_str(), "r");
  if(pmem_file != NULL)
  {
     string line;
     unsigned int hex;
     cout << "Reading PMEM ..." ;
     while ((fscanf(pmem_file, "%x", &hex) != EOF)) {
       PMemory.push_back(hex);
     }
     cout << " done" << endl;
  }
}

string PowerOpt::getPmemAddr()
{
	string pmem_addr_13_val = m_pads[padNameIdMap["pmem_addr\\[13\\]"]]->getSimValue();
	string pmem_addr_12_val = m_pads[padNameIdMap["pmem_addr\\[12\\]"]]->getSimValue();
	string pmem_addr_11_val = m_pads[padNameIdMap["pmem_addr\\[11\\]"]]->getSimValue();
	string pmem_addr_10_val = m_pads[padNameIdMap["pmem_addr\\[10\\]"]]->getSimValue();
	string pmem_addr_9_val  = m_pads[padNameIdMap["pmem_addr\\[9\\]"]]->getSimValue();
	string pmem_addr_8_val  = m_pads[padNameIdMap["pmem_addr\\[8\\]"]]->getSimValue();
	string pmem_addr_7_val  = m_pads[padNameIdMap["pmem_addr\\[7\\]"]]->getSimValue();
	string pmem_addr_6_val  = m_pads[padNameIdMap["pmem_addr\\[6\\]"]]->getSimValue();
	string pmem_addr_5_val  = m_pads[padNameIdMap["pmem_addr\\[5\\]"]]->getSimValue();
	string pmem_addr_4_val  = m_pads[padNameIdMap["pmem_addr\\[4\\]"]]->getSimValue();
	string pmem_addr_3_val  = m_pads[padNameIdMap["pmem_addr\\[3\\]"]]->getSimValue();
	string pmem_addr_2_val  = m_pads[padNameIdMap["pmem_addr\\[2\\]"]]->getSimValue();
	string pmem_addr_1_val  = m_pads[padNameIdMap["pmem_addr\\[1\\]"]]->getSimValue();
	string pmem_addr_0_val  = m_pads[padNameIdMap["pmem_addr\\[0\\]"]]->getSimValue();

    string pmem_addr_str = pmem_addr_13_val+ pmem_addr_12_val+ pmem_addr_11_val+ pmem_addr_10_val+ pmem_addr_9_val + pmem_addr_8_val + pmem_addr_7_val + pmem_addr_6_val + pmem_addr_5_val + pmem_addr_4_val + pmem_addr_3_val + pmem_addr_2_val + pmem_addr_1_val + pmem_addr_0_val ;
    return pmem_addr_str;
}

string PowerOpt::getDmemAddr()
{
	string dmem_addr_12_val = m_pads[padNameIdMap["dmem_addr\\[12\\]"]]->getSimValue();
	string dmem_addr_11_val = m_pads[padNameIdMap["dmem_addr\\[11\\]"]]->getSimValue();
	string dmem_addr_10_val = m_pads[padNameIdMap["dmem_addr\\[10\\]"]]->getSimValue();
	string dmem_addr_9_val  = m_pads[padNameIdMap["dmem_addr\\[9\\]"]]->getSimValue();
	string dmem_addr_8_val  = m_pads[padNameIdMap["dmem_addr\\[8\\]"]]->getSimValue();
	string dmem_addr_7_val  = m_pads[padNameIdMap["dmem_addr\\[7\\]"]]->getSimValue();
	string dmem_addr_6_val  = m_pads[padNameIdMap["dmem_addr\\[6\\]"]]->getSimValue();
	string dmem_addr_5_val  = m_pads[padNameIdMap["dmem_addr\\[5\\]"]]->getSimValue();
	string dmem_addr_4_val  = m_pads[padNameIdMap["dmem_addr\\[4\\]"]]->getSimValue();
	string dmem_addr_3_val  = m_pads[padNameIdMap["dmem_addr\\[3\\]"]]->getSimValue();
	string dmem_addr_2_val  = m_pads[padNameIdMap["dmem_addr\\[2\\]"]]->getSimValue();
	string dmem_addr_1_val  = m_pads[padNameIdMap["dmem_addr\\[1\\]"]]->getSimValue();
	string dmem_addr_0_val  = m_pads[padNameIdMap["dmem_addr\\[0\\]"]]->getSimValue();

    string dmem_addr_str =  dmem_addr_12_val+ dmem_addr_11_val+ dmem_addr_10_val+ dmem_addr_9_val + dmem_addr_8_val + dmem_addr_7_val + dmem_addr_6_val + dmem_addr_5_val + dmem_addr_4_val + dmem_addr_3_val + dmem_addr_2_val + dmem_addr_1_val + dmem_addr_0_val ;
    return dmem_addr_str;
}

string PowerOpt::getDmemDin()
{
	string dmem_din_15_val = m_pads[padNameIdMap["dmem_din\\[15\\]"]]->getSimValue();
	string dmem_din_14_val = m_pads[padNameIdMap["dmem_din\\[14\\]"]]->getSimValue();
	string dmem_din_13_val = m_pads[padNameIdMap["dmem_din\\[13\\]"]]->getSimValue();
	string dmem_din_12_val = m_pads[padNameIdMap["dmem_din\\[12\\]"]]->getSimValue();
	string dmem_din_11_val = m_pads[padNameIdMap["dmem_din\\[11\\]"]]->getSimValue();
	string dmem_din_10_val = m_pads[padNameIdMap["dmem_din\\[10\\]"]]->getSimValue();
	string dmem_din_9_val  = m_pads[padNameIdMap["dmem_din\\[9\\]"]]->getSimValue();
	string dmem_din_8_val  = m_pads[padNameIdMap["dmem_din\\[8\\]"]]->getSimValue();
	string dmem_din_7_val  = m_pads[padNameIdMap["dmem_din\\[7\\]"]]->getSimValue();
	string dmem_din_6_val  = m_pads[padNameIdMap["dmem_din\\[6\\]"]]->getSimValue();
	string dmem_din_5_val  = m_pads[padNameIdMap["dmem_din\\[5\\]"]]->getSimValue();
	string dmem_din_4_val  = m_pads[padNameIdMap["dmem_din\\[4\\]"]]->getSimValue();
	string dmem_din_3_val  = m_pads[padNameIdMap["dmem_din\\[3\\]"]]->getSimValue();
	string dmem_din_2_val  = m_pads[padNameIdMap["dmem_din\\[2\\]"]]->getSimValue();
	string dmem_din_1_val  = m_pads[padNameIdMap["dmem_din\\[1\\]"]]->getSimValue();
	string dmem_din_0_val  = m_pads[padNameIdMap["dmem_din\\[0\\]"]]->getSimValue();

  string dmem_din_str = dmem_din_15_val+ dmem_din_14_val+ dmem_din_13_val+ dmem_din_12_val+ dmem_din_11_val+ dmem_din_10_val+ dmem_din_9_val + dmem_din_8_val + dmem_din_7_val + dmem_din_6_val + dmem_din_5_val + dmem_din_4_val + dmem_din_3_val + dmem_din_2_val + dmem_din_1_val + dmem_din_0_val ;
    //string dmem_din_str = dmem_din_0_val+ dmem_din_1_val+ dmem_din_2_val+ dmem_din_3_val+ dmem_din_4_val+ dmem_din_5_val+ dmem_din_6_val+ dmem_din_7_val+ dmem_din_8_val+ dmem_din_9_val+ dmem_din_10_val+ dmem_din_11_val+ dmem_din_12_val+ dmem_din_13_val+ dmem_din_14_val+ dmem_din_15_val;

    return dmem_din_str;
}

string PowerOpt::getDmemLow()
{
	string dmem_din_7_val  = m_pads[padNameIdMap["dmem_din\\[7\\]"]]->getSimValue();
	string dmem_din_6_val  = m_pads[padNameIdMap["dmem_din\\[6\\]"]]->getSimValue();
	string dmem_din_5_val  = m_pads[padNameIdMap["dmem_din\\[5\\]"]]->getSimValue();
	string dmem_din_4_val  = m_pads[padNameIdMap["dmem_din\\[4\\]"]]->getSimValue();
	string dmem_din_3_val  = m_pads[padNameIdMap["dmem_din\\[3\\]"]]->getSimValue();
	string dmem_din_2_val  = m_pads[padNameIdMap["dmem_din\\[2\\]"]]->getSimValue();
	string dmem_din_1_val  = m_pads[padNameIdMap["dmem_din\\[1\\]"]]->getSimValue();
	string dmem_din_0_val  = m_pads[padNameIdMap["dmem_din\\[0\\]"]]->getSimValue();

    string dmem_din_str = dmem_din_7_val + dmem_din_6_val + dmem_din_5_val + dmem_din_4_val + dmem_din_3_val + dmem_din_2_val + dmem_din_1_val + dmem_din_0_val ;
    return dmem_din_str;
}

string PowerOpt::getDmemHigh()
{
	string dmem_din_15_val = m_pads[padNameIdMap["dmem_din\\[15\\]"]]->getSimValue();
	string dmem_din_14_val = m_pads[padNameIdMap["dmem_din\\[14\\]"]]->getSimValue();
	string dmem_din_13_val = m_pads[padNameIdMap["dmem_din\\[13\\]"]]->getSimValue();
	string dmem_din_12_val = m_pads[padNameIdMap["dmem_din\\[12\\]"]]->getSimValue();
	string dmem_din_11_val = m_pads[padNameIdMap["dmem_din\\[11\\]"]]->getSimValue();
	string dmem_din_10_val = m_pads[padNameIdMap["dmem_din\\[10\\]"]]->getSimValue();
	string dmem_din_9_val  = m_pads[padNameIdMap["dmem_din\\[9\\]"]]->getSimValue();
	string dmem_din_8_val  = m_pads[padNameIdMap["dmem_din\\[8\\]"]]->getSimValue();

    string dmem_din_str = dmem_din_15_val+ dmem_din_14_val+ dmem_din_13_val+ dmem_din_12_val+ dmem_din_11_val+ dmem_din_10_val+ dmem_din_9_val + dmem_din_8_val;
    return dmem_din_str;
}

void PowerOpt::handleDmem(int cycle_num, bool wavefront)
{
     string dmem_cen = m_pads[padNameIdMap["dmem_cen"]]->getSimValue();
     if (dmem_cen != "0") return;
     string dmem_wen_0 = m_pads[padNameIdMap["dmem_wen\\[0\\]"]]->getSimValue();
     string dmem_wen_1 = m_pads[padNameIdMap["dmem_wen\\[1\\]"]]->getSimValue();
     string dmem_wen = dmem_wen_1 + dmem_wen_0;
     unsigned int dmem_wen_int = strtoull(dmem_wen.c_str(), NULL, 2);
     string dmem_addr_str = getDmemAddr();
     unsigned int dmem_addr = strtoull(dmem_addr_str.c_str(), NULL, 2);
     string dmem_din_str = getDmemDin();
     bitset<16> dmem_din(dmem_din_str);
     bitset<16> Dmem_lower;
     bitset<16> Dmem_higher;
     dmem_request_file << " Accessing Dmem with address : " << hex << dmem_addr << dec << " and write mode  " << dmem_wen_int << ". Size of Memory is " << DMemory.size()  <<  " And cycle num is " << cycle_num << endl;
     pmem_request_file << " Accessing Dmem with address : " << hex << dmem_addr << dec << " and write mode  " << dmem_wen_int << ". Size of Memory is " << DMemory.size()  <<  " And cycle num is " << cycle_num << endl;


     switch (dmem_wen_int)
     {
       case 0:  // WRITE BOTH BYTES 
         DMemory[dmem_addr] = dmem_din;
         dmem_request_file << "Writing Data (full) " << dmem_din_str << endl;
         break;
       case 1: // WRITE HIGH BYTE
         Dmem_lower = DMemory[dmem_addr] & bitset<16> (255);// 11111111
         DMemory[dmem_addr] = Dmem_lower | (dmem_din & bitset<16>(65280) );// 1111111100000000
         dmem_request_file << "Writing Data (high) " << dmem_din_str << endl;
         break;
       case 2: // WRITE LOW BYTE
         Dmem_higher = DMemory[dmem_addr] & bitset<16> (65280);
         DMemory[dmem_addr] = Dmem_higher | (dmem_din & bitset<16>(255) );
         dmem_request_file << "Writing Data (low) " << dmem_din_str << endl;
         break;
       case 3: // READ
         //sendData(DMemory[dmem_addr].to_string());
         dmem_data = DMemory[dmem_addr].to_string();
         send_data = true;
         break;
     }

}

bool PowerOpt::sendInputs()
{
  static int i = 0; 
	string per_enable_val  = m_pads[padNameIdMap["per_en"]]->getSimValue();
  if (per_enable_val != "1") return false;

	string per_we_1_val  = m_pads[padNameIdMap["per_we\\[1\\]"]]->getSimValue();
	string per_we_0_val  = m_pads[padNameIdMap["per_we\\[0\\]"]]->getSimValue();

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "00") return false;

  string per_addr_13_val = m_pads[padNameIdMap["per_addr\\[13\\]"]]->getSimValue();
  string per_addr_12_val = m_pads[padNameIdMap["per_addr\\[12\\]"]]->getSimValue();
  string per_addr_11_val = m_pads[padNameIdMap["per_addr\\[11\\]"]]->getSimValue();
  string per_addr_10_val = m_pads[padNameIdMap["per_addr\\[10\\]"]]->getSimValue();
  string per_addr_9_val  = m_pads[padNameIdMap["per_addr\\[9\\]"]]->getSimValue();
  string per_addr_8_val  = m_pads[padNameIdMap["per_addr\\[8\\]"]]->getSimValue();
  string per_addr_7_val  = m_pads[padNameIdMap["per_addr\\[7\\]"]]->getSimValue();
  string per_addr_6_val  = m_pads[padNameIdMap["per_addr\\[6\\]"]]->getSimValue();
  string per_addr_5_val  = m_pads[padNameIdMap["per_addr\\[5\\]"]]->getSimValue();
  string per_addr_4_val  = m_pads[padNameIdMap["per_addr\\[4\\]"]]->getSimValue();
  string per_addr_3_val  = m_pads[padNameIdMap["per_addr\\[3\\]"]]->getSimValue();
  string per_addr_2_val  = m_pads[padNameIdMap["per_addr\\[2\\]"]]->getSimValue();
  string per_addr_1_val  = m_pads[padNameIdMap["per_addr\\[1\\]"]]->getSimValue();
  string per_addr_0_val  = m_pads[padNameIdMap["per_addr\\[0\\]"]]->getSimValue();

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000010000") return false; // per_addrs is 0010
  string data_str; 
  //cout << " Sending Inputs for i " << i << endl;
  //pmem_request_file << " Sending Inputs for i " << i << endl;
  switch (i)
  {
    case 0:
            //data_str = "0000000001001101"; 
            data_str = "1000000000000000"; 
            //data_str = "XXXXXXXX00000000"; 
            sendPerDout(data_str);
            pmem_request_file << "In Case 0 " << endl;
            break;
    case 1:
            data_str = "0000000000000000"; 
            //data_str = "XXXXXXXX00000000"; 
            sendPerDout(data_str);
            pmem_request_file << "In Case 1 " << endl;
            break;
//    case 2:
//            data_str = "1100000000000000"; 
//            //data_str = "XXXXXXXX00000000"; 
//            sendPerDout(data_str);
//            pmem_request_file << "In Case 2 " << endl;
//            break;
//    case 3:
//            data_str = "0000000000000000"; 
//            //data_str = "XXXXXXXX00000000"; 
//            sendPerDout(data_str);
//            pmem_request_file << "In Case 3 " << endl;
//            break;
    default : break;
  }
  i++;
  if (i == 2) return true;
  return false;
}

void PowerOpt::recvInputs1(int cycle_num, bool wavefront)
{
	string per_enable_val  = m_pads[padNameIdMap["per_en"]]->getSimValue();
  if (per_enable_val != "1") return ;

	string per_we_1_val  = m_pads[padNameIdMap["per_we\\[1\\]"]]->getSimValue();
	string per_we_0_val  = m_pads[padNameIdMap["per_we\\[0\\]"]]->getSimValue();

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return ;

  string per_addr_13_val = m_pads[padNameIdMap["per_addr\\[13\\]"]]->getSimValue();
  string per_addr_12_val = m_pads[padNameIdMap["per_addr\\[12\\]"]]->getSimValue();
  string per_addr_11_val = m_pads[padNameIdMap["per_addr\\[11\\]"]]->getSimValue();
  string per_addr_10_val = m_pads[padNameIdMap["per_addr\\[10\\]"]]->getSimValue();
  string per_addr_9_val  = m_pads[padNameIdMap["per_addr\\[9\\]"]]->getSimValue();
  string per_addr_8_val  = m_pads[padNameIdMap["per_addr\\[8\\]"]]->getSimValue();
  string per_addr_7_val  = m_pads[padNameIdMap["per_addr\\[7\\]"]]->getSimValue();
  string per_addr_6_val  = m_pads[padNameIdMap["per_addr\\[6\\]"]]->getSimValue();
  string per_addr_5_val  = m_pads[padNameIdMap["per_addr\\[5\\]"]]->getSimValue();
  string per_addr_4_val  = m_pads[padNameIdMap["per_addr\\[4\\]"]]->getSimValue();
  string per_addr_3_val  = m_pads[padNameIdMap["per_addr\\[3\\]"]]->getSimValue();
  string per_addr_2_val  = m_pads[padNameIdMap["per_addr\\[2\\]"]]->getSimValue();
  string per_addr_1_val  = m_pads[padNameIdMap["per_addr\\[1\\]"]]->getSimValue();
  string per_addr_0_val  = m_pads[padNameIdMap["per_addr\\[0\\]"]]->getSimValue();

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000011000") return ; // per_addrs is 0010
  
  recv_inputs = true;
}

void PowerOpt::recvInputs2(int cycle_num, bool wavefront)
{
  if (!recv_inputs) return;
  string per_din_15_val = m_pads[padNameIdMap["per_din\\[15\\]"]]->getSimValue();
  string per_din_14_val = m_pads[padNameIdMap["per_din\\[14\\]"]]->getSimValue();
  string per_din_13_val = m_pads[padNameIdMap["per_din\\[13\\]"]]->getSimValue();
  string per_din_12_val = m_pads[padNameIdMap["per_din\\[12\\]"]]->getSimValue();
  string per_din_11_val = m_pads[padNameIdMap["per_din\\[11\\]"]]->getSimValue();
  string per_din_10_val = m_pads[padNameIdMap["per_din\\[10\\]"]]->getSimValue();
  string per_din_9_val  = m_pads[padNameIdMap["per_din\\[9\\]"]]->getSimValue();
  string per_din_8_val  = m_pads[padNameIdMap["per_din\\[8\\]"]]->getSimValue();
  string per_din_7_val  = m_pads[padNameIdMap["per_din\\[7\\]"]]->getSimValue();
  string per_din_6_val  = m_pads[padNameIdMap["per_din\\[6\\]"]]->getSimValue();
  string per_din_5_val  = m_pads[padNameIdMap["per_din\\[5\\]"]]->getSimValue();
  string per_din_4_val  = m_pads[padNameIdMap["per_din\\[4\\]"]]->getSimValue();
  string per_din_3_val  = m_pads[padNameIdMap["per_din\\[3\\]"]]->getSimValue();
  string per_din_2_val  = m_pads[padNameIdMap["per_din\\[2\\]"]]->getSimValue();
  string per_din_1_val  = m_pads[padNameIdMap["per_din\\[1\\]"]]->getSimValue();
  string per_din_0_val  = m_pads[padNameIdMap["per_din\\[0\\]"]]->getSimValue();
  
  string per_din_str = per_din_15_val+ per_din_14_val+ per_din_13_val+ per_din_12_val+ per_din_11_val+ per_din_10_val+ per_din_9_val + per_din_8_val + per_din_7_val + per_din_6_val + per_din_5_val + per_din_4_val + per_din_3_val + per_din_2_val + per_din_1_val + per_din_0_val ;
  
  cout << " Output value is " << per_din_str <<  endl;
  recv_inputs = false;
}

void PowerOpt::readMem(int cycle_num, bool wavefront)
{
    string pmem_addr_str = getPmemAddr();
    unsigned int pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);// binary to int
    static string  last_addr_string;
    unsigned int instr = PMemory[pmem_addr];
    if (instr == 25) {
      cout << "Writing P3Dout " << endl;
      print_processor_state_profile(cycle_num);
      print_dmem_contents(cycle_num);
      Net::flip_check_toggles();
    }
    pmem_instr = binary(instr);
    send_instr = true;
    pmem_request_file << "At address " << pmem_addr << " ( " << pmem_addr_str << " ) instruction is " << hex << instr << dec << " -> " << pmem_instr << " and cycle is " << cycle_num << endl;
    handleDmem(cycle_num, wavefront);
}

void PowerOpt::sendPerDout(string data_str)
{
	m_pads[padNameIdMap["per_dout\\[15\\]"]] -> setSimValue(data_str.substr(15,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[14\\]"]] -> setSimValue(data_str.substr(14,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[13\\]"]] -> setSimValue(data_str.substr(13,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[12\\]"]] -> setSimValue(data_str.substr(12,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[11\\]"]] -> setSimValue(data_str.substr(11,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[10\\]"]] -> setSimValue(data_str.substr(10,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[9\\]"]]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[8\\]"]]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[7\\]"]]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[6\\]"]]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[5\\]"]]  -> setSimValue(data_str.substr(5 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[4\\]"]]  -> setSimValue(data_str.substr(4 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[3\\]"]]  -> setSimValue(data_str.substr(3 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[2\\]"]]  -> setSimValue(data_str.substr(2 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[1\\]"]]  -> setSimValue(data_str.substr(1 ,1), sim_wf);
	m_pads[padNameIdMap["per_dout\\[0\\]"]]  -> setSimValue(data_str.substr(0 ,1), sim_wf);
}

void PowerOpt::sendData (string data_str)
{
  if (!send_data) return;
  dmem_request_file << " Reading Data " << data_str << endl;
  cout << " Reading Data " << data_str << endl;
  // NOTE : inverting the assignments because to_string() of bitset assumes the same orientation of the bits as the constructor. to_string() does NOT invert the bit string for you.
	m_pads[padNameIdMap["dmem_dout\\[15\\]"]] -> setSimValue(data_str.substr(0 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[14\\]"]] -> setSimValue(data_str.substr(1 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[13\\]"]] -> setSimValue(data_str.substr(2 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[12\\]"]] -> setSimValue(data_str.substr(3 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[11\\]"]] -> setSimValue(data_str.substr(4 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[10\\]"]] -> setSimValue(data_str.substr(5 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[9\\]"]]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[8\\]"]]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[7\\]"]]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[6\\]"]]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[5\\]"]]  -> setSimValue(data_str.substr(10,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[4\\]"]]  -> setSimValue(data_str.substr(11,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[3\\]"]]  -> setSimValue(data_str.substr(12,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[2\\]"]]  -> setSimValue(data_str.substr(13,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[1\\]"]]  -> setSimValue(data_str.substr(14,1), sim_wf);
	m_pads[padNameIdMap["dmem_dout\\[0\\]"]]  -> setSimValue(data_str.substr(15,1), sim_wf);
  send_data = false;
}

void PowerOpt::sendInstr(string instr_str)
{
  if (!send_instr) return;
	m_pads[padNameIdMap["pmem_dout\\[15\\]"]] -> setSimValue(instr_str.substr(15,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[14\\]"]] -> setSimValue(instr_str.substr(14,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[13\\]"]] -> setSimValue(instr_str.substr(13,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[12\\]"]] -> setSimValue(instr_str.substr(12,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[11\\]"]] -> setSimValue(instr_str.substr(11,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[10\\]"]] -> setSimValue(instr_str.substr(10,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[9\\]"]]  -> setSimValue(instr_str.substr(9 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[8\\]"]]  -> setSimValue(instr_str.substr(8 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[7\\]"]]  -> setSimValue(instr_str.substr(7 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[6\\]"]]  -> setSimValue(instr_str.substr(6 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[5\\]"]]  -> setSimValue(instr_str.substr(5 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[4\\]"]]  -> setSimValue(instr_str.substr(4 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[3\\]"]]  -> setSimValue(instr_str.substr(3 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[2\\]"]]  -> setSimValue(instr_str.substr(2 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[1\\]"]]  -> setSimValue(instr_str.substr(1 ,1), sim_wf);
	m_pads[padNameIdMap["pmem_dout\\[0\\]"]]  -> setSimValue(instr_str.substr(0 ,1), sim_wf);
  send_instr = false;
}

void PowerOpt::testReadAddr()
{
   m_pads[padNameIdMap["pmem_addr\\[13\\]"]] -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[12\\]"]] -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[11\\]"]] -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[10\\]"]] -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[9\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[8\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[7\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[6\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[5\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[4\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[3\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[2\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[1\\]"]]  -> setSimValue("0", sim_wf);
   m_pads[padNameIdMap["pmem_addr\\[0\\]"]]  -> setSimValue("0", sim_wf);

//   readAddr() ;
//
//   m_pads[padNameIdMap["pmem_addr\\[13\\]"]] -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[12\\]"]] -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[11\\]"]] -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[10\\]"]] -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[9\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[8\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[7\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[6\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[5\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[4\\]"]]  -> setSimValue("0");
//   m_pads[padNameIdMap["pmem_addr\\[3\\]"]]  -> setSimValue("1");
//   m_pads[padNameIdMap["pmem_addr\\[2\\]"]]  -> setSimValue("1");
//   m_pads[padNameIdMap["pmem_addr\\[1\\]"]]  -> setSimValue("1");
//   m_pads[padNameIdMap["pmem_addr\\[0\\]"]]  -> setSimValue("0");
//
//   readAddr() ;

}

void PowerOpt::initialRun()
{
  string instr_str = "0000000000000101";
  pmem_request_file << "Applying instr " << instr_str << endl;
  for (int i = 0; i < 100; i++)
  {
    readMem(-1, false);
    runSimulation(false);
    updateRegOutputs();
    updateFromMem();
  }
  readConstantTerminals();
/*  for (int i = 0; i < 1; i++)
  {
    m_pads[padNameIdMap["reset_n"]] -> setSimValue("0", sim_wf);
    sendInstr(instr_str);
    runSimulation(false);
    updateRegOutputs();
    string pmem_addr_str = getPmemAddr();
    unsigned int pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);
    pmem_request_file << "At address " << pmem_addr << " ( " << pmem_addr_str << " ) instruction is FORCED " << endl;
    //print_processor_state_profile(-2);
    //printSelectGateValues();
  }
  for (int i = 0; i < 1; i++)
  {
    m_pads[padNameIdMap["reset_n"]] -> setSimValue("X", sim_wf);
    sendInstr(instr_str);
    runSimulation(false);
    updateRegOutputs();
    string pmem_addr_str = getPmemAddr();
    unsigned int pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);
    pmem_request_file << "At address " << pmem_addr << " ( " << pmem_addr_str << " ) instruction is FORCED " << endl;
    // print_processor_state_profile(-1);
    // printSelectGateValues();
  }*/
}


bool PowerOpt::check_peripherals()
{
	string per_enable_val  = m_pads[padNameIdMap["per_en"]]->getSimValue();
  if (per_enable_val != "1") return false;
  
	string per_we_1_val  = m_pads[padNameIdMap["per_we\\[1\\]"]]->getSimValue();
	string per_we_0_val  = m_pads[padNameIdMap["per_we\\[0\\]"]]->getSimValue();

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return false;
  
  string per_addr_13_val = m_pads[padNameIdMap["per_addr\\[13\\]"]]->getSimValue();
  string per_addr_12_val = m_pads[padNameIdMap["per_addr\\[12\\]"]]->getSimValue();
  string per_addr_11_val = m_pads[padNameIdMap["per_addr\\[11\\]"]]->getSimValue();
  string per_addr_10_val = m_pads[padNameIdMap["per_addr\\[10\\]"]]->getSimValue();
  string per_addr_9_val  = m_pads[padNameIdMap["per_addr\\[9\\]"]]->getSimValue();
  string per_addr_8_val  = m_pads[padNameIdMap["per_addr\\[8\\]"]]->getSimValue();
  string per_addr_7_val  = m_pads[padNameIdMap["per_addr\\[7\\]"]]->getSimValue();
  string per_addr_6_val  = m_pads[padNameIdMap["per_addr\\[6\\]"]]->getSimValue();
  string per_addr_5_val  = m_pads[padNameIdMap["per_addr\\[5\\]"]]->getSimValue();
  string per_addr_4_val  = m_pads[padNameIdMap["per_addr\\[4\\]"]]->getSimValue();
  string per_addr_3_val  = m_pads[padNameIdMap["per_addr\\[3\\]"]]->getSimValue();
  string per_addr_2_val  = m_pads[padNameIdMap["per_addr\\[2\\]"]]->getSimValue();
  string per_addr_1_val  = m_pads[padNameIdMap["per_addr\\[1\\]"]]->getSimValue();
  string per_addr_0_val  = m_pads[padNameIdMap["per_addr\\[0\\]"]]->getSimValue();

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000001100") return false; // per_addrs is 000c
  return true;
}


bool PowerOpt::check_sim_end(int& i, bool wavefront)
{
	string per_enable_val  = m_pads[padNameIdMap["per_en"]]->getSimValue();
  if (per_enable_val != "1") return false;
  
	string per_we_1_val  = m_pads[padNameIdMap["per_we\\[1\\]"]]->getSimValue();
	string per_we_0_val  = m_pads[padNameIdMap["per_we\\[0\\]"]]->getSimValue();

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return false;

  string per_addr_13_val = m_pads[padNameIdMap["per_addr\\[13\\]"]]->getSimValue();
  string per_addr_12_val = m_pads[padNameIdMap["per_addr\\[12\\]"]]->getSimValue();
  string per_addr_11_val = m_pads[padNameIdMap["per_addr\\[11\\]"]]->getSimValue();
  string per_addr_10_val = m_pads[padNameIdMap["per_addr\\[10\\]"]]->getSimValue();
  string per_addr_9_val  = m_pads[padNameIdMap["per_addr\\[9\\]"]]->getSimValue();
  string per_addr_8_val  = m_pads[padNameIdMap["per_addr\\[8\\]"]]->getSimValue();
  string per_addr_7_val  = m_pads[padNameIdMap["per_addr\\[7\\]"]]->getSimValue();
  string per_addr_6_val  = m_pads[padNameIdMap["per_addr\\[6\\]"]]->getSimValue();
  string per_addr_5_val  = m_pads[padNameIdMap["per_addr\\[5\\]"]]->getSimValue();
  string per_addr_4_val  = m_pads[padNameIdMap["per_addr\\[4\\]"]]->getSimValue();
  string per_addr_3_val  = m_pads[padNameIdMap["per_addr\\[3\\]"]]->getSimValue();
  string per_addr_2_val  = m_pads[padNameIdMap["per_addr\\[2\\]"]]->getSimValue();
  string per_addr_1_val  = m_pads[padNameIdMap["per_addr\\[1\\]"]]->getSimValue();
  string per_addr_0_val  = m_pads[padNameIdMap["per_addr\\[0\\]"]]->getSimValue();

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000001100") return false;

  string per_din_0_val = m_pads[padNameIdMap["per_din\\[0\\]"]]->getSimValue();
  //ToggleType per_din_0_tog_type = m_pads[padNameIdMap["per_din\\[0\\]"]]->getSimToggType(); // checking for FALL might be hard when starting simulation from a saved state.
  if (per_din_0_val != "0") return false;
  return true;
}

void PowerOpt::debug_per_din(int cycle_num)
{
  
  string per_din_15_val = m_pads[padNameIdMap["per_din\\[15\\]"]]->getSimValue();
  string per_din_14_val = m_pads[padNameIdMap["per_din\\[14\\]"]]->getSimValue();
  string per_din_13_val = m_pads[padNameIdMap["per_din\\[13\\]"]]->getSimValue();
  string per_din_12_val = m_pads[padNameIdMap["per_din\\[12\\]"]]->getSimValue();
  string per_din_11_val = m_pads[padNameIdMap["per_din\\[11\\]"]]->getSimValue();
  string per_din_10_val = m_pads[padNameIdMap["per_din\\[10\\]"]]->getSimValue();
  string per_din_9_val  = m_pads[padNameIdMap["per_din\\[9\\]"]]->getSimValue();
  string per_din_8_val  = m_pads[padNameIdMap["per_din\\[8\\]"]]->getSimValue();
  string per_din_7_val  = m_pads[padNameIdMap["per_din\\[7\\]"]]->getSimValue();
  string per_din_6_val  = m_pads[padNameIdMap["per_din\\[6\\]"]]->getSimValue();
  string per_din_5_val  = m_pads[padNameIdMap["per_din\\[5\\]"]]->getSimValue();
  string per_din_4_val  = m_pads[padNameIdMap["per_din\\[4\\]"]]->getSimValue();
  string per_din_3_val  = m_pads[padNameIdMap["per_din\\[3\\]"]]->getSimValue();
  string per_din_2_val  = m_pads[padNameIdMap["per_din\\[2\\]"]]->getSimValue();
  string per_din_1_val  = m_pads[padNameIdMap["per_din\\[1\\]"]]->getSimValue();
  string per_din_0_val  = m_pads[padNameIdMap["per_din\\[0\\]"]]->getSimValue();
  
  string per_din_str = per_din_15_val+ per_din_14_val+ per_din_13_val+ per_din_12_val+ per_din_11_val+ per_din_10_val+ per_din_9_val + per_din_8_val + per_din_7_val + per_din_6_val + per_din_5_val + per_din_4_val + per_din_3_val + per_din_2_val + per_din_1_val + per_din_0_val ;
  
//  bitset<16> temp(per_din_str); 
//  unsigned n = temp.to_ulong();
//  cout << " per_din is " << hex << n << dec << " at cycle " << cycle_num << endl; 
}

bool PowerOpt::readHandShake()
{



}

void PowerOpt::simulate()
{
   // Read file for initial values of reg-Q pins and input ports
  cout << "Simulating" << endl;
  readSimInitFile();
  readDmemInitFile();
  initialRun();
  bool wavefront = true;
  bool done_inputs = false;
  for (int i = 0; i < num_sim_cycles; i++)
  {
    if (check_peripherals() == true) { 
      cout << " STARTING SIMULATION NOW !! " << endl;
      dump_Dmemory();
      print_processor_state_profile(i); 
    }
    readMem(i, wavefront);
    runSimulation(wavefront); // simulates the negative edge

    if (!done_inputs)
    {
      done_inputs = sendInputs();
    }
    else 
      recvInputs1(i, wavefront);
    // debug_per_din(i);
  
    
//    string dmem_din_str = getDmemDin();
//    debug_file << "Dmem_din : " << dmem_din_str << endl;

//    sendData(dmem_data);
//    handleDmem(i, wavefront);

    updateFromMem();
    recvInputs2(i, wavefront);

    runSimulation(wavefront); // simulates  the positive edge
    updateRegOutputs();

    if (check_sim_end(i, wavefront) == true ) { 
      cout << "ENDING SIMULATION" << endl; 
      break ;
    }
    //if (i == 65) print_dmem_contents(i);
  }
  print_dmem_contents(num_sim_cycles-1);
}


void PowerOpt::updateFromMem()
{
  sendData(dmem_data);
  sendInstr(pmem_instr);
}

void PowerOpt::dump_Dmemory()
{
  debug_file << "DMEM Contents" << endl;
  
  map<int, bitset<16> >::iterator it = DMemory.begin();
  for (; it != DMemory.end(); it++)
  {
    debug_file << it->first << " --> " << (it->second).to_string() << endl;
  }  

}

void PowerOpt::computeExprTopo(Graph* graph)
{
     while (graph->getWfSize() != 0)
     {
        GNode* node = graph->getWfNode();
        cout << "Removing : " << node->getName() << endl;
        graph->popWfNode();
        if (node->getVisited() == true)
          continue;
        node->setVisited(true);
        if (node->getIsSource())
        {
            if (node->getIsPad())
            {
              Pad* pad = node->getPad();
              assert(!node->getIsGate());
              if (pad->getNetNum() == 0) continue;
              Net* net = pad->getNet(0); // Expecting only one net per pad
              //net->addOperator(); // EMPTY OPERATOR FOR PADS

              string expr (pad->getName());
              net->addExpression(expr); //

              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == OUTPUT)
                  continue;
                Gate* gate_next = term->getGate();
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_next = gate_next->getGNode();
                if ((node_next->getVisited() == false) && (node_next->getIsSink() == false))// && (node_next->allInputsReady() == true))
                {
                  //cout << "Adding : " << node_next->getName() << endl;
                  graph->addWfNode(node_next);// add to queue
                }
              }
              //cout << "Pad : " << pad->getName() << endl;
            }
            else
            {
              Gate* gate = node->getGate();
              assert(!node->getIsPad());
              assert(gate->getFanoutTerminalNum() == 1); // gates have only one fanout terminal
              Terminal* fanout_term = gate->getFanoutTerminal(0);
              assert(fanout_term->getNetNum() == 2);//(Minor BUG) Both are the same net
              Net* net = fanout_term->getNet(0);

              string expr;
              expr.append(gate->getName());
              net->addExpression(expr+"&");
              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == OUTPUT)
                  continue;
                Gate* gate_next = term->getGate();
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_next = gate_next->getGNode();
                if ((node_next->getVisited() == false) && (node_next->getIsSink() == false))// && (node_next->allInputsReady() == true))
                {
                  //cout << "Adding : " << node_next->getName() << endl;
                  graph->addWfNode(node_next);// add to queue
                }
              }
              //cout << "Gate : " << gate->getName() << " : " << node->getVisited()  << endl;
            }
        }
        else if (!(node->getIsSink()))
        {
            Gate* gate = node->getGate();
            assert(!node->getIsPad());
            assert(gate->getFanoutTerminalNum() == 1); // gates have only one fanout terminal
            Terminal* fanout_term = gate->getFanoutTerminal(0);
            assert(fanout_term->getNetNum() == 2);//(Minor BUG) Both are the same net
            Net* net = fanout_term->getNet(0);

            string expr;
            bool skip_for_now = false;
            for (int i = 0; i < gate->getFaninTerminalNum(); i ++)
            {
               Terminal * fanin_terminal = gate->getFaninTerminal(i);
               assert(fanin_terminal->getNetNum() == 2); // same Minor BUG as above
               Net* fanin_net = fanin_terminal->getNet(0);
               string net_expr = fanin_net->getExpr();
               if (!net_expr.length())
               {
                 graph->addWfNode(node) ;
                 skip_for_now = true;
                 break;
               }
               //net_expr += "EMPTY" ; }
               net_expr.insert(0,"[Net :"+fanin_net->getName()+"]{");
               net_expr += "}";
               //assert(net_expr.length());
               expr.append(net_expr+"&");
            }
            if (skip_for_now)
              continue;

            net->addExpression(expr);
            for (int i = 0; i < net->getTerminalNum(); i++)
            {
              Terminal * term = net->getTerminal(i);
              if (term->getPinType() == OUTPUT)
                continue;
              Gate* gate_next = term->getGate();
              // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
              GNode* node_next = gate_next->getGNode();
              //if (!node_next->getIsSink())
              if (!(node_next->getVisited()) && !(node_next->getIsSink()))// && (node_next->allInputsReady() == true))
              {
                //cout << "Adding : " << node_next->getName() << endl;
                graph->addWfNode(node_next);// add to queue
              }
            }

           //net->addExpressionOp
            //cout << "Gate : " << gate->getName() << " : " << node->getVisited() << endl;
        }
        //else {assert(0); }
     }
}

void PowerOpt::computeTopoSort_new(Graph* graph)
{
  int count = 0;
  //m_gates_topo.resize(count);

  vector<Gate*>::iterator it = m_gates.begin();
  while (graph->getWfSize() != 0)
  {
    GNode* node = graph->getWfNode();
    //cout << "Removing : " << node->getName() << endl;
    graph->popWfNode();
    nodes_topo.push_back(node);
    node->setTopoId(count++);
    node->setVisited(true);
    Net* net;
    if (node->getIsPad() == true)
    {
      Pad* pad = node->getPad();
      if (pad->getNetNum() == 0) continue;
      net = pad->getNet(0);
    }
    else
    {
      assert(node->getIsGate() == true);
      Gate* gate = node->getGate();
      assert(gate->getFanoutTerminalNum() == 1);
      net = gate->getFanoutTerminal(0)->getNet(0);
    }
    net->setTopoSortMarked(true);
    for (int i = 0; i < net->getTerminalNum(); i++)
    {
      Terminal * term = net->getTerminal(i);
      if (term->getPinType() == OUTPUT)
        continue;
      Gate* gate_nxt = term->getGate();
      GNode* node_nxt = gate_nxt->getGNode();
      if (gate_nxt->allInpNetsVisited() == true && node_nxt->getVisited() == false)
      {
        //cout << "Adding : " << node_nxt->getName() << endl;
        graph->addWfNode(node_nxt);// add to queue
      }
    }
  }
}
/*     while (graph->getWfSize() != 0)
     {
        GNode* node = graph->getWfNode();
        cout << "Removing : " << node->getName() << endl;
        graph->popWfNode();
        if (node->getVisited() == true)
          continue;
        node->setVisited(true);
        if (node->getIsSource() == true)
        {
            if (node->getIsPad() == true)
            {
              Pad* pad = node->getPad();
              assert(!node->getIsGate());
              if (pad->getNetNum() == 0) continue;
              Net* net = pad->getNet(0);
              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == OUTPUT)
                  continue;
                Gate* gate_prev = term->getGate(); // THIS IS AN OUTPUT TERMINAL
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_prev = gate_prev->getGNode();
                if ((node_prev->getVisited() == false) && (node_prev->getIsSource() == false) && (node_prev->isInWf() == false))// && (node_next->allInputsReady() == true))
                {
                  cout << "Adding : " << node_prev->getName() << endl;
                  graph->addWfNode(node_prev);// add to queue
                }
              }
            }
            else
            {
              Gate* gate = node->getGate();
              count--;
              //m_gates_topo[count] = gate;
              gate->setTopoId(count);
              assert(!node->getIsPad());
              assert(gate->getFanoutTerminalNum() == 1); // gates have only one fanout terminal
              for (int i = 0; i < gate->getFaninTerminalNum(); i++)
              {
                Terminal* fanin_term = gate->getFaninTerminal(i);
                //if ((fanin_term->getName() == "CP" )|| (fanin_term->getName() == "Q")) continue;
                //if (fanin_term->getName() == "D") continue;
                Net* net = fanin_term->getNet(0);
                for (int i = 0; i < net->getTerminalNum(); i++)
                {
                  Terminal * term = net->getTerminal(i);
                  if (term->getPinType() == INPUT)
                    continue;
                  Gate* gate_prev = term->getGate();
                  // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                  GNode* node_prev = gate_prev->getGNode();
                  if ((node_prev->getVisited() == false) && (node_prev->getIsSource() == false) && (node_prev->isInWf() == false))// && (node_next->allInputsReady() == true))
                  {
                    cout << "Adding : " << node_prev->getName() << endl;
                    graph->addWfNode(node_prev);// add to queue
                  }
                }
              }
              //cout << "Gate : " << gate->getName() << " : " << node->getVisited()  << endl;
            }
        }
        else if (node->getIsSink() == false)
        {
            Gate* gate = node->getGate();
            count--;
            //m_gates_topo[count] = gate;
            gate->setTopoId(count);
            assert(!node->getIsPad());
            for (int i = 0; i < gate->getFaninTerminalNum(); i++)
            {
              Terminal* fanin_term = gate->getFaninTerminal(i);
              Net* net = fanin_term->getNet(0);

              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == INPUT)
                  continue;
                Gate* gate_prev = term->getGate();
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_prev = gate_prev->getGNode();
                if (!(node_prev->getVisited()) && !(node_prev->getIsSource()) && !(node_prev->isInWf()))// && (node_prev->allInputsReady() == true))
                {
                  cout << "Adding : " << node_prev->getName() << endl;
                  graph->addWfNode(node_prev);// add to queue
                }
              }
            }
        }
        else {assert(0); }
     }*/

void PowerOpt::computeTopoSort(Graph* graph)
{
     int count = m_gates.size();
     m_gates_topo.resize(count);
     vector<Gate*>::iterator it = m_gates.begin();
     while (graph->getWfSize() != 0)
     {
        GNode* node = graph->getWfNode();
        cout << "Removing : " << node->getName() << endl;
        graph->popWfNode();
        if (node->getVisited() == true)
          continue;
        node->setVisited(true);
        if (node->getIsSink() == true)
        {
            if (node->getIsPad() == true)
            {
              Pad* pad = node->getPad();
              assert(!node->getIsGate());
              if (pad->getNetNum() == 0) continue;
              Net* net = pad->getNet(0);
              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == INPUT)
                  continue;
                Gate* gate_prev = term->getGate(); // THIS IS AN OUTPUT TERMINAL
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_prev = gate_prev->getGNode();
                if ((node_prev->getVisited() == false) && (node_prev->getIsSource() == false) && (node_prev->isInWf() == false))// && (node_next->allInputsReady() == true))
                {
                  cout << "Adding : " << node_prev->getName() << endl;
                  graph->addWfNode(node_prev);// add to queue
                }
              }
            }
            else
            {
              Gate* gate = node->getGate();
              count--;
              m_gates_topo[count] = gate;
              gate->setTopoId(count);
              assert(!node->getIsPad());
              assert(gate->getFanoutTerminalNum() == 1); // gates have only one fanout terminal
              for (int i = 0; i < gate->getFaninTerminalNum(); i++)
              {
                Terminal* fanin_term = gate->getFaninTerminal(i);
                //if ((fanin_term->getName() == "CP" )|| (fanin_term->getName() == "Q")) continue;
                //if (fanin_term->getName() == "D") continue;
                Net* net = fanin_term->getNet(0);
                for (int i = 0; i < net->getTerminalNum(); i++)
                {
                  Terminal * term = net->getTerminal(i);
                  if (term->getPinType() == INPUT)
                    continue;
                  Gate* gate_prev = term->getGate();
                  // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                  GNode* node_prev = gate_prev->getGNode();
                  if ((node_prev->getVisited() == false) && (node_prev->getIsSource() == false) && (node_prev->isInWf() == false))// && (node_next->allInputsReady() == true))
                  {
                    cout << "Adding : " << node_prev->getName() << endl;
                    graph->addWfNode(node_prev);// add to queue
                  }
                }
              }
              //cout << "Gate : " << gate->getName() << " : " << node->getVisited()  << endl;
            }
        }
        else if (node->getIsSink() == false)
        {
            Gate* gate = node->getGate();
            count--;
            m_gates_topo[count] = gate;
            gate->setTopoId(count);
            assert(!node->getIsPad());
            for (int i = 0; i < gate->getFaninTerminalNum(); i++)
            {
              Terminal* fanin_term = gate->getFaninTerminal(i);
              Net* net = fanin_term->getNet(0);

              for (int i = 0; i < net->getTerminalNum(); i++)
              {
                Terminal * term = net->getTerminal(i);
                if (term->getPinType() == INPUT)
                  continue;
                Gate* gate_prev = term->getGate();
                // NEED TO CREATE GATE TO GNODE and PAD TO GNODE MAP;
                GNode* node_prev = gate_prev->getGNode();
                if (!(node_prev->getVisited()) && !(node_prev->getIsSource()) && !(node_prev->isInWf()))// && (node_prev->allInputsReady() == true))
                {
                  cout << "Adding : " << node_prev->getName() << endl;
                  graph->addWfNode(node_prev);// add to queue
                }
              }
            }
        }
        else {assert(0); }
     }
}

string add_delim (vector<string> tokens, char delim, int start_point)
{
  string full_string = "";
  char * dlm = &delim;
  for (int i = 2; i < tokens.size(); i++)
  {
    full_string.append(tokens[i]);
    full_string.append(dlm);
  }
  return full_string;
}

void PowerOpt::readClusters()
{
   ifstream cluster_file_names;
   cluster_file_names.open(clusterNamesFile.c_str());
   if (cluster_file_names.is_open()){
     string cluster_file_name;
     int cluster_id = 0;
     while ( getline(cluster_file_names, cluster_file_name))
     {
       ifstream cluster_file;
       cluster_file.open(cluster_file_name.c_str());
       vector<Gate*> cluster;
       if(cluster_file.is_open())
       {
         string gate_name;
         while (getline(cluster_file, gate_name))
         {
           Gate* gate = m_gates[gateNameIdMap[gate_name]];
           gate->setClusterId(cluster_id);
           cluster.push_back(gate);
         }
       }
       clusters.insert(make_pair(cluster_file_name, cluster)); // bad coding. Might copy the entire vector into the map;
       cluster_id++;
     }
   }
}

void PowerOpt::countClusterCuts()
{

    // Read File with clusterFile Names
    // For each clusterfile create a vector of gates with names 1, 2, 3 ......  . ALSO maintain a cluster id for each gate
    readClusters();
    // Iterate over each cluster and get the number of cuts. Add them all and divide by 2 since each cut is counted twice.
    map<string, vector<Gate*> > :: iterator it;
    int total_cut_count = 0;
    for (it = clusters.begin(); it != clusters.end(); it++)
    {
        vector<Gate*> cluster = it->second;
        int cluster_cut_count = 0;
        for (int i = 0; i < cluster.size(); i++)
        {
            Gate* gate = cluster[i];
            if(gate->isClusterBoundaryDriver())
            {
                cluster_cut_count++;
                total_cut_count++;
            }
        }
        cout << "Cluster Number " << it->first << " of size " << cluster.size() << " has " << cluster_cut_count << " cuts " << endl;
    }
    cout << " Total cut count " << total_cut_count << endl;
}

void PowerOpt::print_pads()
{
  debug_file << " Pads " << endl;
  for (int i = 0; i < m_pads.size(); i++)
  {
    Pad* pad = getPad(i);
    debug_file << "Pad : " <<  pad->getName()  << " Net_num : " << pad->getNetNum() << endl;
  }

}

void PowerOpt::print_nets()
{
  debug_file << "FINAL " << endl;
  for (int i = 0; i < getNetNum(); i++)
  {
    Net * net = getNet(i);
    //cout << "net : " << net->getName() << endl;
    if (net->getToggled() == false && net->getSimValue() != "X") debug_file << " NOT TOGGLED " << net->getName() << " val : " << net->getSimValue() << endl;
    else if (net->getSimValue() == "X") debug_file << " IS X " << net->getName() << endl;
    //Gate* gate = net->getDriverGate();
//    if (net->getToggled() == true)
//      debug_file << "TOGGLED " <<  net->getName() << endl;
//    else
//      debug_file << "NOT TOGGLED " << net->getName() << endl;
  }
}

void PowerOpt::print_regs()
{
  list<Gate*>::iterator it;
  for(it = m_regs.begin(); it != m_regs.end(); it++)
  {
    Gate* ff_gate = *it;
    cout << ff_gate->getName() << endl;
  }
}


void PowerOpt::print_gates()
{
  for (int i =0; i < getGateNum(); i++)
  {
    Gate* gate = m_gates[i];
    cout << gate->getName() << " : " << i << endl;
  }

}

void PowerOpt::print_term_exprs()
{
    for (int i = 0; i < getTerminalNum(); i++)
    {
        Terminal* term = terms[i];
        debug_file << i << " : " << term->getFullName() << " ( " << term->getNetName() << " ) --> " << term->getExpr() << endl;
    }

}

void PowerOpt::print_terminals()
{
  for (int i = 0; i < getTerminalNum(); i++)
  {
    Terminal* term = terms[i];
    debug_file << i << " : " << term->getFullName() << " --> " ;
    term->printNets(debug_file);
    debug_file << endl;
  }
}

void PowerOpt::printTopoOrder()
{
  list<GNode*>::iterator it = nodes_topo.begin();
  for (it = nodes_topo.begin(); it != nodes_topo.end(); it ++)
  {
     GNode* node = *it;
     cout << node->getTopoId() << " : " <<  node->getName() << endl;
  }
/*  for (int i =0; i < m_gates_topo.size(); i++)
  {
    cout << m_gates_topo[i]->getName() << endl;
  }*/
}


void PowerOpt::checkTopoOrder(Graph* graph)
{
   vector<GNode*>& nodes = graph->getNodeVec();
   for (int i = 0; i < nodes.size(); i++)
   {
      GNode * node = nodes[i];
      node->checkTopoOrder();
   }
}



void PowerOpt::topoSort()
{
// Create the Sources and Sinks
    graph = new Graph;
    populateGraphDatabase(graph);
    cout << "DONE POPULATING GRAPH" << endl;
    computeTopoSort_new(graph);
    graph->clear_visited();
    graph->print();
    cout << "Finished Top Sort" << endl;
    cout << " Checking topo order ... " ;
    checkTopoOrder(graph);
    cout << " done " << endl;
    //printTopoOrder();

    //TODO: COMPUTE EXPRESSIONS!!!

    //graph->print_sources();
    //graph->print_wf();

    // computeExprTopo(graph);
    //cout << " DONE ExprComputation" << endl;
    //print_nets();
}

int PowerOpt::checkNumErFF()
{
    int num_er_ff = 0;
    Path *p;
    Gate *g;
    clearCheck();
    for (int i = 0; i < criticalPaths.size(); i++)
    {
        p = criticalPaths[i];
        g = p->getGate(p->getGateNum()-1);
        if (g->getFFFlag()) g->setChecked(true);
    }
    for (int i = 0; i < getGateNum(); i ++) {
        if (m_gates[i]->isChecked()) num_er_ff ++;
    }
    clearCheck();
    return num_er_ff;
}

void PowerOpt::clearCheck()
{
    for (int i = 0; i < getGateNum(); i ++) {
        m_gates[i]->setChecked(false);
    }
}

// JMS-SHK begin
/*
void PowerOpt::swap_razor_flops(designTiming *T)
{

  vector<Path *>::iterator pit;
  for ( pit = criticalPaths.begin(); pit != criticalPaths.end(); pit++) {
    // @@ fill in the holes in this code
    if(endpoint of *pit is not a RZ-FF){
      perform a cell swap to make the endpoint of *pit into a RZ-FF;
    }
  }
}
*/
// JMS-SHK end

// extract a list of negative slack toggled paths
void PowerOpt::extCriticalPaths(designTiming *T)
{
    clearCriticalPath();
    Path *p;
    double path_slack;
    // extras for exeop 14
    double random_variation = 0.0;
    int total_path_gates;
    Gate *g;
    // TEST
    double min_slack = DBL_MAX;
    for (int i = 0; i < toggledPaths.size(); i++)
    {
        p = toggledPaths[i];
        path_slack = T->getPathSlack(p->getPathStr());

        // ADD RANDOM VARIATION
        if(getExeOp() == 14){
          // CALCULATE RANDOM VARIATION
          random_variation = 0.0;
          total_path_gates = p->getGateNum();
          // don't add random variation for last cell if it is a FF
          if((p->getGate(total_path_gates-1))->getFFFlag()){
            total_path_gates--;
          }
          for (int j = 0; j < total_path_gates; j++){
            g = p->getGate(j);
            random_variation += g->getDelayVariation();
          }

          // @TODO: ADD WIRE VARIATION

          p->setVariation(random_variation);

          // ADD TO PATH SLACK
          path_slack -= random_variation;
        }

        p->setSlack(path_slack);
        if (path_slack < 0)
        {
            addCriticalPath(p);
            //cout << "path : " << p->getId() << " : "
            //     << path_slack << endl;
        }
        //TEST
        if(path_slack < min_slack){ min_slack = path_slack;}
    }
    //cout << "min slack = " << min_slack << endl;
}

double PowerOpt::calErrorRate(int total_cycles, designTiming *T)
{
    double ErrorRate = 0.0;
    int num_errors = 0;
    // number of cycles in which a negative slack path is toggled

    // criticalPaths is a vector of negative slack paths
    // create a list of error_cycles
    //
    //vector< vector<int> > error_cycle(2, total_cycles);
    // alternating variables to hold the union set
#ifdef EXTRA_ERRS
    vector<int> extra_errs;
#endif
    // extra error cycles added for paths with delays
    // longer than 2T
    // !!This can't happen in reality, since a new input is applied at time T
    // !!i.e., EXTRA_ERRS should never be defined
    vector<int>::iterator iterr[2];   // pointer to end of current union set


#ifdef EXTRA_ERRS
    vector<int>::iterator loopiter;
    Path *p;
    double path_slack;
    for (int i = 0; i < criticalPaths.size(); i++)
    {
        p = criticalPaths[i];
        path_slack = p->getSlack();
        // if the delay is more than 2 times the clock period
        // (negative slack is more than 1 period), insert
        // additional error cycles
        // @NOTE: this should be reconsidered -- will 2 errors
        // really happen for this case?
        // @NOTE: this code currently assumes a static
        // period of 1.15
        if(path_slack < -1.15){
            loopiter = p->getToggleCycles();
            for(int ee=0; ee< p->getNumToggled(); ee++,loopiter++)
            {
                extra_errs.push_back( (*loopiter)+1 );
            }
        }
    }

    // sort extra_errs for union operation
    sort(extra_errs.begin(), extra_errs.end());
    // remove duplicate items
    iterr[1] = unique(extra_errs.begin(), extra_errs.end());
#endif

    int current_result = 0;
    // tells which result set contains the union
    int next_result = 1;
    int num_paths = criticalPaths.size();

    // take the union over all the clk_cycle sets
    // initialize the result vectors
    if(num_paths > 0){
#ifdef EXTRA_ERRS
        iterr[0]=set_union(extra_errs.begin(), iterr[1], criticalPaths[0]->getToggleCycles(), (criticalPaths[0]->getToggleCycles())+(criticalPaths[0]->getNumToggled()) , error_cycle[0].begin());
#else
        iterr[0]=copy(criticalPaths[0]->getToggleCycles(), (criticalPaths[0]->getToggleCycles())+(criticalPaths[0]->getNumToggled()), error_cycle[0].begin() );
#endif
    }
   else{
     numErrorCycles = 0;
     return 0.0;
   }

   // the rest of the unions are the current union set with the remaining vectors
   for(int ii = 1; ii < num_paths; ii++){
     iterr[next_result]=set_union(error_cycle[current_result].begin(), iterr[current_result],criticalPaths[ii]->getToggleCycles(), (criticalPaths[ii]->getToggleCycles())+(criticalPaths[ii]->getNumToggled()), error_cycle[next_result].begin());
     // swap current and next result
     current_result = next_result;
     next_result = (current_result+1)%2;
   }

   num_errors = int(iterr[current_result] - error_cycle[current_result].begin());
   //cout << "num_errors = " << num_errors << endl;
   // THIS WOULD BE WHERE TO PRINT THE ERROR CYCLES TO TAKE A UNION OVER ALL STAGES FOR PROCESSOR ERROR RATE *******
   //cout << "error cycles = ";
   //for(int ii = 0; ii < num_errors; ii++){
   //  cout << error_cycle[current_result][ii] << " ";
   //}
   //cout << endl;


   // initialize error_cycles -- the set of cycles in which at least one negative slack path toggled
   // also initialize the size of minus_cycles, but not the content -- contect will be initilized in ReducePower
   // also initialize the size of delta_cycles
   copy(error_cycle[current_result].begin(), iterr[current_result], error_cycles.begin() );
   numErrorCycles = num_errors;


   ErrorRate = (double) num_errors / (double) total_cycles;
   if (ErrorRate > 1.0 || ErrorRate < 0) ErrorRate = (double) 0.0;

   //chkToggledGate();
   //extPaths();


   return ErrorRate;
}

// report a sorted list of the toggle rates of all FFs in the design
void PowerOpt::reportToggleFF(designTiming *T) {
    Gate *g;
    list<Gate *> gate_list;
    list<Gate *>::iterator  it;

    for (int i = 0; i < getGateNum(); i ++){
        g = m_gates[i];
        // get the toggle rates for all FF
        if(g->getFFFlag()){
          g->setSwActivity(T->getCellToggleRate(g->getName()));
          gate_list.push_back(g);
        }
    }

    gate_list.sort(GateSWL2S());

    ofstream fout;
    fout.open("ff_toggle.list");

    for ( it = gate_list.begin(); it != gate_list.end(); it++) {
        g = *it;
        fout << g->getName() << "\t" << g->getSwActivity() << endl;
    }

    fout.close();
}

// parseCycle is the number of cycles to parse in each VCD file
// if parseCycle == NEGATIVE, parse the entire VCD
void PowerOpt::parseVCDALL(designTiming *T) {
    toggledPaths.clear();
    //int total_cycles=0;           // total number of cycles from all VCD files
    totalSimCycle = 0;
    // totalSimCycle also functions as current cycle number offset for setting toggled cycles of paths
    int vcd_cycles=0;             // number of cycles parsed from the VCD file

    if (vcdFile.size() == 0) {
        cout << "VCD files are not specified!!"<< endl;
        exit(1);
    }

    for (int i = 0; i < vcdFile.size(); i++) {
        cout << "[PowerOpt] Parsing VCD File:" << vcdFile[i] << endl;
        string vcdfilename = vcdPath + "/" + vcdFile[i];
        vcd_cycles = parseVCD(vcdfilename , T, parseCycle, totalSimCycle);
        totalSimCycle += vcd_cycles;
        cout << "[PowerOpt] Parsing VCD done with cycles=" << vcd_cycles << "(total=" << totalSimCycle << ")" << endl;
    }
    resizeCycleVectors(totalSimCycle);
    cout << "[PowerOpt] # Toggled Paths : " << getToggledPathsNum() << endl;
}

// parseCycle is the number of cycles to parse in each VCD file
// if parseCycle == NEGATIVE, parse the entire VCD
void PowerOpt::parseVCDALL_mode_15(designTiming *T) {
    toggledPaths.clear();
    //int total_cycles=0;           // total number of cycles from all VCD files
    totalSimCycle = 0;
    // totalSimCycle also functions as current cycle number offset for setting toggled cycles of paths
    int vcd_cycles=0;             // number of cycles parsed from the VCD file

    if (vcdFile.size() == 0) {
        cout << "VCD files are not specified!!"<< endl;
        exit(1);
    }
    for (int i = 0; i < vcdFile.size(); i++) {
        cout << "[PowerOpt] Parsing VCD File:" << vcdFile[i] << endl;
        string vcdfilename = vcdPath + "/" + vcdFile[i];
        //vcd_cycles = parseVCD_mode_15(vcdfilename , T, parseCycle, totalSimCycle); // this is the flat vcd parser
        vcd_cycles = parseVCD_mode_15_new(vcdfilename , T, parseCycle, totalSimCycle); // this is the complete vcd parser
        totalSimCycle += vcd_cycles;
        cout << "[PowerOpt] Parsing VCD done with cycles=" << vcd_cycles << "(total=" << totalSimCycle << ")" << endl;
    }
    resizeCycleVectors(totalSimCycle);
    cout << "[PowerOpt] # Toggled Paths : " << getToggledPathsNum() << endl;
}

void PowerOpt::resizeCycleVectors(int total_cyc)
{
    error_cycles.resize(total_cyc);
    minus_cycles.resize(total_cyc);
    delta_cycles.resize(total_cyc);
    error_cycle.resize(2, vector<int> (total_cyc) );
}

void PowerOpt::addGate(Gate *g)
{
    m_gates.push_back(g);
    if (g->getIsMux()) m_muxes.push_back(g);
    if (g->getFFFlag()) m_regs.push_back(g);
    chip.addGate(g);
    gate_name_dictionary.insert(std::pair<std::string, Gate*>(g->getName(), g) );
    //cout << "Inserting gate : " << g->getName() << " with pointer value : " << g << endl;
}

void PowerOpt::build_net_name_id_map()
{
  for (int i = 0; i< nets.size(); i++)
  {
    string name = nets[i]->getName();
    netNameIdMap[name] = i;
  }
}

void PowerOpt::mark_clock_tree_cells(designTiming* T)
{
    string clock_tree_cells_str = T->getClockTreeCells();
    vector<string> clock_tree_cells;
    tokenize(clock_tree_cells_str, ' ', clock_tree_cells);
    //cout << clock_tree_cells_str << " " << clock_tree_cells.size();
    for (int i = 0; i < clock_tree_cells.size(); i++)
    {
       string cell_name = clock_tree_cells[i];
       int id = gateNameIdMap[cell_name];
       Gate * gate = m_gates[id];
       gate->setIsClkTree(true);
       //cout << "Clock Tree " << gate->getName();
    }
}

/*void PowerOpt::build_net_pin_library()
{


}*/

void PowerOpt::print_net_name_id_map()
{
  map<string, int>:: iterator it ;
  for (it = netNameIdMap.begin(); it != netNameIdMap.end(); it++)
  {
    cout << it->first << " : " << it->second << endl;
  }
  //exit(0);
}

void PowerOpt::build_term_name_id_map()
{
  for (int i = 0; i< terms.size(); i++)
  {
    string name = terms[i]->getFullName();
    terminalNameIdMap[name] = i;
  }
}

void PowerOpt::my_debug(designTiming * T)
{
  cout << "In my_debug" << endl;
//  int id = gateNameIdMap["U101"];
//  Gate* gate = m_gates[id];
//  cout << gate->getCellName() << endl;
//  Terminal * term  = terms[terminalNameIdMap["frontend_0/U500/A1"]];
//  T->setFalsePathForTerm(term->getFullName());

    Gate *g;
    string master;
    for (int j = 0; j < getGateNum(); j++) {
        g = m_gates[j];
        master = g->getCellName();
        cout << g->getName() << " : " << libLeakageMap[master] << endl;
    }
/*  double slack = T->getMaxFallSlack(term->getFullName());
  cout << "Slack is " << slack << endl;
  Terminal * term_1  = terms[terminalNameIdMap["frontend_0/U720/A1"]];
  T->setFalsePathForTerm(term_1->getFullName());
  slack = T->getMaxFallSlack(term_1->getFullName());
  cout << "Slack is " << slack << endl;*/
//  for (int i = 0; i < gate->getFanoutTerminalNum(); i++)
//  {
//    cout << gate->getFanoutTerminal(i)->getName() << endl;
//  }
//  for (int i = 0; i < gate->getFaninTerminalNum(); i++)
//  {
//    cout << gate->getFaninTerminal(i)->getName() << endl;
//  }
}




void PowerOpt::read_modules_of_interest()
{
    ifstream module_names_file;
    module_names_file.open(moduleNamesFile.c_str());
    if (module_names_file.is_open()){
      string module;
      while (getline(module_names_file, module))
      {
        trim(module);
        modules_of_interest.insert(module);
        cout << "Inserting module of interest " << module << endl;
      }
    }
    else {
      cout << "File modules_of_interest not found. Is it where it should be? " << endl;
      exit(0);
    }
}

void PowerOpt::createSetTrie()
{
  tree = new SetTrie;
}

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

void PowerOpt::handle_toggled_nets_newer(vector< pair<string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
    if (cycle_time == 282550000 )  Net::flip_check_toggles();
    for (int i = 0; i < toggled_nets.size(); i++)
    {
      Net* net = nets[netNameIdMap[toggled_nets[i].first]];
      net->setSimValue(toggled_nets[i].second);
    }
}

void PowerOpt::handle_toggled_nets_new(vector< pair<string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
  if (toggled_nets.size() == 0) return;
   if (exeOp == 22) { handle_toggled_nets_newer(toggled_nets, T, cycle_num,  cycle_time); return ;}
   cout << "Handling Toggled Nets for cycle " << cycle_num << " " << cycle_time  << endl;
   for (int i = 0; i < toggled_nets.size(); i++) // MARK NETS
   {
     int net_id = netNameIdMap[toggled_nets[i].first];
     Net* net = nets[net_id];
     net->updateToggled();
   }
   for (int i = 0; i < terms.size(); i ++) // INFER TOGGLES FROM NETS TO PINS (Since same net can have multiple hierarchical names leading to a the same pins marked many times)
   {
        Terminal* term = terms[i];
        term->inferToggleFromNet(); // THIS ENSURES THAT YOU DON'T NEED TO CLEAR TOGGLES AT THE END
   }
   set_mux_false_paths();
   //if (cycle_num > 25) check_gates(); // many checks fail for first cycle since previous values of nets don't exist
   //check_gates(); // many checks fail for first cycle since previous values of nets don't exist

   vector < pair<Terminal*, ToggleType> > toggled_terms;
   string toggled_terms_str;
   vector<int> toggled_indices;
   for (int i = 0; i < terms.size(); i++) // BUILD STRING OF TOGGLED PINS
   {
     Terminal* term = terms[i];
     if (term->isToggled() == true)
     {
       //if (term->isMuxOutput()) handle_mux_pin(term);
       ToggleType togg_type = term->getToggType();
       toggled_terms.push_back(make_pair(term, togg_type));
       toggled_terms_str.append(term->getFullName());
       switch (togg_type)
       {
         case POWEROPT::RISE: toggled_terms_str.append("-R");break;
         case POWEROPT::FALL: toggled_terms_str.append("-F");break;
         case POWEROPT::UNKN: toggled_terms_str.append("-X");break;
         default : assert(0);
       }
       toggled_terms_str.append(",");


       int index = term->getId();
       ToggleType toggl_type = term->getToggType();
       index = index*3 + togg_type;
       toggled_indices.push_back(index);
       //toggle_info_file << term->getFullName() << " " << term->getToggledTo() << endl;
     }
   }

   //UNIQUIFICATION
   static int unique_toggle_set_id = 0;
   map< string, pair<int, pair<int, float> > >:: iterator it = toggled_terms_counts.find(toggled_terms_str);
   if (it == toggled_terms_counts.end()) // NEW INSERTION
   {
     pair< map<string, pair<int, pair< int, float> > >::iterator, bool> ret;
     ret = toggled_terms_counts.insert(make_pair( toggled_terms_str, make_pair(unique_toggle_set_id, make_pair(1, 10000.0))));
     assert(ret.second);
     unique_toggle_set_id++;
     per_cycle_toggled_sets.push_back(make_pair (cycle_time, ret.first));
     unique_toggle_gate_sets << unique_toggle_set_id << ":" << toggled_terms_str << endl;
   }
   else
   {
     (it->second.second.first)++ ;
     per_cycle_toggled_sets.push_back(make_pair (cycle_time, it) );
   }
   toggled_term_vector.push_back(toggled_terms);
   toggle_info_file << "************" << endl;


/*    // UNITS
    if (!tree->essr(toggled_indices, false))
        tree->insert(toggled_indices);*/

   for (int i = 0; i < toggled_nets.size() ; i++)
   {
        int net_id = netNameIdMap[toggled_nets[i].first];
        Net* net = nets[net_id];
        net->updatePrevVal();
        net->setToggled(false);// THIS IS NOT NECESSARY SINCE THE NEXT TIME WE MARK NETS THE NON_TOGGLED NETS WILL BE MARKED AS NON_TOGGLED
   }
   toggled_nets.clear();

}

void PowerOpt::handle_toggled_nets(vector< pair<string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
//  handle_toggled_nets_to_pins(toggled_nets, T, cycle_num, cycle_time);
//  return ;
  //static int pathID = 0;
  //net_gate_maps << "Cycle " << cycle_time << endl;
  if (exeOp == 22) return;
  if (toggled_nets.size())
  {
    cout << "Handling Toggled Nets"  << endl;
      for (int i = 0 ; i < toggled_nets.size() ; i++)
      {
          string cellstr;
          string net_name;
          map<string, string>::iterator gate_lookup =  source_gate_dictionary.find(toggled_nets[i].first);
          if( gate_lookup == source_gate_dictionary.end() ){
            net_name = toggled_nets[i].first;
            replace_substr(net_name, "[", "\\[" ); // FOR PT SOCKET
            cellstr =  T->getCellFromNet(net_name);
            source_gate_dictionary[toggled_nets[i].first] = cellstr;
          }
          else{
            cellstr = (*gate_lookup).second;
          }
          //cout << toggled_nets[i] << " --> " << cellstr << endl;
          net_gate_maps << toggled_nets[i].first  << " --> " << cellstr << endl;
          if ( cellstr != "NULL_CELL" ) {
            if ( cellstr == "MULTI_DRIVEN_NET" ) {
              cout << "[ERROR] This net " << net_name << " is driven by multiple cells" << endl;
            }
            //map<string, Gate*>::iterator gate_lookup2 =  gate_name_dictionary.find(cellstr);
            int CurInstId = gateNameIdMap[cellstr];
            m_gates[CurInstId]->setToggled ( true, toggled_nets[i].second) ;
            m_gates[CurInstId]->incToggleCount();
            m_gates[CurInstId]->updateToggleProfile(cycle_num);
          }
      }
      //check_for_flop_toggles_fast(cycle_num, cycle_time, T);
      //extractPaths(cycle_num, pathID);
      if (exeOp == 15)
        check_for_flop_toggles(cycle_num, cycle_time, T);
      else if (exeOp == 16)
        check_for_flop_toggles_fast(cycle_num, cycle_time, T);
      else if (exeOp == 17)
        check_for_flop_toggles_fast_subset(cycle_num, cycle_time, T);
      else {
        cout << "Wrong ExeOp given " << endl;
        exit(0);
      }
    //cout << "Exiting "  << endl;
    //exit(0);
  }
  clearToggled();
  toggled_nets.clear();
}

/*void PowerOpt::handle_mux_pin(Terminal * term) // Written only to handle 2  muxes
{
  Gate* gate = term->getGate();
  Terminal * select_pin;
  for (int i = 0; i < gate->getFaninTerminalNum(); i++)
  {
    select_pin = gate->getFaninTerminal(i);
    if (select_pin->getName() == "S") break;
  }

}*/


void PowerOpt::check_gates()
{
  for (int i = 0; i < m_gates.size(); i++)
  {
    Gate* gate = m_gates[i];
    if (gate->check_toggle_fine() == false)
        cout << " Gate with problem : " << gate->getName() << " Type " << gate->getCellName() << endl;
  }

}

void PowerOpt::check_for_term_toggles_fast(int cycle_num, int cycle_time, designTiming* T)
{
  cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  toggle_info_file << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  vector < pair<Terminal*, ToggleType> > toggled_terms;
  string toggled_terms_str;
  //check_gates();
  for (int i = 0; i < getTerminalNum(); i++)
  {
    Terminal* term = terms[i];
    if (term->isToggled() == true)
    {
      //if (term->isMuxOutput()) handle_mux_pin(term);
      ToggleType togg_type = term->getToggType();
      toggled_terms.push_back(make_pair(term, togg_type));
      toggled_terms_str.append(term->getFullName());
      switch (togg_type)
      {
        case POWEROPT::RISE: toggled_terms_str.append("-R");break;
        case POWEROPT::FALL: toggled_terms_str.append("-F");break;
        case POWEROPT::UNKN: toggled_terms_str.append("-X");break;
        default : assert(0);
      }
      toggled_terms_str.append(",");
      toggle_info_file << term->getFullName() << " " << term->getToggledTo() << endl;
    }
  }
  map< string, pair<int, pair<int, float> > >:: iterator it = toggled_terms_counts.find(toggled_terms_str);
  static int unique_toggle_set_id = 0;
  if (it == toggled_terms_counts.end())
  {
    pair< map<string, pair<int, pair<int, float> > >::iterator, bool> ret;
    ret = toggled_terms_counts.insert(make_pair( toggled_terms_str, make_pair(unique_toggle_set_id, make_pair(1, 10000.0))));
    assert(ret.second);
    unique_toggle_set_id++;
    per_cycle_toggled_sets_terms.push_back(make_pair (cycle_time, ret.first));
    unique_toggle_gate_sets << toggled_terms_str << endl;
  }
  else
  {
    (it->second.first)++ ;
    per_cycle_toggled_sets.push_back(make_pair (cycle_time, it) );
  }
  //toggled_term_vector.push_back(toggled_terms);
  toggle_info_file << "************" << endl;
}

void PowerOpt::check_for_term_toggles_fast_subset ( int cycle_num, int cycle_time, designTiming *T)
{
  cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  toggle_info_file << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  //vector < pair<Terminal*, ToggleType> > toggled_terms;
  vector<int> toggled_indices;
  //string toggled_terms_str;
  //int num_terminals = getTerminalNum();
  for (int i = 0; i < getTerminalNum(); i++)
  {
    Terminal* term = terms[i];
    if (term->isToggled() == true)
    {
      //if (term->isMuxOutput()) handle_mux_pin(term);
      int index = term->getId();
      ToggleType togg_type = term->getToggType();
      index = index*3 + togg_type;
      toggled_indices.push_back(index);
      //toggled_terms.push_back(make_pair(term, togg_type));
      //toggled_terms_str.append(term->getFullName());
/*      switch (togg_type)
      {
        case POWEROPT::RISE: index += RISE*num_terminals;
        case POWEROPT::FALL: toggled_terms_str.append("-F");break;
        case POWEROPT::UNKN: toggled_terms_str.append("-X");break;
        default : assert(0);
      }*/
      //toggled_terms_str.append(",");
      toggle_info_file << term->getFullName() << " " << term->getToggledTo() << endl;
    }
  }
/*  map< string, pair<int, float> >:: iterator it = toggled_terms_counts.find(toggled_terms_str);
  if (it == toggled_terms_counts.end())
  {
    pair< map<string, pair<int, float> >::iterator, bool> ret;
    ret = toggled_terms_counts.insert(make_pair( toggled_terms_str, make_pair(1, 10000.0)));
    assert(ret.second);
    per_cycle_toggled_sets_terms.push_back(make_pair (cycle_time, ret.first));
    unique_toggle_gate_sets << toggled_terms_str << endl;
  }
  //toggled_term_vector.push_back(toggled_terms);
  toggle_info_file << "************" << endl;*/
    if (!tree->essr(toggled_indices, false))
        tree->insert(toggled_indices);


}

/*void PowerOpt::topo_search_gates()
{
  for (int i = 0; i < m_gates_topo.size(); i++)
  {
      Gate* gate = m_gates_topo[i];
      if (gate->getIsMux())
      {
        // get select_pin's value
        string val = gate->getMuxSelectPinVal();

        // mark other input pin as false path
        if (val == "0") gate->untoggleMuxInput("1");
        if (val == "1") gate->untoggleMuxInput("0");
      }
      else
      {
        int num_toggled_inputs = gate->numToggledInputs();
        if(!gate->getFFFlag() && (num_toggled_inputs > 1))
        {
          vector<int> false_term_ids;
          gate->checkControllingMIS(false_term_ids);
          cout << "Gate : " << gate->getName() << " ( " << gate->getCellName() << " ) "  << " has MIS ( " << num_toggled_inputs << " ) "  << endl;
        }
      }
  }
}*/

void PowerOpt::set_mux_false_paths()
{
    list<Gate*>::iterator it;
    for (it  = m_muxes.begin(); it != m_muxes.end(); it++)
    {
        Gate* gate = *it;
        string val = gate->getMuxSelectPinVal();
        if (val == "0") gate->untoggleMuxInput("1");
        if (val == "1") gate->untoggleMuxInput("0");
    }
}

void PowerOpt::handle_toggled_nets_to_pins(vector< pair<string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
  if (toggled_nets.size()) {
    cout << "Handling Toggled Nets for cycle " << cycle_num << " " << cycle_time  << endl;
    net_pin_maps << "Handling Toggled Nets for cycle " << cycle_num << " " << cycle_time  << " Size of toggled_nets : " << toggled_nets.size() << endl;
    for(int i= 0; i < toggled_nets.size(); i++) {
      string net_name = toggled_nets[i].first;
      map<string, string >::iterator term_lookup  = net_terminal_dictionary.find(net_name);
      string term_list;
      if (term_lookup == net_terminal_dictionary.end()) {
        replace_substr(net_name, "[", "\\[" ); // FOR PT SOCKET
        term_list = T->getTermsFromNet(net_name);
        net_terminal_dictionary.insert(make_pair(net_name, term_list));
      }
      else {
        term_list = (*term_lookup).second;
      }
      net_pin_maps << net_name << " : " << term_list << endl;
      vector<string> terminals;
      tokenize(term_list, ' ', terminals);
      for(int j = 0; j < terminals.size(); j++) {
        int id = terminalNameIdMap[terminals[j]];
        Terminal* term = terms[id];
//        if (term->isToggled() == false)
//        {
          term->setToggled(true, toggled_nets[i].second);
        //}
      }
    }
    net_pin_maps << "net_terminal_dictionary's size : " << net_terminal_dictionary.size() << endl;
    //topo_search_gates();
    //set_mux_false_paths();
    check_for_term_toggles_fast(cycle_num, cycle_time, T);
    //check_for_term_toggles_fast_subset(cycle_num, cycle_time, T);
  }
  clearToggledTerms();
  toggled_nets.clear();
}

void PowerOpt::dump_toggled_sets()
{
  map<string, pair< int, pair <int, float> > >::iterator it;
  for(it = toggled_sets_counts.begin(); it != toggled_sets_counts.end(); it++)
  {
     toggle_info_file << it->first << " --> " << it->second.second.first << " --> " << it->second.second.second << endl;
  }
}

int PowerOpt::parseVCD_mode_15_new (string vcd_file_name, designTiming  *T, int parse_cyc, int cycle_offset)
{
  bool bit_blasted_vcd = false;
  int cycle_num = 0 ;
  ifstream vcd_file ;
  //missed_nets << "New_cycle " << parse_cyc << endl;
  vector< pair<string, string> > toggled_nets; // (name, value)
  vcd_file.open(vcd_file_name.c_str());
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  if (vcd_file.is_open())
  {
    string line;
    vector<string> line_contents;
    vector<string> hierarchy;
    uint64_t time;
    uint64_t min_time = vcdStartCycle,  max_time = vcdStopCycle;
    bool ignore_module = false;
    bool valid_time_instant = false;
    while (getline(vcd_file, line))
    {
      if (line.compare(0,6,"$scope") == 0) {
        string module_name;

        tokenize (line, ' ', line_contents);

        module_name = line_contents[2];
        hierarchy.push_back(module_name);
          assert (line_contents[3] == "$end");
            //cout << "Checking " << module_name << endl;
          if (modules_of_interest.find(module_name) != modules_of_interest.end()){
            /* DEBUG
            for (int i = 0; i < line_contents.size(); i++)
            {
               cout << line_contents[i] << endl;
            }*/
            cout << "Interested in module " << module_name << endl;
            ignore_module = false;
          }
          else ignore_module = true;
      }
      else if (line.compare(0, 8, "$upscope") == 0) {
        hierarchy.pop_back();

        tokenize (line, ' ', line_contents);

        assert (line_contents[1] == "$end");

        /* DEBUG
         * for (int i = 0; i < line_contents.size(); i++)
        {
           cout << line_contents[i] << endl;
        }*/
      }
      else if (!ignore_module && line.compare(0, 4, "$var") == 0) {
      //else if (!ignore_module && line.compare(0, 4, "$var") == 0)
        tokenize (line, ' ', line_contents);
        string hierarchy_string = add_delim (hierarchy, '/', 2);

        int lc_size = line_contents.size();
        assert (line_contents[lc_size-1] == "$end");

        if (line_contents[1] == "wire") // other cases possible, but ignored
        {
          int bit_width = atoi(line_contents[2].c_str());
          if (bit_blasted_vcd)
            assert (bit_width == 1); // make sure the design is bit_blasted
          if (bit_width == 1)
          {
            //assert (lc_size == 6);
            string abbrev = line_contents[3];
            string true_net_name = hierarchy_string;
            if(lc_size == 6)
            {
              true_net_name.append(line_contents[4]);
              net_dictionary.insert(pair<string, string> (abbrev, true_net_name));
              debug_file << true_net_name << endl;
            }
            else if (lc_size == 7)
            {
              true_net_name.append(line_contents[4]);
              true_net_name.append(line_contents[5]);
              net_dictionary.insert(pair<string, string> (abbrev, true_net_name));
              debug_file << true_net_name << endl;
            }
            else
            {
              assert(0);
            }
          }
          else
          {
            assert (lc_size == 7);
            string abbrev = line_contents[3];
            string true_bus_name = hierarchy_string;
            true_bus_name.append(line_contents[4]);

            int bus_width_str_size = line_contents[5].length();
            string bus_width_str = line_contents[5].substr(1, bus_width_str_size-2);

            vector<string> bus_width;
            tokenize(bus_width_str, ':', bus_width);
            assert(bus_width.size() == 2);

            int msb = atoi((bus_width[0].c_str()));
            int lsb = atoi((bus_width[1].c_str()));
            //cout << abbrev << ", " << true_bus_name << " ( " << msb << " : " << lsb << " ) " << endl;

            Bus bus (true_bus_name, msb, lsb);
            bus_dict.insert(pair <string, Bus> (abbrev, bus));
          }
        }
      }
      else if (line.compare(0, 15, "$enddefinitions") == 0) {
        assert(hierarchy.size() == 0);
        cout << " Net Dictionary size is " << net_dictionary.size() << endl;
        cout << " Bus Dictionary size is " << bus_dict.size() << endl;
        //exit(0);
      }
      else if (line.compare(0,1, "#") == 0) {
        if (exeOp > 14 && exeOp < 18)
        {
          handle_toggled_nets(toggled_nets, T, cycle_num, time);// handle all toggled nets from previous cycle
        }
        if (exeOp > 22 && exeOp < 24)
        {
          handle_toggled_nets_new(toggled_nets, T, cycle_num, time);// handle all toggled nets from previous cycle
        }
        time = strtoull(line.substr(1).c_str(), NULL, 10);
        //if (time <= max_time && time >= min_time && (time %5000 == 0) )
        if (time == vcdCheckStartCycle)  { Net::flip_check_toggles(); cout << " CHECKING TOGGLES" << endl; }
//        //if (time == 333885000)  { Net::flip_check_toggles(); cout << " CHECKING TOGGLES" << endl; }// mult
//        //if (time == 315740000 )  Net::flip_check_toggles(); // intFilt
//        //if (time == 282650000 )  Net::flip_check_toggles(); // tea8
//        //if (time == 282610000 )  Net::flip_check_toggles(); // tHold
//        if (time == 296780000 )  Net::flip_check_toggles(); // intAVG
//        //if (time == 282550000 ) { Net::flip_check_toggles(); cout << " CHECKING TOGGLES " << endl ; }// div
//        //if (time == 290260000 ) { Net::flip_check_toggles(); cout << " CHECKING TOGGLES " << endl ; }  //binSearch
//        //if (time == 289155000 ) { Net::flip_check_toggles(); cout << " CHECKING TOGGLES " << endl ; } //inSort
//        //if (time == 286390000 )  Net::flip_check_toggles();// rle
//        //if (time == 282710000 )  Net::flip_check_toggles(); // rle_test
//        //if (time == 189870000 )  Net::flip_check_toggles(); // RTOS_intAVG
//           //if (time == 282550000 )  Net::flip_check_toggles();
         if (time <= max_time && time >= min_time )
         {
           //cout << "TIME IS " << time << endl;
//         missed_nets << "TIME IS " << time << endl;
           cycle_num++;
           valid_time_instant = true;
           debug_file << "TIME IS " << time << endl;
           //cout << "TIME IS " << time << endl;
           //toggled_nets_file << "TIME IS " << time << endl;
         }
         else
         {
            valid_time_instant = false;
            //cout << " Skipping cycle " << time << endl;
            if (time > max_time)
              break;
         }
      }
      else if (valid_time_instant) { // parse for time values of interest
        string value = line.substr(0, 1);
        string abbrev;
        if (value == "b") { // bus // This code is deprecated since there were issues with sending '[' over the socket to primetime.
                                   // instead we use vcdpost to remove all buses from the vcd file.
          int pos = line.find(" ",0);
          value  = line.substr(1, pos-1);
          abbrev = line.substr(pos+1);
          map<string, Bus>::iterator  it = bus_dict.find(abbrev);
          //int size = bus_dict.size();
          if (it != bus_dict.end())
          {
             Bus& bus = it->second;
             int prev_value_size = bus.prev_value.length();
             assert ( prev_value_size == value.length());
             //for (int i = bus.lsb ; i < (bus.lsb + prev_value_size) ; i++)
             for (int i = 0 ; i < prev_value_size ; i++)
             {
               int bit_index = bus.msb - i;
               if (bus.prev_value[i] != value[i]) // compare each bit and find toggled bits
               {
                 string net_name = bus.name;
                 //int bit_index = prev_value_size - 1 - i + bus.lsb;
                 net_name.append("[");
                 char buffer[2]; // 2 digits are enough to hold the number 64 (max possible size of a bus till date);
                 sprintf(buffer, "%d", bit_index);
                 net_name.append(buffer);
                 net_name.append("]");
                 Net * net =  nets[netNameIdMap[net_name]];
                 if(exeOp == 22)
                 {
                   net->setSimValue(value);
                 }
                 else
                 {
                   net->updateValue(value);
                   toggled_nets.push_back(make_pair(net_name, value));
                 }
                 //toggled_nets_file << net_name << " -- > " << value << endl;
               }
             }
            bus.prev_value = value;
          }
        }
        else if (value == "$") { // net
          assert (line.compare(0, 9, "$dumpvars") == 0); // I am sure that this line is $dumpvars
          valid_time_instant = false; // this is just a dump of all vars with probably 'x' as values, so we don't care
          //exit(0);
        }
        else {
          abbrev = line.substr(1);// first character is the value
          //cout << " At valid time : " << time << endl;
          map<string, string> :: iterator it = net_dictionary.find(abbrev);
          if (it != net_dictionary.end())
          {
            string net_name = net_dictionary[abbrev];
            vector<string> tokens;
            tokenize(net_name, '/', tokens);
            net_name = tokens[1];
            Net * net = nets[netNameIdMap[net_name]];
            if(exeOp == 22)
            {
              net->setSimValue(value);
            }
            else
            {
              net->updateValue(value);
              toggled_nets.push_back(make_pair(net_name, value));
            }
            //cout << net_name << " -- > "  << value << endl;
            // cout << " Toggled Net : " << net_name << endl;
          }
//        IMPORTANT: we miss eseveral nets because they belong to modules that are not of interest.
//        Every gate is a module in the vcd file.
//          else
//          cout << "Missed" <<  abbrev << endl;
        }
      }
      line_contents.clear();
    }
    // There is one inside the while loop
    //handle_toggled_nets(toggled_nets, T, cycle_num, time);// handle all toggled nets from previous cycle
    //handle_toggled_nets_new(toggled_nets, T, cycle_num, time); // FOR LAST CYCLE
    cout << "Total parsed cycles (all benchmarks) = " << cycle_num << endl;
    //cout << "Unique toggle groups count = " << toggled_terms_counts.size() << endl;
    cout << "Unique toggle groups count = " << toggled_sets_counts.size() << endl;
//    cout << "Not Toggled gates size is " << not_toggled_gate_map.size() << endl;

    if (exeOp == 17) {
      //cout << "Unique toggle groups count = " << toggled_gate_set.size() << endl;
      cout << "Unique (subsetted, during insert) toggle groups count = " << tree->num_sets() << endl;
      tree->remove_subsets();
      cout << "Unique (subsetted) toggle groups count = " << tree->num_sets() << endl;
      //exit(0);
    }
/*    if (exeOp == 16 || exeOp == 15) {
      //cout << "Unique toggle groups count = " << toggled_gate_set.size() << endl;
      cout << "Unique toggle groups count = " << toggled_terms_counts.size() << endl;
      cout << "Not Toggled gates size is " << not_toggled_gate_vector.size() << endl;
    }*/
  }
  missed_nets.close();
//  dump_toggle_counts();
//  histogram_toggle_counts();

  update_profile_sizes();
  print_toggle_profiles();

  //estimate_correlation();
  //print_correlations();
  return cycle_num;
}

// parse the VCD file and extract toggle information
  int PowerOpt::parseVCD_mode_15(string VCDfilename, designTiming *T, int parse_cyc, int cycle_offset)
{
  FILE * VCDfile;
  VCDfile = fopen(VCDfilename.c_str(), "r");
  if(VCDfile == NULL){
    printf("ERROR, could not open VCD file: %s\n",VCDfilename.c_str());
    exit(1);
  }

  char str[2048];
  char str1[20],str2[20],str3[20];           // dummy placeholder strings
  char abbrev[MAX_ABBREV];                   // abbreviated net name
  char net_name[MAX_NAME];                   // full net name

  char net_index[16];                        // there can be an index for multi-bit nets
  char * end_index;                          // the end of the net index
  char * returnval;                          // to check if we reached the end of a file
  int num_read=0;                            // number of values read in fscanf
  int timescale;
  char time_unit[10];                        // time unit for the timescale
  int cycle_num=0;                           // the current cycle number
  int cycle_time=0;                          // the current time
  int num_toggled=0;                         // the number of nets that toggled in a cycle (minus the clocks)
  int num_clks=0;                            // the number of clocks (that we ignore)
  char clk_names[MAX_CLKS][MAX_ABBREV];      // the abbreviated names of the clocks
  int num_nets=0;                            // the number of nets in the circuit
  int j;                                     // a loop counter
  int net_is_clk = 0;                        // tells whether the current net is a clock signal
  vector<string> toggled_nets(VECTOR_INC);   // a list of net names that toggled in a cycle
  //static int pathID = 0;                     // for Extraction of toggled paths

  //map<string,string> net_dictionary;         // dictionary of net abbreviation to net name
  string strng;
  map<string,string>::iterator gate_lookup;  // a pointer to the gate's entry in the source_gate_dictionary

  //Gate *g;

  do{
    fgets(str, 2048, VCDfile);
  }while(strstr(str,"timescale") == NULL);

  fscanf(VCDfile, "%d %s", &timescale, &time_unit);

  // seek to the beginning of the dut module among many modules
  const char* dut_name = dutName.c_str();
  do{
    fgets(str, 2048, VCDfile);
  }while(strstr(str, dut_name) == 0 );

  num_read = fscanf(VCDfile, "%s %s %s %s %s %s", &str1,&str2,&str3,&abbrev,&net_name,&net_index);

  while(strstr(str1,"var") != NULL){
    num_nets++;

    if( net_index[0] != '$' ){ // multi-bit net handling
      end_index = strchr(net_index,']');
      strcat(net_name, "\\[");
      strncat(net_name, &net_index[1], end_index-(&net_index[1]) );
      strcat(net_name, "\\]");
    }

    // add the entry <abbrev,name> to the net dictionary
    strng = abbrev;
    net_dictionary[strng] = net_name;

    if( strstr(net_name,"clk") != NULL){
      returnval = strchr(abbrev,'\n');
      if(returnval > 0){ *returnval = '\0';} // null terminate the string
      strcpy(clk_names[num_clks],abbrev);
      num_clks++;
    }

    fgets(str, 2048, VCDfile);
    num_read = fscanf(VCDfile, "%s %s %s %s %s %s", &str1,&str2,&str3,&abbrev,&net_name,&net_index);
  }
  //printf("Total nets = %d\n",num_nets);

  // seek to where the inital values of all nets are dumped -- I don't care about values, just toggles
  if ((strstr(str1, "dumpvars") != NULL) && (strstr(str2, "dumpvars") != NULL) && (strstr(str3, "dumpvars") != NULL) && (strstr(abbrev, "dumpvars") != NULL) && (strstr(net_name, "dumpvars") != NULL) && (strstr(net_index, "dumpvars") != NULL) )
  {// dumpvars not found in the last parse
    do{
        fgets(str, 2048, VCDfile);
    }while(strstr(str,"dumpvars") == NULL);
  }

  // seek to where the toggle information starts
  do{
    fgets(str, 2048, VCDfile);
  }while(str[0] != '#');

  // now, for each cycle in which nets toggle (ignore clocks):
  // record the toggled nets
  // traverse the netlist with toggled nets to extract toggled paths
  // record the current cycle as a toggle cycle for all extracted paths

  // the rest of the file is toggle information, so keep going until the end if parse_cyc is negative
  // or else, go until parse_cyc cycles have been parsed
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  bool once = true;
  while( (!feof(VCDfile)) && ((cycle_num < parse_cyc) || (parse_cyc < 0)) ){ // parse_cyc is the vcdCycle given in the cmd file

    // we only care about toggles at the positive clock edge (when more than just the clock toggles)
    // current string value is a cycle time, check if it is the positive clock edge (time % 2000 == 1000)

    cycle_time = atoi(&str[1]);
    //printf("%d\n",cycle_time);

    if(((vcdStopCycle < 0) || (cycle_time <= vcdStopCycle)) && (cycle_time >= vcdStartCycle) ) {
      // this is a positive clock edge, increase the cycle count
      if (once)
      {
        cout << " Completed initial skip" << endl;
        cout << "STARTED PARSING THE VCD" << endl;
        cout << "cycle_time is  " << cycle_time << endl;
        once = false;
      }
      cycle_num++;
      //printf("\n Toggles in cycle: %d and with cycle time : %d ", cycle_num, cycle_time);
      // parse toggle information
      num_toggled = 0;
      returnval = fgets(str, 2048, VCDfile);
      //printf("parsing cycles now. Current cycle is %d \n", cycle_num);

      while( (str[0] != '#') && (returnval != NULL) ){
        // extract the net abbreviation from this string
        // first char is the toggled value and can be ignored, rest is the net abbrev

        ////////////////////////////////////
        // NOTE: There is an unresolved problem in some VCD files such that some multi-bit signals are not expanded.
        //       This causes problems for VCD parsing.
        //       The signal can be treated as a single net for all bits or ignored,
        //       until there is a resolution for the VCD format.
        //       Currently, I am ignoring these signals. (unclear how to trace paths through a multibit)
        //       They can be identified in the following ways:
        //       1) There is a space between the value and the abbreviation.
        //       2) The value starts with 'b'.
        //       I will use (1) for ID, since I don't know if (2) is true in general.
        ///////////////////////////////////////
        // If there is a space in str, then discard it (treat it as a clock, which gets ignored)
        if( strchr(str,' ') != NULL ){
          net_is_clk = 1;
        }

        if( strstr(net_name,clockName.c_str()) != NULL){
          net_is_clk = 1;
        }

        ///////////////////////////////////////

        // strcmp requires strings to be null-terminated
        returnval = strchr(str,'\n');
        if(returnval > 0){*returnval = '\0';}

        // if this is not a clock, add it to the list of toggled nets
        if(net_is_clk < 1){
          //strcpy(toggled_nets[num_toggled],&str[1]);
          strng = &str[1];
          string true_net_name = net_dictionary[strng];
          if (!true_net_name.empty())
          {
            toggled_nets[num_toggled] = true_net_name;
            num_toggled++;
          }

          // if the vector is not big enough, resize it by adding VECTOR_INC more elements
          if(toggled_nets.size() == num_toggled){
            toggled_nets.resize(toggled_nets.size() + VECTOR_INC);
            //printf("Resized vector to size %d\n",toggled_nets.size());
          }
        }

        net_is_clk = 0;
        returnval = fgets(str, 2048, VCDfile); // get the next value
      }

      if( (returnval != NULL) && (num_toggled > 0) ){
        string  cellstr;
        int     CurInstId;
        for(j = 0; j < num_toggled; j++){
          //only lookup the source gate from PT if it is not in the dictionary
          gate_lookup = source_gate_dictionary.find(toggled_nets[j]);
          if( gate_lookup == source_gate_dictionary.end() ){
//            string test_string ("count[0]");
//            string test_cell = T->getCellFromNet(test_string);
//            cout << test_cell << endl;
            string net_name = toggled_nets[j];
            cellstr =  T->getCellFromNet(net_name);
            source_gate_dictionary[net_name] = cellstr;
            /*// TESTCODE
//            string net_string = "clock_module_0_por_a";
//            string test_cell = T->getCellFromNet(net_string);
//            printf("the cell of the net %s is %s \n", net_string, test_cell);

//            net_string = "watchdog_0_wdtcnt[1]";
//            test_cell = T->getCellFromNet(net_string);*/
          }else{
            cellstr = (*gate_lookup).second;
          }
          if ( cellstr != "NULL_CELL" ) {
            if ( cellstr == "MULTI_DRIVEN_NET" )
            {
              cout << "[ERROR] This net " << net_name << " is driven by multiple cells" << endl;
            }
            map<string, Gate*>::iterator gate_lookup2 =  gate_name_dictionary.find(cellstr);
            CurInstId = gateNameIdMap[cellstr];
            m_gates[CurInstId]->setToggled(true, "x"); // "x" is set as default. not to be used
//            if (gate_lookup2 != gate_name_dictionary.end())
//            {
//              Gate* g = gate_lookup2->second;
////              g->setToggled(true);
////              printf("%s, ", cellstr);
//            }
//            else
//            {
//              printf("Failed lookup of gate from gate_name \n");
//              exit(0);
//            }
          }
          else{
          }
        }
//        extractPaths_mode_15(cycle_num+cycle_offset,pathID, T);
        // check_for_toggles(cycle_num, cycle_time);
        if (exeOp == 15)
          check_for_flop_toggles(cycle_num, cycle_time, T);
        else if (exeOp == 16)
          check_for_flop_toggles_fast(cycle_num, cycle_time, T);
        else if (exeOp == 17)
          check_for_flop_toggles_fast_subset(cycle_num, cycle_time, T);
        else {
          cout << "Wrong ExeOp given " << endl;
          exit(0);
        }
        // trace_toggled_path(cycle_num, cycle_time);
        clearToggled();
      }
    } else{
      //cout << " Skipping cycle " << cycle_time << endl;
      do{
        returnval = fgets(str, 2048, VCDfile);
      }while( (str[0] != '#') && (returnval != NULL) );
    }
  }


  fclose (VCDfile);
  // return the number of simulation cycles
  cout << "Total parsed cycles (all benchmarks) = " << cycle_num+cycle_offset << endl;
  if (exeOp == 17) {
    cout << "Unique (subsetted, during insert) toggle groups count = " << tree->num_sets() << endl;
    tree->remove_subsets();
    cout << "Unique (subsetted) toggle groups count = " << tree->num_sets() << endl;
  }
  if (exeOp == 16 || exeOp == 15) {
    cout << "Unique toggle groups count = " << toggled_gate_set.size() << endl;
    cout << "Not Toggled gates size is " << not_toggled_gate_vector.size() << endl;
  }


  /*// free up all dynamically allocated memory

     for(j = 0; j < toggled_nets.size(); j++){
     delete[] toggled_nets[j];
     }*/

  return cycle_num;
}

// parse the VCD file and extract toggle information
  int PowerOpt::parseVCD(string VCDfilename, designTiming *T, int parse_cyc, int cycle_offset)
{
  FILE * VCDfile;
  VCDfile = fopen(VCDfilename.c_str(), "r");
  if(VCDfile == NULL){
    printf("ERROR, could not open VCD file: %s\n",VCDfilename.c_str());
    exit(1);
  }

  char str[2048];
  char str1[20],str2[20],str3[20];    // dummy placeholder strings
  char abbrev[MAX_ABBREV];       // abbreviated net name
  char net_name[MAX_NAME];     // full net name
  char net_index[16];     // there can be an index for multi-bit nets

  char * end_index;      // the end of the net index
  char * returnval;      // to check if we reached the end of a file
  int num_read=0;        // number of values read in fscanf
  int timescale;
  char time_unit[10];    // time unit for the timescale
  int cycle_num=0;       // the current cycle number
  int cycle_time=0;      // the current time
  int num_toggled=0;     // the number of nets that toggled in a cycle (minus the clocks)
  int num_clks=0;        // the number of clocks (that we ignore)
  char clk_names[MAX_CLKS][MAX_ABBREV];  // the abbreviated names of the clocks
  int num_nets=0;        // the number of nets in the circuit
  int j;                 // a loop counter
  int net_is_clk = 0;    // tells whether the current net is a clock signal
  vector<string> toggled_nets(VECTOR_INC); // a list of net names that toggled in a cycle
  double t0 = 0.0;
  /*
     for(j = toggled_nets.size()-VECTOR_INC; j < toggled_nets.size(); j++){
     toggled_nets[j] = new char[MAX_ABBREV];
     }
   */

  static int pathID = 0; // for Extraction of toggled paths

  map<string,string> net_dictionary;  // dictionary of net abbreviation to net name
  string strng;
  map<string,string> source_gate_dictionary;  // dictionary of net name to source gate
  map<string,string>::iterator gate_lookup;   // a pointer to the gate's entry in the source_gate_dictionary


  // make gate_outpin_dictionay
  Gate *g;
  string out_pin;
  for (int i = 0; i < getGateNum(); i ++)
  {
    g = m_gates[i];
    out_pin = T->getOutPin(g->getName());
    gate_outpin_dictionary[g->getName()] = out_pin;
  }

  // get the timescale -- we may not need this info
  do{
    fgets(str, 2048, VCDfile);
  }while(strstr(str,"timescale") == NULL);

  fscanf(VCDfile, "%d %s", &timescale, &time_unit);
  dprintf("Timescale: %d %s\n",timescale, time_unit);

  // seek to the beginning of the dut module
  const char* dut_name = dutName.c_str();
  printf(" dut_name is %s" , dut_name);
  do{
    fgets(str, 2048, VCDfile);
  }while(strstr(str, dut_name) == 0 );

  // Crossreference of net abbreviation to net name is here
  // make a dictionary of net abbrev to net name so I can pass net names to extractPaths
  // I can possibly use Net.h here

  num_read = fscanf(VCDfile, "%s %s %s %s %s %s", &str1,&str2,&str3,&abbrev,&net_name,&net_index);

  while(strstr(str1,"var") != NULL){
    num_nets++;

    // net_index may or may not exist
    // if there is an index, we need to append it to net_name using strcat
    if( net_index[0] != '$' ){
      end_index = strchr(net_index,']');
      strcat(net_name, "\\[");
      strncat(net_name, &net_index[1], end_index-(&net_index[1]) );
      strcat(net_name, "\\]");
      //printf("net with index: %s\n",net_name);
    }

    // add the entry <abbrev,name> to the net dictionary
    strng = abbrev;

    if (internal_module_of_interest.size())
    {
       string temp(net_name);
       temp = internal_module_of_interest+"/"+temp;
       net_name[temp.size()] = 0;
       memcpy(net_name, temp.c_str(), temp.size());
    }

    net_dictionary[strng] = net_name;
    //dprintf("Added %s,%s to dictionary\n",abbrev,(net_dictionary[strng]).c_str());

    // Identify clock nets
    if( strstr(net_name,clockName.c_str()) != NULL){
      returnval = strchr(abbrev,'\n');
      if(returnval > 0){ *returnval = '\0';}
      strcpy(clk_names[num_clks],abbrev);
      dprintf("Found clock %s in netlist with abbreviation %s\n",net_name,clk_names[num_clks]);
      num_clks++;
    }

    // clear the end of the line and read the next
    fgets(str, 2048, VCDfile);
    num_read = fscanf(VCDfile, "%s %s %s %s %s %s", &str1,&str2,&str3,&abbrev,&net_name,&net_index);
  }
  dprintf("Total nets = %d\n",num_nets);


  /*
  // seek to where the inital values of all nets are dumped -- I don't care about values, just toggles
  do{
  fgets(str, 2048, VCDfile);
  }while(strstr(str,"dumpvars") == NULL);
   */

  // seek to where the toggle information starts
  do{
    fgets(str, 2048, VCDfile);
  }while(str[0] != '#');

  // now, for each cycle in which nets toggle (ignore clocks):
  // record the toggled nets
  // traverse the netlist with toggled nets to extract toggled paths
  // record the current cycle as a toggle cycle for all extracted paths

  // the rest of the file is toggle information, so keep going until the end if parse_cyc is negative
  // or else, go until parse_cyc cycles have been parsed
  while( (!feof(VCDfile)) && ((cycle_num < parse_cyc) || (parse_cyc < 0)) ){

    // we only care about toggles at the positive clock edge (when more than just the clock toggles)
    // current string value is a cycle time, check if it is the positive clock edge (time % 2000 == 1000)

    cycle_time = atoi(&str[1]);
    //dprintf("%d\n",cycle_time);

    //if( (cycle_time % clockPeriod) == 2500 ){} // HARI NEED TO GENERALIZE THIS. 1000 is used to detect positive edge ? not generic
    if(((cycle_time == 1000) || (cycle_time == 9300) || ( (cycle_time % 500) == 0 )) && (cycle_time <= vcdStopCycle) && (cycle_time >= vcdStartCycle))  { // HARI NEED TO GENERALIZE THIS. 1000 is used to detect positive edge ? not generic
      // this is a positive clock edge, increase the cycle count
      cycle_num++;
      // parse toggle information
      num_toggled = 0;
      returnval = fgets(str, 2048, VCDfile);

      while( (str[0] != '#') && (returnval != NULL) ){
        // extract the net abbreviation from this string
        // first char is the toggled value and can be ignored, rest is the net abbrev

        ////////////////////////////////////
        // NOTE: There is an unresolved problem in some VCD files such that some multi-bit signals are not expanded.
        //       This causes problems for VCD parsing.
        //       The signal can be treated as a single net for all bits or ignored,
        //       until there is a resolution for the VCD format.
        //       Currently, I am ignoring these signals. (unclear how to trace paths through a multibit)
        //       They can be identified in the following ways:
        //       1) There is a space between the value and the abbreviation.
        //       2) The value starts with 'b'.
        //       I will use (1) for ID, since I don't know if (2) is true in general.
        ///////////////////////////////////////
        // If there is a space in str, then discard it (treat it as a clock, which gets ignored)
        if( strchr(str,' ') != NULL ){
          net_is_clk = 1;
        }

        ///////////////////////////////////////

        // strcmp requires strings to be null-terminated
        returnval = strchr(str,'\n');
        if(returnval > 0){*returnval = '\0';}

        // if the net is a clock, ignore it, else add it to the list of toggled nets
        for(j = 0; j < num_clks; j++)
        {
          if( strcmp(clk_names[j], &str[1]) == 0 )
          {
            //dprintf("Ignoring clock %s\n",&str[1]);
            net_is_clk = 1;
            break;
          }
        }

        // if this is not a clock, add it to the list of toggled nets
        if(net_is_clk < 1){
          //strcpy(toggled_nets[num_toggled],&str[1]);
          strng = &str[1];
          toggled_nets[num_toggled] = net_dictionary[strng];
          num_toggled++;

          //dprintf("Net %s toggled at time %d\n",(toggled_nets[num_toggled-1]).c_str(),cycle_time);

          // if the vector is not big enough, resize it by adding VECTOR_INC more elements
          if(toggled_nets.size() == num_toggled){
            toggled_nets.resize(toggled_nets.size() + VECTOR_INC);
            /*
               for(j = toggled_nets.size()-VECTOR_INC; j < toggled_nets.size(); j++){
               toggled_nets[j] = new char[MAX_ABBREV];
               }
             */
            dprintf("Resized vector to size %d\n",toggled_nets.size());
          }
        }

        net_is_clk = 0;
        returnval = fgets(str, 2048, VCDfile); // get the next value
      }
      //dprintf("Cycle %d at time %d, %d toggled nets\n",cycle_num,cycle_time,num_toggled);

      // feed the toggle information to the path extraction function (if there is any -- check for NULL returnval first)
      if( (returnval != NULL) && (num_toggled > 0) ){
        //cout << "toggled number= " << num_toggled << endl;
        //cout << "cycle number= " << cycle_num << endl;
        string  cellstr;
        int     CurInstId;
        //int     CurNetId;
        for(j = 0; j < num_toggled; j++){
          //only lookup the source gate from PT if it is not in the dictionary
          gate_lookup = source_gate_dictionary.find(toggled_nets[j]);
          // if the return value is the end of the dictionary, there is no entry yet for this net
          if( gate_lookup == source_gate_dictionary.end() ){
            // add the entry to the dictionary
            cellstr =  T->getCellFromNet(toggled_nets[j]);
            source_gate_dictionary[toggled_nets[j]] = cellstr;
          }else{
            cellstr = (*gate_lookup).second;
          }
          //cout << "net : " << toggled_nets[j];
          //cout << " -- cell : " << cellstr << endl;
          if ( cellstr != "NULL_CELL" ) {
            CurInstId = gateNameIdMap[cellstr];
            m_gates[CurInstId]->setToggled(true, "x"); // "x" is sent as default. usage here is deprecated
            //ofstream gate_toggled;
//            gate_toggled.open("my_output.txt", ios::app);
//            gate_toggled << "Id is " << CurInstId << " gate name is " << m_gates[CurInstId]->getName() << endl;
            //std::cout << "Id is " << CurInstId << "gate name is " << m_gates[CurInstId]->getName() << endl;
            //gate_toggled.close();
          }
          else{
            //    CurNetId = subnetNameIdMap[toggled_nets[j]];
            //    cout << "PI : " << subnets[CurNetId]->getName() << endl;
            // if there is no source gate, then the toggled net is a PI
          }
        }
        TICK();
        extractPaths(cycle_num+cycle_offset,pathID);
        TOCK(path_time);
        clearToggled();
      }
    } else{
      // since this is a negative clock edge, seek to the next clock time
      do{
        returnval = fgets(str, 2048, VCDfile);
      }while( (str[0] != '#') && (returnval != NULL) );
    }
  }

  fclose (VCDfile);
  // return the number of simulation cycles
  cout << "Total parsed cycles (all benchmarks) = " << cycle_num+cycle_offset << endl;
  cout << "All toggled paths = " << toggledPaths.size() << endl;
  cout << "Time taken " << path_time << endl;
  return cycle_num;

  // free up all dynamically allocated memory
  /*
     for(j = 0; j < toggled_nets.size(); j++){
     delete[] toggled_nets[j];
     }
   */
}

void PowerOpt::check_for_flop_toggles(int cycle_num, int cycle_time, designTiming * T)
{
    cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
    toggle_info_file << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
    //T->resetPathsThroughAllCells();
    int count_not_toggled = 0;
    int count_toggled = 0;
    vector<string> not_toggled_gates;
    vector< pair<string, string> > toggled_gates;
    for (int i = 0; i < getGateNum(); i ++) {
      Gate* g = m_gates[i];
      string gate_name = g->getName();
      if(g->isToggled())
      {
        count_toggled++;
        //toggle_info_file << gate_name << ", ";
         toggled_gates.push_back(make_pair(gate_name, g->getToggledTo()));
          toggle_info_file << gate_name << " " << g->getToggledTo() << endl;
      }
      else
      {
        count_not_toggled++;
        unique_not_toggle_gate_sets << gate_name << ", " ;
        not_toggled_gates.push_back(gate_name);
      }
    }
    not_toggled_gate_vector.push_back(not_toggled_gates);
    toggled_gate_vector_rise_fall.push_back(toggled_gates);
    cycles_of_toggled_gate_vector.push_back(cycle_time);
//    cout << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
//    cout << "[TOG] count_toggled is " <<  count_toggled << endl;
//    toggle_info_file << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
//    toggle_info_file << "[TOG] count_toggled is " <<  count_toggled << endl;
    //toggle_info_file << endl;
    toggle_info_file << "************" << endl;
}

void PowerOpt::read_unt_dump_file()
{
  ifstream file;
  string line;
  file.open(untDumpFile.c_str());
  if (file.is_open())
  {
    while (getline(file,line))
    {
      stringstream ssl(line);
      string gate;
      vector<string> not_toggled_gates;
      while (getline(ssl, gate, ',')) {
        not_toggled_gates.push_back(gate);
        //cout << "Reading " << gate  << endl;
      }
      cout << not_toggled_gates.size() << endl;
      //not_toggled_gate_map.insert(std::pair<vector<string>, float>(not_toggled_gates, -1));
      not_toggled_gate_vector.push_back(not_toggled_gates);
    }
  }
  else {
    cout << " Couldn't Open File" << untDumpFile << endl;
    exit(0);
  }
}


void PowerOpt::read_ut_dump_file()
{
  cout << "READING Dump file  " << endl;
  ifstream file;
  string line;
  file.open(utDumpFile.c_str());
  if (file.is_open())
  {
    while (getline(file,line))
    {
      vector<string> tokens;
      tokenize(line, ':', tokens);
      assert(tokens.size() == 2);
      int id = atoi(tokens[0].c_str());
      stringstream ssl(tokens[1]);
      string gate;
      vector<string> toggled_gates;
      while (getline(ssl, gate, ',')) {
        toggled_gates.push_back(gate);
        //cout << "Reading " << gate  << endl;
      }
      //cout << toggled_gates.size() << endl;
      //not_toggled_gate_map.insert(std::pair<vector<string>, float>(not_toggled_gates, -1));
      toggled_gate_vector.push_back(toggled_gates);
    }
  }
  else {
    cout << " Couldn't Open File" << untDumpFile << endl;
    exit(0);
  }
}

void PowerOpt::reset_all (designTiming* T)
{
    for (int i = 0; i < getGateNum(); i ++) {
        Gate *gate = m_gates[i];
        string gate_name = gate->getName();
        if (gate->isendpoint())
        {
          T->resetPathTo(gate_name);
          T->resetPathFrom(gate_name);
        }
        else
        {
          T->resetPathThrough(gate_name);
        }
    }
}

void PowerOpt::dump_units(){

  list < vector <int> >& collection = tree->get_collection();
  list <vector < int > > :: iterator it;
  for(it = collection.begin(); it != collection.end(); it++)
  {
    vector<int> & unit = *it;
    for (int i =0; i < unit.size(); i++)
    {
      int index = unit[i];
      string gate_name = m_gates[index]->getName();
      units_file << gate_name << ", " ;
    }
    units_file << endl;
  }

}

void PowerOpt::dump_slack_profile()
{
  if (exeOp != 16) return;
    for (int i = 0; i < per_cycle_toggled_sets.size();  i++)
    {
        map<string, pair< int, pair <int, float> > > ::iterator it = per_cycle_toggled_sets[i].second;
        slack_profile_file << " Cycle : " << per_cycle_toggled_sets[i].first << " id: " << it->second.first  << " slack:" << it->second.second.second << endl;
    }
}


void PowerOpt::dump_toggle_counts()
{
    for (int i = 0; i < getGateNum(); i ++) {
        int count = m_gates[i]->getToggleCount();
        toggle_counts_file << m_gates[i]->getName() << " :" << count << endl;
    }
}

void PowerOpt::update_profile_sizes()
{
  int max = m_gates[0]->max_toggle_profile_size;
  //int max = 0;
  for (int i = 0 ; i < getGateNum(); i++)
  {
    m_gates[i]->resizeToggleProfile(max);
  }
}

void PowerOpt::print_toggle_profiles()
{
  cout << "[PowerOpt] Printing toggle profiles" << endl;
  for (int i = 0; i < getGateNum(); i++)
  {
    m_gates[i]->printToggleProfile(toggle_profile_file);
  }
}


void PowerOpt::print_correlations()
{
  map<int, list <pair<Gate*, Gate*> > >::reverse_iterator rit;

  for (rit = corr_map.rbegin(); rit != corr_map.rend(); rit++)
  {
    list<pair<Gate* , Gate*> > my_list = rit->second;
    int size = my_list.size();
    if (size > 5000) size = 5000;
    cout << "Next" << endl;
    for (int i = 0; i < size; i ++)
    {
      cout << rit->first << " : " << my_list.front().first->getName() << ", " << my_list.front().second->getName() << endl;
      my_list.pop_front();
    }
  }
}


void PowerOpt::estimate_correlation()
{

  cout << "[PowerOpt] Estimating Toggle Correlations" << endl;
  for(int i = 0; i < getGateNum(); i++)
  {
    Gate* gate1 = m_gates[i];
    if (gate1->notInteresting()) continue;
    for (int j = i+1; j < getGateNum(); j++)
    {
      Gate* gate2 = m_gates[j];
      if (gate2->notInteresting()) continue;
      int correlation = gate1->getToggleCorrelation(gate2);
      corr_map[correlation].push_back(make_pair (gate1, gate2));
      //cout <<  gate1->getName() << ", " << gate2->getName() <<  " : " << correlation << endl;
    }
  }

}


void PowerOpt::histogram_toggle_counts()
{
    map<int, int> mem_backbone_0   ;
    map<int, int> frontend_0       ;
    map<int, int> sfr_0            ;
    map<int, int> dbg_0            ;
    map<int, int> watchdog_0       ;
    map<int, int> multiplier_0     ;
    map<int, int> execution_unit_0 ;
    map<int, int> clock_module_0   ;
    map<int, int> glue             ;
    map<int, int> total            ;

    map<int, int>::iterator mb_it   = mem_backbone_0.begin()   ;
    map<int, int>::iterator fe_it   = frontend_0.begin()       ;
    map<int, int>::iterator sfr_it  = sfr_0.begin()            ;
    map<int, int>::iterator dbg_it  = dbg_0.begin()            ;
    map<int, int>::iterator wdg_it  = watchdog_0.begin()       ;
    map<int, int>::iterator mul_it  = multiplier_0.begin()     ;
    map<int, int>::iterator eu_it   = execution_unit_0.begin() ;
    map<int, int>::iterator clk_it  = clock_module_0.begin()   ;
    map<int, int>::iterator glue_it = glue.begin()             ;
    map<int, int>::iterator tot_it  = total.begin()            ;

    cout << " Computing Histrogram" << endl;

    for (int i = 0; i < getGateNum(); i++) {

       int count = m_gates[i]->getToggleCount();
       string gate = m_gates[i]->getName();
       vector<string> hierarchy ;
       tokenize (gate, '/', hierarchy);
       /*if (hierarchy[0] == "mem_backbone_0"  )       { if ( count > mem_backbone_0  .size() ) { mem_backbone_0  .resize(count+1,0);} int val = mem_backbone_0  .at(count)+1;  mem_backbone_0   .insert (mb_it  +count, val); }
       else if (hierarchy[0] == "frontend_0"  )      { if ( count > frontend_0      .size() ) { frontend_0      .resize(count+1,0);} int val = frontend_0      .at(count)+1;  frontend_0       .insert (fe_it  +count, val); }
       else if (hierarchy[0] == "sfr_0"  )           { if ( count > sfr_0           .size() ) { sfr_0           .resize(count+1,0);} int val = sfr_0           .at(count)+1;  sfr_0            .insert (sfr_it +count, val); }
       else if (hierarchy[0] == "dbg_0" )            { if ( count > dbg_0           .size() ) { dbg_0           .resize(count+1,0);} int val = dbg_0           .at(count)+1;  dbg_0            .insert (dbg_it +count, val); }
       else if (hierarchy[0] == "watchdog_0" )       { if ( count > watchdog_0      .size() ) { watchdog_0      .resize(count+1,0);} int val = watchdog_0      .at(count)+1;  watchdog_0       .insert (wdg_it +count, val); }
       else if (hierarchy[0] == "multiplier_0" )     { if ( count > multiplier_0    .size() ) { multiplier_0    .resize(count+1,0);} int val = multiplier_0    .at(count)+1;  multiplier_0     .insert (mul_it +count, val); }
       else if (hierarchy[0] == "execution_unit_0" ) { if ( count > execution_unit_0.size() ) { execution_unit_0.resize(count+1,0);} int val = execution_unit_0.at(count)+1;  execution_unit_0 .insert (eu_it  +count, val); }
       else if (hierarchy[0] == "clock_module_0"  )  { if ( count > clock_module_0  .size() ) { clock_module_0  .resize(count+1,0);} int val = clock_module_0  .at(count)+1;  clock_module_0   .insert (clk_it +count, val); }
       else                                          { if ( count > glue            .size() ) { glue            .resize(count+1,0);} int val = glue            .at(count)+1;  glue             .insert (glue_it+count, val); }*/

       if (hierarchy[0] == "mem_backbone_0"  )       { mem_backbone_0  [count]++;}
       else if (hierarchy[0] == "frontend_0"  )      { frontend_0      [count]++;}
       else if (hierarchy[0] == "sfr_0"  )           { sfr_0           [count]++;}
       else if (hierarchy[0] == "dbg_0" )            { dbg_0           [count]++;}
       else if (hierarchy[0] == "watchdog_0" )       { watchdog_0      [count]++;}
       else if (hierarchy[0] == "multiplier_0" )     { multiplier_0    [count]++;}
       else if (hierarchy[0] == "execution_unit_0" ) { execution_unit_0[count]++;}
       else if (hierarchy[0] == "clock_module_0"  )  { clock_module_0  [count]++;}
       else                                          { glue            [count]++;}

        total[count]++;
    }

    histogram_toggle_counts_file << "Module : mem_backbone_0   " << endl; for (mb_it   = mem_backbone_0   .begin(); mb_it   != mem_backbone_0   .end(); mb_it   ++) { cout <<  mb_it   ->first  << " : " << mb_it   ->second << endl; }
    histogram_toggle_counts_file << "Module : frontend_0       " << endl; for (fe_it   = frontend_0       .begin(); fe_it   != frontend_0       .end(); fe_it   ++) { cout <<  fe_it   ->first  << " : " << fe_it   ->second << endl; }
    histogram_toggle_counts_file << "Module : sfr_0            " << endl; for (sfr_it  = sfr_0            .begin(); sfr_it  != sfr_0            .end(); sfr_it  ++) { cout <<  sfr_it  ->first  << " : " << sfr_it  ->second << endl; }
    histogram_toggle_counts_file << "Module : dbg_0            " << endl; for (dbg_it  = dbg_0            .begin(); dbg_it  != dbg_0            .end(); dbg_it  ++) { cout <<  dbg_it  ->first  << " : " << dbg_it  ->second << endl; }
    histogram_toggle_counts_file << "Module : watchdog_0       " << endl; for (wdg_it  = watchdog_0       .begin(); wdg_it  != watchdog_0       .end(); wdg_it  ++) { cout <<  wdg_it  ->first  << " : " << wdg_it  ->second << endl; }
    histogram_toggle_counts_file << "Module : multiplier_0     " << endl; for (mul_it  = multiplier_0     .begin(); mul_it  != multiplier_0     .end(); mul_it  ++) { cout <<  mul_it  ->first  << " : " << mul_it  ->second << endl; }
    histogram_toggle_counts_file << "Module : execution_unit_0 " << endl; for (eu_it   = execution_unit_0 .begin(); eu_it   != execution_unit_0 .end(); eu_it   ++) { cout <<  eu_it   ->first  << " : " << eu_it   ->second << endl; }
    histogram_toggle_counts_file << "Module : clock_module_0   " << endl; for (clk_it  = clock_module_0   .begin(); clk_it  != clock_module_0   .end(); clk_it  ++) { cout <<  clk_it  ->first  << " : " << clk_it  ->second << endl; }
    histogram_toggle_counts_file << "Module : glue             " << endl; for (glue_it = glue             .begin(); glue_it != glue             .end(); glue_it ++) { cout <<  glue_it ->first  << " : " << glue_it ->second << endl; }
    histogram_toggle_counts_file << "Module : total            " << endl; for (tot_it  = total            .begin(); tot_it  != total             .end(); tot_it ++) { cout <<  tot_it  ->first  << " : " << tot_it  ->second << endl; }

}


void PowerOpt::find_dynamic_slack_subset(designTiming *T)
{
  if (exeOp == 24)
  {
    find_dynamic_slack_pins_subset(T);
    return;
  }
  cout << "Finding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  float dynamic_slack = 10000.0;
  list < vector <int> >& collection = tree->get_collection();
  list <vector < int > > :: iterator it;
  for (it = collection.begin(); it != collection.end() ; it++)
  {
    vector <int>& unit = *it;
    T->setFalsePathsThroughAllCells();
    for (int i = 0; i < unit.size(); i ++) // this is the toggled gate unit
    {
      int index = unit[i];
      string gate_name = m_gates[index]->getName();
      T->resetPathForCell(gate_name);
//      if (gate_name_dictionary.find(gate_name)->second->isendpoint())
//      cout  << "HARI " << gate_name << endl;
    }
    //float worst_slack = T->runTimingPBA(10.0);
    float worst_slack = T->runTiming(10.0);
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      cout << "Updating Dynamic Slack" << endl;
    }
    if (worst_slack - dynamic_slack < 1.0)
    {
      T->reportTiming(-1);
      cout << "[PTLOG]";
      print_indexed_toggled_set(unit, worst_slack);
    }
    cout << "Worst slack for this 'toggled' gate set ( of size " << unit.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
  cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;
}

void PowerOpt::print_indexed_toggled_set(vector<int> & unit, float worst_slack)
{
  static bool once = false;
  if(once == false)
  {
     toggle_info_file << "THESE ARE UNITS THAT ARE CLOSE TO DYNAMIC SLACK" << endl;
     once == true;
  }
  toggle_info_file << worst_slack << " --> ";
  for (int i = 0; i < unit.size(); i++)
  {
    int index = unit[i];
    string gate_name = m_gates[index]->getName();
    toggle_info_file << gate_name << ", ";
  }
  toggle_info_file << endl;
}

void PowerOpt::find_dynamic_slack_pins_subset(designTiming* T)
{
    cout << "Finding Dynamic Slack" << endl;
    T->suppressMessage("UITE-216");
    float dynamic_slack = 10000.0;
    list <vector <int> > & collection = tree->get_collection();
    list <vector <int> > :: iterator it;
    for (it = collection.begin(); it != collection.end(); it++)
    {
        vector <int>& unit = *it;
        T->setFalsePathsThroughAllPins();
        for (int i = 0; i < unit.size(); i++)
        {
            int index = unit[i];
            int term_id = index/3;
            Terminal * term = terms[term_id];
            string term_name = term->getFullName();
            ToggleType togg_type = static_cast<ToggleType>(index%3);
            switch (togg_type)
            {
                case RISE: T->resetRisePathForTerm(term_name); break;
                case FALL: T->resetFallPathForTerm(term_name); break;
                case UNKN: T->resetPathForTerm(term_name); break;
                default : assert(0);
            }
        }

        // IMPORTANT TODO : 1) IDENTIFY MIS (Controlling value) GATES IN TOPO ORDER 2) SET UNTOGGLE THESE AND RE-EVALUATE DTS
        float worst_slack = T->runTiming(10.0);
        //it->second.second = worst_slack; // STORING SLACK HERE
        if (worst_slack < dynamic_slack)
        {
          dynamic_slack = worst_slack;
          T->reportTiming(-1);
          cout << "Updating Dynamic Slack" << endl;
        }
        cout << "Worst slack for this toggled term set ( of size " << unit.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
    }
}

void PowerOpt::find_dynamic_slack_pins_fast(designTiming *T)
{
  cout << "Finiding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216");
  float dynamic_slack = 10000.0;
  map<string, pair<int, pair<int, float > > > :: iterator it;
  int skip = 0;
  for (it = toggled_terms_counts.begin(); it != toggled_terms_counts.end(); it++)
  {
    string toggled_terms_str = it->first;
    vector<string> terms_names;
    tokenize(toggled_terms_str, ',', terms_names);
    //if (skip < 837) { skip++;  continue; }
    T->setFalsePathsThroughAllPins();
    for (int i = 0; i < terms_names.size(); i ++)
    {
      vector<string> tokens;
      tokenize(terms_names[i], '-', tokens);
      string term_name = tokens[0];
      string togg_type = tokens[1];
      int term_id = terminalNameIdMap[term_name];
      replace_substr(term_name, "/", "\\/" );
      if      ( togg_type == "R") { terms[term_id]->setToggled ( true, "1") ; T->resetRisePathForTerm  (term_name) ; }
      else if ( togg_type == "F") { terms[term_id]->setToggled ( true, "0") ; T->resetFallPathForTerm  (term_name) ; }
      else if ( togg_type == "X") { terms[term_id]->setToggled ( true, "x") ; T->resetPathForTerm      (term_name) ; }
      else  assert(0);
    }


    // IMPORTANT TODO : 1) IDENTIFY MIS (Controlling value) GATES IN TOPO ORDER 2) SET UNTOGGLE THESE AND RE-EVALUATE DTS
    T->updateTiming();
    for (int i = 0; i < m_gates_topo.size(); i++)
    {
      Gate* gate = m_gates_topo[i];
      vector<int> false_term_ids;
      gate->checkControllingMIS(false_term_ids, T);
      if (!false_term_ids.empty()) cout << " Gate " << gate->getName() << " has a slow MIS input" << endl;

      for (int j = 0; j < false_term_ids.size(); j++)
      {
        string term_name = terms[false_term_ids[j]]->getFullName();
        T->setFalsePathForTerm(term_name);
      }
    }

    float worst_slack = T->runTiming(10.0);
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      //T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << terms_names.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
    clearToggledTerms();
  }
  cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;

}

void PowerOpt::find_dynamic_slack_pins_basic(designTiming *T)
{
  cout << "Finding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  float dynamic_slack = 10000.0;
  //map<string, pair<int, float> > :: iterator it = toggled_sets_counts.begin();
  vector<vector< pair<Terminal*, ToggleType > > > :: iterator it = toggled_term_vector.begin();
  for ( ; it != toggled_term_vector.end(); it++)
  {
    vector< pair<Terminal*, ToggleType> >& toggled_terms =  *it;
    T->setFalsePathsThroughAllPins();
    for (int j = 0; j < toggled_terms.size(); j++)
    {
      Terminal* term = toggled_terms[j].first;
      ToggleType togg_type = toggled_terms[j].second;
      string term_name = term->getFullName();
      //cout << gate ;
      replace_substr(term_name,"/", "\\/");
      switch (togg_type)
      {
        case RISE:
                 T->resetRisePathForTerm(term_name);
                 break;
        case FALL:
                 T->resetFallPathForTerm(term_name);
                 break;
        case UNKN: T->resetPathForTerm(term_name); break;
//        case UNKN:
//                 break;
        default :
                 assert(0);
      }
      //T->resetPathForCell(gate);
    }
    float worst_slack = T->runTiming(10.0);
    //it->second.second = worst_slack; // STORING SLACK HERE
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << toggled_terms.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
    cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;
    //dump_slack_profile();

}

void PowerOpt::find_dynamic_slack_3(designTiming *T)
{
  //find_dynamic_slack_pins_basic(T);
  if (exeOp == 23)
  {
    find_dynamic_slack_pins_fast(T);
    return;
  }
//  if (exeOp == 24)
//  {
//    find_dynamic_slack_pins_subset(T);
//    return;
//  }
  cout << "Finding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  float dynamic_slack = 10000.0;
  map<string, pair< int, pair<int, float> > > :: iterator it = toggled_sets_counts.begin();
  for ( ; it != toggled_sets_counts.end(); it++)
  {
    string toggled_gates_str =  it->first;
    vector<string> toggled_gates;
    tokenize(toggled_gates_str, ',', toggled_gates);
    T->setFalsePathsThroughAllCells();
    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      //cout << gate ;
      T->resetPathForCell(gate);
    }
    float worst_slack = T->runTiming(10.0);
    it->second.second.second = worst_slack; // STORING SLACK HERE
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << toggled_gates.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
   cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;
   dump_slack_profile();
  /*for (int i = 0; i < toggled_gate_vector.size() ; i++)
  {
    vector<string> toggled_gates = toggled_gate_vector[i];
    double t0 = 0.0;
    //double reset_time = 0.0, set_fp_time = 0.0 , timing_time = 0.0;
    cout << "Setting False Paths" << endl;
    //TICK();
    T->setFalsePathsThroughAllCells();
    //TOCK(set_fp_time);
    //cout << "Time taken to set all false paths: " << set_fp_time << endl;
    //reset_all(T);
    //cout << "Reseting Necessary Cells" << endl;
    //TICK ();
    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      //cout << gate ;
      T->resetPathForCell(gate);
    }
    //TOCK(reset_time);
    //cout << "Time take to reset necessary cells" << reset_time << endl;
    //TICK();
    //float worst_slack = T->runTimingPBA(10.0);
    float worst_slack = T->runTiming(10.0);
    //TOCK(timing_time);
    //cout << " Time taken to report_timing " << timing_time << endl;
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << toggled_gates.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
    cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;*/
}

void PowerOpt::leakage_compute_coarse()
{
// Parse Leakage Data.
  float leakage_power = 0.0;
// Handle toggled gate vector
  for (int i = 0 ; i < toggled_gate_vector.size(); i ++)
  {
    vector<string> toggled_gates = toggled_gate_vector[i];
    leakage_power +=  6.21714e-6 + 9.113778e-6; // misc power
    bool mem_backbone_0   = false;
    bool frontend_0       = false;
    bool sfr_0            = false;
    bool dbg_0            = false;
    bool watchdog_0       = false;
    bool multiplier_0     = false;
    bool execution_unit_0 = false;
    bool clock_module_0   = false;

    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      vector<string> hierarchy ;
      tokenize (gate, '/', hierarchy);
      int size = hierarchy.size();
      if (hierarchy[0] == "mem_backbone_0" && hierarchy[size-1].find("_reg") != string::npos )         {
         mem_backbone_0 = true;
      }
      else if (hierarchy[0] == "frontend_0" && hierarchy[size-1].find("_reg") != string::npos )        {
          frontend_0 = true;
      }
      else if (hierarchy[0] == "sfr_0" && hierarchy[size-1].find("_reg") != string::npos )             {
          sfr_0 = true;
      }
      else if (hierarchy[0] == "dbg_0" && hierarchy[size-1].find("_reg") != string::npos  )            {
         dbg_0 = true;
      }
      else if (hierarchy[0] == "watchdog_0" && hierarchy[size-1].find("_reg") != string::npos  )       {
          watchdog_0 = true;
      }
      else if (hierarchy[0] == "multiplier_0" && hierarchy[size-1].find("_reg") != string::npos  )     {
           multiplier_0 = true;
      }
      else if (hierarchy[0] == "execution_unit_0" && hierarchy[size-1].find("_reg") != string::npos  ) {
           execution_unit_0 = true;
      }
      else if (hierarchy[0] == "clock_module_0"  && hierarchy[size-1].find("_reg") != string::npos )   {
           clock_module_0 = true;
      }
    }
    if (mem_backbone_0  ) { leakage_power +=  4.627409e-6 ; cout << " mem_backbone_0  ," ;}
    if (frontend_0      ) { leakage_power +=  1.505669e-5 ; cout << " frontend_0      ," ;}
    if (sfr_0           ) { leakage_power +=  6.558473e-7 ; cout << " sfr_0           ," ;}
    if (dbg_0           ) { leakage_power +=  1.022792e-5 ; cout << " dbg_0           ," ;}
    if (watchdog_0      ) { leakage_power +=  4.707044e-6 ; cout << " watchdog_0      ," ;}
    if (multiplier_0    ) { leakage_power +=  9.694503e-6 ; cout << " multiplier_0    ," ;}
    if (execution_unit_0) { leakage_power +=  2.555991e-5 ; cout << " execution_unit_0," ;}
    //if (clock_module_0  ) { leakage_power +=  9.113778e-6 ; cout << " clock_module_0  ," ;}
    cout << endl;
  }
  leakage_power /= toggled_gate_vector.size();
  cout << " Computed Leakage_power is " << leakage_power << endl;
}


void PowerOpt::leakage_compute()
{
  string line;
  ifstream file;
  file.open("leakage_data");
  if (!file.is_open())
  {
    cout << " File named leakage_data is not available. Please make sure it is available in the current folder "<< endl;
    exit(0);
  }
  float gate_pow ;
  map<string, float> :: iterator leak_it;
  cout << " Doing Leakage Computation" << endl;
  float total_leakage = 0;

  vector<string> line_contents;
  while (getline(file, line))
  {
    tokenize(line, ' ', line_contents);
    assert(line_contents.size() == 2) ;
    float power = ::atof(line_contents[1].c_str());
    string gate = line_contents[0];
    leakage_data.insert(std::make_pair(gate, power));
    line_contents.clear();
  }
  cout << " Toggled gate vector size is " << toggled_gate_vector.size() << endl;

  for (int i = 0 ; i < toggled_gate_vector.size(); i ++)// Fine Grained in space and Time
  {
    vector<string> toggled_gates = toggled_gate_vector[i];
    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      //cout << "Gate is " << gate << endl;
      if (leakage_data.find(gate) != leakage_data.end())
        gate_pow = leakage_data[gate];
      else assert(0);
      total_leakage += gate_pow;
    }
  }
  /*for (int i = 0; i < getGateNum(); i ++) {
      if (m_gates[i]->isToggled())
          total_leakage += leakage_data[m_gates[i]->getName()];
  }*/
  float avg_total_leakage = total_leakage/toggled_gate_vector.size();
  cout << "Leakage Data: "  << endl;
  cout << " Total leakage : " << total_leakage << endl;
  cout << " ideal Leakage : " << avg_total_leakage << endl;
}


void PowerOpt::leakage_compute_per_module()
{
  string line;
  ifstream file;
  file.open("leakage_data");
  if (!file.is_open())
  {
    cout << " File named leakage_data is not available. Please make sure it is available in the current folder "<< endl;
    exit(0);
  }
  //float gate_pow ;
  map<string, float> :: iterator leak_it;
  cout << " Doing Leakage Computation" << endl;
  //float total_leakage = 0;

  vector<string> line_contents;
  while (getline(file, line)) // reading leakage_data
  {
    tokenize(line, ' ', line_contents);
    assert(line_contents.size() == 2) ;
    float power = ::atof(line_contents[1].c_str());
    string gate = line_contents[0];
    leakage_data.insert(std::make_pair(gate, power));
    line_contents.clear();
  }
  cout << " Toggled gate vector size is " << toggled_gate_vector.size() << endl;


// Actual computation
  cout << "mem_backbone_0, frontend_0, sfr_0, dbg_0, watchdog_0, multiplier_0, execution_unit_0, clock_module_0, glue" << endl;
  for (int i = 0 ; i < toggled_gate_vector.size(); i ++)
  {
    vector<string> toggled_gates = toggled_gate_vector[i];

    float mem_backbone_0   = 0.0; float mem_backbone_0_norm_full  = 0.0; float mem_backbone_0_norm_module  = 0.0; int mem_backbone_0_count   = 0;
    float frontend_0       = 0.0; float frontend_0_norm_full      = 0.0; float frontend_0_norm_module      = 0.0; int frontend_0_count       = 0;
    float sfr_0            = 0.0; float sfr_0_norm_full           = 0.0; float sfr_0_norm_module           = 0.0; int sfr_0_count            = 0;
    float dbg_0            = 0.0; float dbg_0_norm_full           = 0.0; float dbg_0_norm_module           = 0.0; int dbg_0_count            = 0;
    float watchdog_0       = 0.0; float watchdog_0_norm_full      = 0.0; float watchdog_0_norm_module      = 0.0; int watchdog_0_count       = 0;
    float multiplier_0     = 0.0; float multiplier_0_norm_full    = 0.0; float multiplier_0_norm_module    = 0.0; int multiplier_0_count     = 0;
    float execution_unit_0 = 0.0; float execution_unit_0_norm_full= 0.0; float execution_unit_0_norm_module= 0.0; int execution_unit_0_count = 0;
    float clock_module_0   = 0.0; float clock_module_0_norm_full  = 0.0; float clock_module_0_norm_module  = 0.0; int clock_module_0_count   = 0;
    float glue             = 0.0; float glue_norm_full            = 0.0; float glue_norm_module            = 0.0; int glue_count             = 0;



    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      vector<string> hierarchy ;
      tokenize (gate, '/', hierarchy);
      //int size = hierarchy.size();
      if (hierarchy[0] == "mem_backbone_0"  )       { mem_backbone_0   += leakage_data[gate]; mem_backbone_0_count++;   }
      else if (hierarchy[0] == "frontend_0"  )      { frontend_0       += leakage_data[gate]; frontend_0_count++;       }
      else if (hierarchy[0] == "sfr_0"  )           { sfr_0            += leakage_data[gate]; sfr_0_count++;            }
      else if (hierarchy[0] == "dbg_0" )            { dbg_0            += leakage_data[gate]; dbg_0_count++;            }
      else if (hierarchy[0] == "watchdog_0" )       { watchdog_0       += leakage_data[gate]; watchdog_0_count++;       }
      else if (hierarchy[0] == "multiplier_0" )     { multiplier_0     += leakage_data[gate]; multiplier_0_count++;     }
      else if (hierarchy[0] == "execution_unit_0" ) { execution_unit_0 += leakage_data[gate]; execution_unit_0_count++; }
      else if (hierarchy[0] == "clock_module_0"  )  { clock_module_0   += leakage_data[gate]; clock_module_0_count++;   }
      else                                          { glue             += leakage_data[gate]; glue_count++;             }
      //cout << "Gate is " << gate << endl;
    }

    mem_backbone_0_norm_full   = mem_backbone_0   /toggled_gate_vector.size();
    frontend_0_norm_full       = frontend_0       /toggled_gate_vector.size();
    sfr_0_norm_full            = sfr_0            /toggled_gate_vector.size();
    dbg_0_norm_full            = dbg_0            /toggled_gate_vector.size();
    watchdog_0_norm_full       = watchdog_0       /toggled_gate_vector.size();
    multiplier_0_norm_full     = multiplier_0     /toggled_gate_vector.size();
    execution_unit_0_norm_full = execution_unit_0 /toggled_gate_vector.size();
    clock_module_0_norm_full   = clock_module_0   /toggled_gate_vector.size();
    glue_norm_full             = glue             /toggled_gate_vector.size();

    cout << "[FULL]"  <<  mem_backbone_0_norm_full   << ", " << frontend_0_norm_full       << ", " << sfr_0_norm_full            << ", " << dbg_0_norm_full            << ", " << watchdog_0_norm_full       << ", " << multiplier_0_norm_full     << ", " << execution_unit_0_norm_full << ", " << clock_module_0_norm_full   << ", " << glue_norm_full             << ", " << endl;

    mem_backbone_0_norm_module   = 100*(mem_backbone_0   /4.627409e-6);
    frontend_0_norm_module       = 100*(frontend_0       /1.505669e-5);
    sfr_0_norm_module            = 100*(sfr_0            /6.558473e-7);
    dbg_0_norm_module            = 100*(dbg_0            /1.022792e-5);
    watchdog_0_norm_module       = 100*(watchdog_0       /4.707044e-6);
    multiplier_0_norm_module     = 100*(multiplier_0     /9.694503e-6);
    execution_unit_0_norm_module = 100*(execution_unit_0 /2.555991e-5);
    clock_module_0_norm_module   = 100*(clock_module_0   /9.113778e-6);
    glue_norm_module             = 100*(glue             /6.21714e-6 );

    cout << "[MODULE][" << cycles_of_toggled_gate_vector[i] << "] " << mem_backbone_0_norm_module << ", " << frontend_0_norm_module << ", " << sfr_0_norm_module << ", " << dbg_0_norm_module << ", " << watchdog_0_norm_module << ", " << multiplier_0_norm_module << ", " << execution_unit_0_norm_module << ", " << clock_module_0_norm_module << ", " << glue_norm_module << ", " << endl ;

  }
}

void PowerOpt::find_dynamic_slack_1(designTiming *T)
{
  cout << "Finding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216");
  float dynamic_slack = 10000.0;
  for (int i = 0; i < toggled_gate_vector.size() ; i++)
  {
    vector<string> toggled_gates = toggled_gate_vector[i];
    double t0 = 0.0;
    //double reset_time = 0.0, set_fp_time = 0.0 , timing_time = 0.0;
    cout << "Setting False Paths" << endl;
    T->setFalsePathsThroughAllCells();
    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string gate = toggled_gates[j];
      T->resetPathForCell(gate);
    }
    float worst_slack = T->runTiming(10.0);
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << toggled_gates.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
  cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;
}

void PowerOpt::find_dynamic_slack_2(designTiming *T)
{
  cout << "Finding Dynamic Slack" << endl;
  T->suppressMessage("UITE-216"); // won't warn that a gate is not a start/endpoint while reseting paths
  float dynamic_slack = 10000.0;
  for (int i = 0 ; i < toggled_gate_vector_rise_fall.size(); i ++)
  {
    vector< pair<string, string> >& toggled_gates = toggled_gate_vector_rise_fall[i];
    T->setFalsePathsThroughAllCells();
    for (int j = 0; j < toggled_gates.size(); j++)
    {
      string& gate = toggled_gates[j].first;
      //T->resetPathForCell(gate);
      if(toggled_gates[j].second == "1") T->resetPathForCellRise(gate);
      else if(toggled_gates[j].second == "0") T->resetPathForCellFall(gate);
      else T->resetPathForCell(gate);
    }
    float worst_slack = T->runTiming(10.0);
    if (worst_slack < dynamic_slack)
    {
      dynamic_slack = worst_slack;
      T->reportTiming(-1);
      cout << "Updating Dynamic Slack" << endl;
    }
    cout << "Worst slack for this toggled gate set ( of size " << toggled_gates.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
  }
   cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;
}

void PowerOpt::find_dynamic_slack(designTiming* T)
{
    cout << "Finding Dynamic Slack" << endl;
    float dynamic_slack = 10000.0;
    map <vector<string>, float>::iterator it_dynamic_slack;
    for (map <vector<string>, float>:: iterator it = not_toggled_gate_map.begin(); it != not_toggled_gate_map.end(); it++)
    {
      vector<string> not_toggled_gates = it->first;
      float wrst_ep_slack = it->second;
      if (dynamic_slack > wrst_ep_slack)
      {
        T->resetPathsThroughAllCells();
        for (int i =0; i < not_toggled_gates.size(); i++)
        {
          string gate = not_toggled_gates[i];
          //cout << gate ;
          if (gate_name_dictionary.find(gate)->second->isendpoint())
          {
            //cout << " is endpoint "  << endl;
            T->setFalsePathTo(gate);
            T->setFalsePathFrom(gate);
          }
          else
          {
            //cout << " is not endpoint "  << endl;
            T->setFalsePathThrough(gate);
          }
        }
        float worst_slack = T->runTiming(10.0);// The argument doesn't matter
        if (worst_slack < dynamic_slack)
        {
          dynamic_slack = worst_slack;
          //it_dynamic_slack = it;
          T->reportTiming(-1);
          cout << "Updating Dynamic Slack" << endl;
        }
        cout << "Worst slack for this gate set ( of size " << not_toggled_gates.size() << " ) is " << worst_slack << " Dynamic Slack is " << dynamic_slack << endl;
      }
    }

    cout << "Dynamic Slack for BenchMark is " << dynamic_slack << endl;

}

void PowerOpt::check_for_flop_toggles_fast_subset( int cycle_num, int cycle_time, designTiming * T)
{
    cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
    //toggle_info_file << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
    int count_not_toggled = 0;
    int count_toggled = 0;
    string toggled_gates;
    vector<string> not_toggled_gates; // vector for reading easily
    vector<int> toggled_indices;
    string not_toggled_gates_str; // string for dumping
    for (int i = 0; i < getGateNum(); i ++) {
      Gate* g = m_gates[i];
      string gate_name = g->getName();
      if(g->isToggled())
      {
        count_toggled++;
        toggled_gates.append(gate_name);
        toggled_gates.append(",");
        toggled_indices.push_back(i);
        //toggle_info_file << "[TOG]" << gate_name << " Toggled !" << endl;
        if (g->isendpoint())
        {
          map<string, float> :: iterator it = endpoint_worst_slacks.find(gate_name);
          if (it == endpoint_worst_slacks.end())
          {
            cout << "Searched for a non-existing flop" << endl;
            exit(0);
          }
        }
      }
      else
      {
        count_not_toggled++;
        not_toggled_gates.push_back(gate_name);
        not_toggled_gates_str.append(gate_name);
        not_toggled_gates_str.append(",");
      }
    }
//    bool unique = toggled_gate_set.insert(toggled_gates).second; // insert returns true if it was a new element and false if an old element that was inserted
//    if (unique)
//    {
//      not_toggled_gate_map.insert(std::pair<vector<string>, float>(not_toggled_gates, 10000.0));
//      unique_not_toggle_gate_sets << not_toggled_gates_str << endl;
//    }
    if (!tree-> essr(toggled_indices, false))
      tree->insert(toggled_indices);

    /*cout << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
    cout << "[TOG] count_toggled is " <<  count_toggled << endl;
    cout << "[TOG] Worst endpoint slack for this cycle is " << ep_wrst_slk_for_cyc << endl;
    toggle_info_file << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
    toggle_info_file << "[TOG] count_toggled is " <<  count_toggled << endl;*/
}

void PowerOpt::check_for_flop_toggles_fast(int cycle_num, int cycle_time, designTiming * T)
{
    cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
    int count_not_toggled = 0;
    int count_toggled = 0;
    string toggled_gates_str;
    static int unique_toggled_set_id = 0;

    vector<string> not_toggled_gates; // vector for reading easily
    vector<string> toggled_gates_vec; // vector for reading easily
    set<string> toggled_gates_set;
    string not_toggled_gates_str; // string for dumping
    for (int i = 0; i < getGateNum(); i ++) {
      Gate* g = m_gates[i];
      string gate_name = g->getName();
      if(g->isToggled())
      {
        count_toggled++;
        toggled_gates_vec.push_back(gate_name);
        toggled_gates_set.insert(gate_name);
        toggled_gates_str.append(gate_name);
        toggled_gates_str.append(",");
        //toggle_info_file << gate_name << ", ";
        if (g->isendpoint()) // THIS IS JUST A CHECK
        {
          map<string, float> :: iterator it = endpoint_worst_slacks.find(gate_name);
          if (it == endpoint_worst_slacks.end())
          {
            cout << "Searched for a non-existing flop" << endl;
            exit(0);
          }
        }
      }
/*      else
      {
        count_not_toggled++;
        not_toggled_gates.push_back(gate_name);
        not_toggled_gates_str.append(gate_name);
        not_toggled_gates_str.append(",");
      }*/
    }
    cout << "inserting" << endl;
    //bool unique = toggled_gate_set.insert(toggled_gates_str).second; // insert returns true if it was a new element and false if an old element that was inserted
    map<string, pair<int, pair< int, float > > > :: iterator it = toggled_sets_counts.find(toggled_gates_str);
    if (it != toggled_sets_counts.end())
    {
      (it->second.second.first)++;
      //const string * ptr = &(it->first);
      per_cycle_toggled_sets.push_back(make_pair (cycle_time, it));
      //cout << "Not unique :" << &(it->first) << " at cycle : " << cycle_num << endl;
    }
    else  // NEW GATESET
    {
      std::pair<std::map<string, pair<int, pair<int, float> > >::iterator , bool> ret;
      ret = toggled_sets_counts.insert(std::pair<string, std::pair<int, std::pair<int, float> > > (toggled_gates_str, std::make_pair(unique_toggled_set_id, std::make_pair (1, 10000.0))));
      assert(ret.second); // true if unique inserted
      //const string * ptr = &(ret.first->first);
      per_cycle_toggled_sets.push_back(make_pair (cycle_time, ret.first));
      // not_toggled_gate_map.insert(std::pair<vector<string>, float>(not_toggled_gates, 10000.0));
      toggled_gate_vector.push_back(toggled_gates_vec);
      toggled_sets.push_back(toggled_gates_set);
      not_toggled_gate_vector.push_back(not_toggled_gates); // DEPRECATED
      unique_not_toggle_gate_sets << not_toggled_gates_str << endl; // DEPRECATED
      unique_toggle_gate_sets << unique_toggled_set_id << ":" << toggled_gates_str << endl;
      unique_toggled_set_id++;
    }

    //cout << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
    //cout << "[TOG] count_toggled is " <<  count_toggled << endl;
    //toggle_info_file << endl;
}

void PowerOpt::check_for_dead_ends(designTiming* T)
{
// READ THE unique toggled gates file

  cout << "IN THE DEAD END CHECK FUNCTION" << endl;
  if (!preprocess()) {
    read_ut_dump_file();
  }
  cout  << "[TOP] toggled_gate_vector's size is " << toggled_gate_vector.size() << endl;
  int count = 0;
  for (vector<set<string> >::iterator it1 = toggled_sets.begin() ; it1 != toggled_sets.end(); it1++)
  {
      if (count >0) break;
    set<string> toggled_gate_set = *it1;
    cout << "Size of toggled_gate_vec is " << toggled_gate_set.size() << endl;
    for (set<string>::iterator it2 = toggled_gate_set.begin(); it2 != toggled_gate_set.end(); it2++)
    {
      string gate_name = *it2;
      //Gate* gate = gate_name_dictionary[gate_name];
      // get the fanout gates
      string fanout_cells_str = T->getFanoutCellsFromCell(gate_name);
      vector<string> fanout_cells;
      tokenize(fanout_cells_str, ',' , fanout_cells);
      bool fanout_toggled = false;
      //cout << "CHECKING GATE " << gate_name << endl;
//      if (!gate->isToggled())
//        assert(0);
      for (vector<string>::iterator it3 = fanout_cells.begin(); it3 != fanout_cells.end(); it3++)
      {

        string fanout_gate_name = *it3;
//        Gate* fanout_gate = gate_name_dictionary[fanout_gate_name];
//        if (fanout_gate->isToggled())
//        {
//           fanout_toggled = true;
//           break;
//        }
        if (toggled_gate_set.find(fanout_gate_name) != toggled_gate_set.end()) // means the gate toggled
        {
          fanout_toggled = true;
          break;
        }
      }
      if (!fanout_toggled)
      {
        cout << gate_name << " --> DEAD END --> " << fanout_cells_str << endl;
      }
      else
      {
        cout << gate_name << " -->          --> " << fanout_cells_str << endl;
      }
      // check if all are non toggled --> them print that gate out
      //cout << gate_name << " --> " << fanout_cells_str << endl ;
    }
    cout << endl;
    count ++;
  }

}

//void PowerOpt::check_for_flop_paths(int cycle_num, int cycle_time)
//{
//  string gate_name = "clock_module_0_sync_cell_dco_wkup_data_sync_reg_1_";
//  cout << "Checking for toggled paths of just one gate" << gate_name << endl;
//  Gate * g;
//  for (int i = 0; i < getGateNum(); i ++) {
//    g = m_gates[i];
//    if( g->getName() == gate_name)
//      break;
//  }
//
//
//}

void PowerOpt::trace_toggled_path(int cycle_num, int cycle_time)
{
  string gate_name = "clock_module_0_sync_cell_dco_wkup_data_sync_reg_1_";
  cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  cout << "Checking for toggled paths of just one gate" << gate_name << endl;
  Gate * g;
  for (int i = 0; i < getGateNum(); i ++) {
    g = m_gates[i];
    if( g->getName() == gate_name)
      break;
  }
  cout << " Printing a toggled Path" << endl;
  if (g->isToggled())
  {
    cout << "The gate toggled in this cycle" << endl;
    while(true)
    {
      Gate * fanin;
      for (int i = 0; i < g->getFaninGateNum(); i++){
        fanin = g->getFaninGate(i);
        if (fanin->isToggled()) break; // JUST THE FIRST ONE YOUR FIND
      }
      g = fanin;
      cout << " " << g->getName() << " " << endl;
      if (g ->getFFFlag()) break;
    }
    cout << endl;
  }
}

void PowerOpt::check_for_toggles(int cycle_num, int cycle_time)
{
  int cycle_toggled_path_count = 0;
  //int end_point_toggle_misses = 0;
  cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
  for(int i = 0 ; i < endpoint_pair_list.size() ; i ++)
  {
    endpoint_pair_t endpoint_pair = endpoint_pair_list[i];
    //path_t test_path = endpoint_pair[0];
//    string launch_ff_name = test_path.front();
//    string capture_ff_name = test_path.back();
//    Gate* launch_ff = gate_name_dictionary.find(launch_ff_name)->second;
//    Gate* capture_ff = gate_name_dictionary.find(capture_ff_name)->second;

//    if (launch_ff->isToggled() && capture_ff->isToggled())
//    {
//      cout << " The launch " << launch_ff->getName() << " and capture " << capture_ff->getName() << " flipflops seem to have toggled " << endl;
      int bin_toggled_path_count = 0;
      for (int j = 0; j < endpoint_pair.size(); j++ )
      {
        path_t path = endpoint_pair[j];
        //i++;
        for ( path_t::iterator it3 = path.begin(); it3 != path.end(); ++it3)
        {
          string gate_name = *it3;
          Gate* gate = gate_name_dictionary.find(gate_name)->second;
          if (!gate->isToggled()) {
            if (gate_name == path.back())
            {
              cycle_toggled_path_count++;
              bin_toggled_path_count++;
              path_toggle_bin_list[i][j] = true;
              cout << "Toggled Path" << endl;
              for (int k = 0; k < path.size(); k++)
              {
                  cout << path[k] ;
              }
              cout << endl;
              //cout << i << endl;
              //cout << "PATH MISSED due to " << gate_name <<  "And it is the end point" <<  endl;
            }
//            else if ((gate_name == path.front()) && (path.size() > 2))
//            {
//              cout << "PATH MISSED due to " << gate_name << " and it is the launch ff" <<   endl;
//            }
//            else
//            {
//              cout << "PATH MISSED due to " << gate_name << " and it is a middle point" << endl;
//            }
            break;
          }
          else if (gate_name == path.back()){
            cycle_toggled_path_count++;
            bin_toggled_path_count++;
            path_toggle_bin_list[i][j] = true;
            cout << "Toggled Path" << endl;
            for (int k = 0; k < path.size(); k++)
            {
                cout << path[k] ;
            }
            cout << endl;
              //cout << i << endl;
          }
        }
      }
      if (bin_toggled_path_count)
        cout << "[COV] Number of toggled paths for bin number " << i << " is " << bin_toggled_path_count   << endl;
    //}
  }
  if (cycle_toggled_path_count)
    cout << "[COV] The number of PCPs that toggled is " << cycle_toggled_path_count << endl;
    //printf("In cycle %d, the number of PCPs that toggled is %d \n", cycle_num, cycle_toggled_path_count);
}

void PowerOpt::extractPaths_mode_15(int cycle_num, int &pathID, designTiming * T)
{
    Gate    *g;
    //Path    *p;

    cout << "Total Number of gates is " << getGateNum() << endl;
    cout << "In cycle " << cycle_num << endl;
    for (int i = 0; i < getGateNum(); i ++)
    {
        vector<GateVector> paths; // extracted paths for this gate
        //printf(" Created a paths vector of max_size: %d \n", paths.max_size());
        Gate    *faninD;
        g = m_gates[i];
        if (!g->getFFFlag()) continue; //@FIX: && !g->getPOFlag() // check if it is a ff

        bool toggled_flag = false;
        for (int l=0; l < g->getFaninGateNum(); ++l)
        {
          Gate *fanin = g->getFaninGate(l);
          Terminal *iTerm, *oTerm;
          int isNum = g->getInputSubnetNum();
          for (int h = 0; h < isNum; h++)
          {
            Subnet *s = g->getInputSubnet(h);
            if (s->inputIsPad() || s->outputIsPad())
                continue;
            iTerm = s->getInputTerminal();
            oTerm = s->getOutputTerminal();
            if ( iTerm->getGate()->getId() == fanin->getId() && oTerm->getGate()->getId() == g->getId())
            {
              if (oTerm->getName() == "D" && fanin->isToggled()) //@FIX: For PO, we need to check if the output is toggled, this check only works for DFF
              {
                toggled_flag = true;
                faninD = fanin;
              }
            }
          }
        }
        if (!toggled_flag) continue;
        // findToggledPaths_mode_15 (g, faninD, paths, T);

        map<string,Path*>::iterator path_lookup;
        // a pointer to a path's entry in the path_exists_dictionary

        // iterate over extracted paths and add to toggled path data structure
        for(int ii=0; ii<paths.size(); ii++){
          string PathString = getPathString (paths[ii]);
          // check if this path has been extracted previously
          path_lookup = path_exists_dictionary.find(PathString);
          // if the return value is the end of the dictionary, there is no entry yet for this path
          if( path_lookup == path_exists_dictionary.end() ){
            // create a path and add the entry to the dictionary
            Path *newPath = new Path(pathID++);
            newPath->setPathStr(PathString);
            // reversed the order of this loop since gates are traced and added to gatevectors in reverse order
            int GateNum = paths[ii].size();
            for (int k=GateNum-1; k >= 0; k--) {
              newPath->addGate(paths[ii][k]);
            }
            newPath->addCycle(cycle_num);
            addToggledPath(newPath);
            //cout << "Path : " << pathID << " : " << cycle_num;
            //cout << " : " << PathString << endl;
            // put a pointer to the path in the path exists dictionary
            path_exists_dictionary[PathString] = newPath;
          }
          else
          {
            // the path exists, add this cycle to its toggle list
            ((*path_lookup).second)->addCycle(cycle_num);
          }
        }
    }
}
void PowerOpt::extractPaths(int cycle_num, int &pathID)
{
    Gate    *g;
    //Path    *p;

    cout << "Total Number of gates is " << getGateNum() << endl;
    cout << "In cycle " << cycle_num << endl;
    for (int i = 0; i < getGateNum(); i ++)
    {
        vector<GateVector> paths; // extracted paths for this gate
        //printf(" Created a paths vector of max_size: %d \n", paths.max_size());
        Gate    *faninD;
        g = m_gates[i];
        if (!g->getFFFlag()) continue; //@FIX: && !g->getPOFlag() // check if it is a ff

        bool toggled_flag = false;
        for (int l=0; l < g->getFaninGateNum(); ++l)
        {
          Gate *fanin = g->getFaninGate(l);
          Terminal *iTerm, *oTerm;
          int isNum = g->getInputSubnetNum();
          for (int h = 0; h < isNum; h++)
          {
            Subnet *s = g->getInputSubnet(h);
            if (s->inputIsPad() || s->outputIsPad())
                continue;
            iTerm = s->getInputTerminal();
            oTerm = s->getOutputTerminal();
            if ( iTerm->getGate()->getId() == fanin->getId() && oTerm->getGate()->getId() == g->getId())
            {
              if (oTerm->getName() == "D" && fanin->isToggled()) //@FIX: For PO, we need to check if the output is toggled, this check only works for DFF
              {
                toggled_flag = true;
                faninD = fanin;
              }
            }
          }
        }
        if (!toggled_flag) continue;
        findToggledPaths (g, faninD, paths);

        map<string,Path*>::iterator path_lookup;
        // a pointer to a path's entry in the path_exists_dictionary

        // iterate over extracted paths and add to toggled path data structure
        for(int ii=0; ii<paths.size(); ii++){
          string PathString = getPathString (paths[ii]);
          // check if this path has been extracted previously
          path_lookup = path_exists_dictionary.find(PathString);
          // if the return value is the end of the dictionary, there is no entry yet for this path
          if( path_lookup == path_exists_dictionary.end() ){
            // create a path and add the entry to the dictionary
            Path *newPath = new Path(pathID++);
            newPath->setPathStr(PathString);
            // reversed the order of this loop since gates are traced and added to gatevectors in reverse order
            int GateNum = paths[ii].size();
            for (int k=GateNum-1; k >= 0; k--) {
              newPath->addGate(paths[ii][k]);
            }
            newPath->addCycle(cycle_num);
            addToggledPath(newPath);
            //cout << "Path : " << pathID << " : " << cycle_num;
            //cout << " : " << PathString << endl;
            // put a pointer to the path in the path exists dictionary
            path_exists_dictionary[PathString] = newPath;
          }
          else
          {
            // the path exists, add this cycle to its toggle list
            ((*path_lookup).second)->addCycle(cycle_num);
          }
        }
    }
}

void PowerOpt::clearToggled()
{
    //return; // RETURNS BACK!
    for (int i = 0; i < getGateNum(); i ++) {
        m_gates[i]->setToggled(false, "x");
    }
}

void PowerOpt::clearToggledTerms()
{
    for (int i =0; i < getTerminalNum(); i++)
    {
      terms[i]->setToggled(false);
    }
}

void PowerOpt::findToggledPaths_mode_15(Gate *g, Gate *faninD, vector<GateVector> &paths, designTiming * T)
{
  // start by pushing the faninD gate onto the stack
  // pop the top and push all toggled fanins as (current path + fanin)
  // when the source FF is reached, add the path to the paths vector
  // alernatively, if there is no toggled fanin and one of the input subnets is a PI, then add the path to the paths vector

  GateVector path;
  int i;
  Gate *fanin;
  Gate *gate;
  int num_toggled_fanins;  // number of toggled fanins (if 0, check for PI)

  stack<GateVector> path_stack;

  // start the first path
  // @NOTE: gates are added to the vector from end to beginning, so they are processed in reverse order later (outside this fn)
  path.push_back(g);
  path.push_back(faninD);
  path_stack.push(path);
  int path_count = 0;

  int j = 0;
  while( !path_stack.empty() ){
    // get the top element and check if the last gate is a FF
    path.assign( (path_stack.top()).begin(), (path_stack.top()).end() );
    path_stack.pop();
    int size = path_stack.size();
    // if the last gate is a FF, then add the path to the vector of extracted paths
    gate = path.back();
    if( gate->getFFFlag() ){
      string pathStr = getPathString(path);
      float path_slack = T->getPathSlack(pathStr);
      if (path_slack < 1.0) // This is 10% of a clock period of 10.0 . NEED TO GENERALIZE THIS through a cmd file parameter
      {
        paths.push_back(path);
        //path_count++;
      }
    }else{
      // find all the toggled fanins of gate and push their paths onto the stack
      num_toggled_fanins = 0;
      for (i = 0; i < gate->getFaninGateNum(); i++){
        fanin = gate->getFaninGate(i);
        if ( fanin->isToggled() ){
          num_toggled_fanins++;
          path_stack.push(path);
          (path_stack.top()).push_back(fanin);

          j++;
          if (j == 1000000)
            j = 0;
          if (j == 0)
          {
            size = path_stack.size();
            printf(" HERE path stack size : %d \n", size);
          }
            //printf(" size of path stack is %d\n", size);

        }
      }
      // now, check if there were no toggled fanins, indicating that a PI caused this gate to toggle
      if(num_toggled_fanins == 0){
        // @NOTE: We check here whether an input net is connected to a PI, but it may not be necessary to check
        //        We could just assume and add the path to the paths vector
        // What if more than 1 PI is connected? We would need to know which one toggled. We can find out in extractPaths.

        paths.push_back(path);
        //path_count++;

        /* not checking if connected to a PI, because it must be the case
        for(i = 0; i < gate->getInputSubnetNum(); i++){
          if( (gate->getInputSubnet(i))->inputIsPad() ){
            paths.push_back(path);
            break;
          }
        }
        */
      }
    }
  }
  printf("path count is : %d \n", path_count);
}

void PowerOpt::parsePtReport()
{
  string line;
  ifstream file_list;
  file_list.open(ptReport.c_str());
  string file;
  int total_path_count = 0;
  if (file_list.is_open())
  {
    while (getline(file_list, file))
    {
      ifstream epp_file;// endpoint_pair_file
      epp_file.open(file.c_str());
      if (epp_file.is_open())
      {
        endpoint_pair_t endpoint_pair;
        int path_count = 0;
        int path_end_count = 0;
        while (getline(epp_file, line))
        {
          if (ignoreClockGates)
          {
            if (line.size() > 10 && (line.compare(2,10,"Startpoint") == 0) && (line.find("clock_gate") != string::npos))
            {
              //cout << "ENDING" << endl;
              while (getline(epp_file,line) && line.size() > 8 && line.compare(2,5,"Point") !=0 ) ;
              getline(epp_file,line);
              getline(epp_file,line); // SKIP this path by skipping past "Point" in the file
              //cout << line << endl;
            }
            if (line.size() > 8 && (line.compare(2,8,"Endpoint") == 0) && (line.find("clock_gate") != string::npos))
            {
              while (getline(epp_file,line) && line.size() > 8 && line.compare(2,5,"Point") !=0 ) ;
              getline(epp_file,line);
              getline(epp_file,line); // SKIP this path by skipping past "Point" in the file
            }
          }
          if (ignoreMultiplier)
          {
            if (line.size() > 10 && (line.compare(2,10,"Startpoint") == 0) && (line.find("multiplier") != string::npos))
            {
              while (getline(epp_file,line) && line.size() > 8 && line.compare(2,5,"Point") !=0 ) ;
              getline(epp_file,line);
              getline(epp_file,line); // SKIP this path by skipping past "Point" in the file
            }
            if (line.size() > 8 && (line.compare(2,8,"Endpoint") == 0) && (line.find("multiplier") != string::npos))
            {
              while (getline(epp_file,line) && line.size() > 8 && line.compare(2,5,"Point") !=0 ) ;
              getline(epp_file,line);
              getline(epp_file,line); // SKIP this path by skipping past "Point" in the file
            }
          }

          if(line.size() > 8 &&  line.compare(2, 5, "Point") == 0)
          {
            path_count++;
            path_t  path;
            getline(epp_file,line); // -------
            getline(epp_file,line); // clock
            getline(epp_file,line); // clock network delay
            getline(epp_file,line); // FlipFlop/CP
            // The next line will be FlipFlop/D
            bool full_path_found = false;
            /*if (getline(epp_file,line))
            {
            }*/
            while (getline(epp_file,line))
            {
              if (line.size () >  20  && line.compare(2, 17, "data arrival time") == 0) // END OF PATH
              {
                path_end_count++;
                total_path_count++;
                full_path_found = true;
                break;
              }
              char buffer[80];
              //std::size_t length = line.copy(buffer,80,2);
              char * token = strtok(buffer, "/");
              string gate(token); // converting to string
              path.push_back(gate);
              //cout << gate << endl;
            }
            if (full_path_found)
            {
              endpoint_pair.push_back(path);
              full_path_found = false;
            }
          }
          //cout << " --------" << endl;
        }
        //if (endpoint_pair.size())

        vector<bool> path_toggle_info (endpoint_pair.size(), false);
        path_toggle_bin_list.push_back(path_toggle_info);
        endpoint_pair_list.push_back(endpoint_pair);
        cout << "[PowerOpt] Finished Parsing PT report: " << file << " and Number of Path starts : " << path_count << " and Number of Path ends :" << path_end_count <<  endl;
        epp_file.close();
      }
      else {
        cout << " File named " << file << " couldn't be opened. "  << endl;
        exit(0);
      }
    }
  }
  else {
    cout << " Coundn't find the file named "<<  ptReport << ". Please check if the file exists in the current folder" << endl;
    exit(0);
  }
  cout << "[PowerOpt] Total paths (among all bins) is : " << total_path_count  << endl;

  file_list.close();
  // TEST CODE
//  for(endpoint_pair_list_t::iterator it1 = endpoint_pair_list.begin(); it1 != endpoint_pair_list.end(); ++it1)
//  {
//    endpoint_pair_t endpoint_pair = *it1;
//    cout << "Endpoint Pair" << endl;
//    for (endpoint_pair_t::iterator it2 = endpoint_pair.begin() ; it2 != endpoint_pair.end(); ++it2)
//    {
//      path_t path = *it2;
//      cout << "Path" << endl;
//      string right_endpoint = path.back();
//      cout << "Hello" << endl;
//      //cout << right_endpoint << endl;
//      for ( path_t::iterator it3 = path.begin(); it3 != path.end(); ++it3)
//      {
//        cout <<  *it3 << endl;
//      }
//      //break;
//
//    }
//    //break;
//
//  }
//
//    // SMALL CHECK
//  Gate *g;
//  string out_pin;
//  for (int i = 0; i < getGateNum(); i ++)
//  {
//    g = m_gates[i];
//    Gate* gate = gate_name_dictionary.find(g->getName())->second;
//    if(g != gate)
//    {
//      cout << "Something wrong at " << i << endl;
//    }
//    if (i %100 == 0 )
//      cout << "Gate  is " << g->getName() <<  " i is " << i << endl;
//  }

}
void PowerOpt::readFlopWorstSlacks()
{
  ifstream file;
  string line;
  string gate;
  string slack;
  stringstream iss;
  file.open(worstSlacksFile.c_str());
  if (file.is_open())
  {
      while(getline(file, line))
      {
        iss << line ;
        getline(iss, gate, ',');
        getline(iss, slack, ',');
        map<string, float> :: iterator it = endpoint_worst_slacks.find(gate);
        if (it != endpoint_worst_slacks.end())
        {
          float slack_float = atof(slack.c_str());
          if (slack_float < it->second)
          {
            slack_float  = it->second;
            endpoint_worst_slacks[gate] = slack_float;
            map<string, Gate*>::iterator gate_lookup =  gate_name_dictionary.find(gate);
            if (gate_lookup != gate_name_dictionary.end())
            {
              gate_lookup->second->setisendpoint(true);
            }
            else
            {
              cout << "Failed gate lookup for gate name " << gate << endl;
              //exit(0);
            }
          }
        }
        else
        {
          //endpoint_worst_slacks.insert(std::pair<string, float> (gate, atof(slack.c_str())));
          endpoint_worst_slacks.insert(std::pair<string, float> (gate, 10000.0));
          map<string, Gate*>::iterator gate_lookup =  gate_name_dictionary.find(gate);
          if (gate_lookup != gate_name_dictionary.end())
          {
            gate_lookup->second->setisendpoint(true);
          }
          else
          {
            cout << "Failed gate lookup for gate name " << gate << endl;
            //exit(0);
          }
        }
        iss.clear();
      }
  }
  else
  {
    cout << "Endpoint Worst Slacks File Not Found. Make sure it is in the current directory" << endl;
    exit(0);
  }
  cout << "DONE READING PROPERLY " << endl;
  cout << "No of endpoints : " << endpoint_worst_slacks.size() << endl;
}

void PowerOpt::findToggledPaths(Gate *g, Gate *faninD, vector<GateVector> &paths)
{
  // start by pushing the faninD gate onto the stack
  // pop the top and push all toggled fanins as (current path + fanin)
  // when the source FF is reached, add the path to the paths vector
  // alernatively, if there is no toggled fanin and one of the input subnets is a PI, then add the path to the paths vector

  GateVector path;
  int i;
  Gate *fanin;
  Gate *gate;
  int num_toggled_fanins;  // number of toggled fanins (if 0, check for PI)

  stack<GateVector> path_stack;

  // start the first path
  // @NOTE: gates are added to the vector from end to beginning, so they are processed in reverse order later (outside this fn)
  path.push_back(g);
  path.push_back(faninD);
  path_stack.push(path);
  int path_count = 0;

  //static int j = 0;
  while( !path_stack.empty() ){
    // get the top element and check if the last gate is a FF
    path.assign( (path_stack.top()).begin(), (path_stack.top()).end() );
    path_stack.pop();
    //int size = path_stack.size();
    // if the last gate is a FF, then add the path to the vector of extracted paths
    gate = path.back();
    if( gate->getFFFlag() ){
        paths.push_back(path);
        //path_count++;
    }else{
      // find all the toggled fanins of gate and push their paths onto the stack
      num_toggled_fanins = 0;
      for (i = 0; i < gate->getFaninGateNum(); i++){
        fanin = gate->getFaninGate(i);
        if ( fanin->isToggled() ){
          num_toggled_fanins++;
          path_stack.push(path);
          (path_stack.top()).push_back(fanin);

//          j++;
////          if (j == 10000)
////            j = 0;
//          if (j%10000 == 0)
//          {
//            size = path_stack.size();
//            printf(" HERE path stack size : %d \n", size);
//          }
//            //printf(" size of path stack is %d\n", size);

        }
      }
      // now, check if there were no toggled fanins, indicating that a PI caused this gate to toggle
      if(num_toggled_fanins == 0){
        // @NOTE: We check here whether an input net is connected to a PI, but it may not be necessary to check
        //        We could just assume and add the path to the paths vector
        // What if more than 1 PI is connected? We would need to know which one toggled. We can find out in extractPaths.

        paths.push_back(path);
        path_count++;

        /* not checking if connected to a PI, because it must be the case
        for(i = 0; i < gate->getInputSubnetNum(); i++){
          if( (gate->getInputSubnet(i))->inputIsPad() ){
            paths.push_back(path);
            break;
          }
        }
        */
      }
    }
  }
  //printf("path count here is : %d \n", path_count);
}

string PowerOpt::getPathString (GateVector paths)
{ string out_pin;
    string PathString = "\\\"";
    int GateNum = paths.size();
    // This loop used to go from 0 to GateNum,
    // but that requires us to insert gates at the beginning of a vector, which is inefficient
    // I reversed the order of the loop so push_back can be used instead (constant time vs linear time)
    for (int k=GateNum-1; k >= 0; k--) {
      Gate *g_tmp = paths[k];
      outpin_lookup = gate_outpin_dictionary.find(g_tmp->getName());
      out_pin = (*outpin_lookup).second;
      if ( k == (GateNum-1) && g_tmp->getFFFlag()){
        PathString.append(" -from ");
        PathString.append(g_tmp->getName());
        PathString.append("/CP");
        if (GateNum == 1)
          PathString.append("\\\"");
      }
      else if ( k == (GateNum-1) ){
        // This is the case for a path that begins at a PI
        PathString.append(" -from ");
        int isNum = g_tmp->getInputSubnetNum();
        Subnet *s;
        for (int kk = 0; kk < isNum; kk++) {
          s = g_tmp->getInputSubnet(kk);
          if( s->inputIsPad() ){
            break;  // This grabs the first input encountered, technically, there could be more than one and this might not have been the one that toggled
          }
        }
        // extract the PI name from the subnet name and append to the PathString
        string subnet_name = s->getName();
        string pi_name;
        size_t location;

        // extract the part before the last hyphen
        location = subnet_name.rfind("-");
        pi_name = subnet_name.substr(0,location);
        // put escape characters before the brackets if they exist
        location = pi_name.rfind("[");
        if(location != string::npos){
          pi_name.replace(location, 1, "\\[");
          pi_name.replace(pi_name.length()-1, 1, "\\]");
        }

        PathString.append(pi_name);
        PathString.append(" -through ");
        PathString.append(out_pin);
        if (GateNum == 1)
          PathString.append("\\\"");
      }
      //else if (k == GateNum-1 && g_tmp->getFFFlag()) {
      else if (k == 0 && g_tmp->getFFFlag()) {
        PathString.append(" -to ");
        PathString.append(g_tmp->getName());
        PathString.append("/D");
        PathString.append("\\\"");
      }
      //else if (k == GateNum-1) {
      else if (k == 0) {
        PathString.append(" -through ");
        PathString.append(out_pin);
        PathString.append("\\\"");
      }
      else {
        PathString.append(" -through ");
        PathString.append(out_pin);
      }
    }
    return PathString;
}

void PowerOpt::initSTA(designTiming *T) {
    T_main = T;
    initWNS = T->getWorstSlack(clockName);
    initWNSMin = T->getWorstSlackMin(clockName);
    //cout << "initial_WNS: " << initWNS << endl;
    //cout << "initlal_WNS(min): " << initWNSMin << endl;
    if (mmmcOn) {
        for (int i = 0; i < mmmcFile.size(); i++) {
            initWNS_mmmc.push_back(T_mmmc[i]->getWorstSlack(clockName));
            initWNSMin_mmmc.push_back(T_mmmc[i]->getWorstSlackMin(clockName));
            cout << "init_WNS_for_mmmc" << i << ": "
                 << initWNS_mmmc[i] << endl;
            cout << "init_WNS_for_mmmc(min)" << i << ": "
                 << initWNSMin_mmmc[i] << endl;
        }
    }
    char Commands[250];
    if (!ptLogSave) {
        sprintf(Commands, "\\rm pt*.log");
        system(Commands);
    }
}

void PowerOpt::exitPT() {
    T_main->Exit();
    for (int i = 0; i < mmmcFile.size(); i++) {
        if (mmmcOn) T_mmmc[i]->Exit();
    }
    char Commands[250];
    if (!ptLogSave) {
        sprintf(Commands, "\\rm %s*", ptClientTcl.c_str());
        system(Commands);
        sprintf(Commands, "\\rm %s*", ptServerTcl.c_str());
        system(Commands);
        sprintf(Commands, "\\rm server.slog");
        system(Commands);
    }
}

bool PowerOpt::checkViolation(string cellInst, designTiming *T) {
    if (checkViolationPT(cellInst,T)) return true;
    if (!mmmcOn) return false;
    for (int i = 0; i < mmmcFile.size(); i++) {
        if (checkViolationMMMC(cellInst, i)) return true;
    }
    return false;
}

bool PowerOpt::checkViolationPT(string cellInst, designTiming *T) {

    double worstSlackMin = 1000;

    // setup check
    if (chkWNSFlag) {
        curWNS = T->getWorstSlack(clockName);
        if ( curWNS < getTargWNS() + (double)getGuardBand() ) return true;
    } else {
        double cellSlack = T->getCellSlack(cellInst);
        if ( cellSlack < (getTargWNS() + (double)getGuardBand())  ) {
            //cout << "SETUP" << endl;
            return true;
        }
    }
    if ( getHoldCheck()==1 ) {
        worstSlackMin = T->getWorstSlackMin(getClockName());
        if (worstSlackMin < getTargWNSMin()) {
            //cout << "HOLD" << endl;
            return true;
        }
    }
    if ( getMaxTrCheck()==1 ) {
        if (!T->checkMaxTran(cellInst)) {
            //cout << "MaxTr" << endl;
            return true;
        }
    }
    return false;
}

bool PowerOpt::checkViolationMMMC(string cellInst, int mmmc_num) {

    double cellSlack = T_mmmc[mmmc_num]->getCellSlack(cellInst);
    double worstSlackMin = 1000;

    if (chkWNSFlag) {
        curWNS = T_mmmc[mmmc_num]->getWorstSlack(clockName);
        if ( curWNS < getTargWNS_mmmc(mmmc_num) ) return true;
    }
    // setup check
    if ( cellSlack < (getTargWNS_mmmc(mmmc_num) + (double)getGuardBand())  ) {
        //cout << "SETUP" << endl;
        return true;
    }
    if ( getHoldCheck()==1 ) {
        worstSlackMin = T_mmmc[mmmc_num]->getWorstSlackMin(getClockName());
        if (worstSlackMin < getTargWNSMin_mmmc(mmmc_num)) {
            //cout << "HOLD" << endl;
            return true;
        }
    }
    if ( getMaxTrCheck()==1 ) {
        if (!T_mmmc[mmmc_num]->checkMaxTran(cellInst)) {
            //cout << "MaxTr" << endl;
            return true;
        }
    }
    return false;
}

void PowerOpt::clearHolded() {
    for (int i = 0; i < getGateNum(); i++)
        m_gates[i]->setHolded(false);
}

float PowerOpt::getTokenF(string line, string option)
{
    float token;
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    sscanf(arg.c_str(), "%f", &token);
    return token;

}

string PowerOpt::getTokenS(string line, string option)
{
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    return arg;
}

char* PowerOpt::getTokenC(string line, string option)
{
    char token[1000];
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    sscanf(arg.c_str(), "%s", &token);
    return token;
}

// wildcard string compare
int PowerOpt::cmpString(string wild, string pattern)
{
    const char *cp = NULL, *mp = NULL;
    const char *wd, *pt;

    wd = wild.c_str();
    //strcpy (wd, wild.c_str());
    //pt = new char [pattern.size()+1];
    pt = pattern.c_str();
    //strcpy (pt, pattern.c_str());

    while ((*pt) && (*wd != '*')) {
        if ((*wd != *pt) && (*wd != '?')) {
            //delete[] wd;
            //delete[] pt;
            return 0;
        }
        wd++;
        pt++;
    }

    while (*pt) {
        if (*wd == '*') {
            if (!*++wd) {
                //delete[] wd;
                //delete[] pt;
                return 1;
            }
            mp = wd;
            cp = pt+1;
        } else if ((*wd == *pt) || (*wd == '?')) {
            wd++;
            pt++;
        } else {
            wd = mp;
            pt = cp++;
        }
    }

    while (*wd == '*') {
        wd++;
    }
    //delete[] wd;
    //delete[] pt;
    return !*wd;
}

}   //namespace POWEROPT
