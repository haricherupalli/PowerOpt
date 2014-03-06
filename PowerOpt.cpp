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

// For Numerical Recipes
#include <limits.h>
//#include <errno.h>
// END

#include "PowerOpt.h"
#include "Path.h"
#include "Globals.h"
#include "Timer.h"

using namespace std;

// max # of clocks in the design
#define MAX_CLKS 256
// max length of an abbreviated net name
#define MAX_ABBREV 5
// max length of net name
#define MAX_NAME 50
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
    oaGenFlag = 1;
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
    fout << "    puts \"got command $msg on ptserver\"" << endl;
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
        fout << "pt_shell " << ptOption << " -f " << ptServerTcl << " |& tee pt.log" << endl;
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
    fout << "    puts \"got command $msg on ptserver\"" << endl;
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
    fout << "    puts \"got command $msg on ptserver\"" << endl;
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
    fout << "    puts \"got command $msg on ptserver\"" << endl;
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
    int token;
    size_t sizeStr, startPos, endPos;
    string arg;
    sizeStr = option.size();
    startPos = line.find_first_not_of(" ", line.find(option)+sizeStr);
    endPos = line.find_first_of(" ", startPos);
    arg = line.substr(startPos, endPos - startPos);
    sscanf(arg.c_str(), "%d", &token);
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
    total_leakage = total_leakage*0.000000001;
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
    char Commands[250];

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
  double tmpSensitivity;
  int new_lib_index;
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
  double path_slack;
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
  double tmpSensitivity;
  int new_lib_index;
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
// report entire RZ overhead
double PowerOpt::reportRZOH(designTiming *T){
    double RZ_dyn_oh = rz_dynamic;
    double RZ_sta_oh = rz_static;
    double sum_dyn_oh = 0;
    double sum_sta_oh = 0;
    double sum_hold_oh = 0;
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
    for (int i = 0; i < toggledPaths.size(); i++)
    {
        p = toggledPaths[i];
        path_slack = T->getPathSlack(p->getPathStr());
        p->setSlack(path_slack);
        slackfile << path_slack << "\t" << (double) p->getNumToggled() / (double) totalSimCycle << endl;
    }
    slackfile.close();
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

void PowerOpt::resizeCycleVectors(int total_cyc)
{
    error_cycles.resize(total_cyc);
    minus_cycles.resize(total_cyc);
    delta_cycles.resize(total_cyc);
    error_cycle.resize(2, vector<int> (total_cyc) );
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

    // seek to the beginning of the uut module
    do{
        fgets(str, 2048, VCDfile);
    }while(strstr(str,"uut") == NULL);

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
        net_dictionary[strng] = net_name;
        //dprintf("Added %s,%s to dictionary\n",abbrev,(net_dictionary[strng]).c_str());

        // if the net name contains CLOCKNAME, then we want to ignore the net when capturing toggle cycles
        // @NOTE: THIS ASSUMES THAT THE CLOCK NAME IS rclk (NOT ANYMORE)
        // record any clocks to ignore later
        //if( strstr(net_name,"rclk") != NULL){
        if( strstr(net_name,clockName.c_str()) != NULL){
            // to use strcmp, strings must be null terminated
            returnval = strchr(abbrev,'\n');
            if(returnval > 0){*returnval = '\0';}
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

        if( (cycle_time % 2000) == 1000){ // HARI NEED TO GENERALIZE THIS. 2000 is the clock period
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
                m_gates[CurInstId]->setToggled(true);
            }
            else{
            //    CurNetId = subnetNameIdMap[toggled_nets[j]];
            //    cout << "PI : " << subnets[CurNetId]->getName() << endl;
            // if there is no source gate, then the toggled net is a PI
            }
          }
          extractPaths(cycle_num+cycle_offset,pathID);
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
    return cycle_num;

    // free up all dynamically allocated memory
    /*
    for(j = 0; j < toggled_nets.size(); j++){
      delete[] toggled_nets[j];
    }
    */
}

void PowerOpt::extractPaths(int cycle_num, int &pathID)
{
    Gate    *g;
    //Path    *p;

    for (int i = 0; i < getGateNum(); i ++)
    {
        vector<GateVector> paths; // extracted paths for this gate
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
    for (int i = 0; i < getGateNum(); i ++) {
        m_gates[i]->setToggled(false);
    }
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

  while( !path_stack.empty() ){
    // get the top element and check if the last gate is a FF
    path.assign( (path_stack.top()).begin(), (path_stack.top()).end() );
    path_stack.pop();
    // if the last gate is a FF, then add the path to the vector of extracted paths
    gate = path.back();
    if( gate->getFFFlag() ){
      paths.push_back(path);
    }else{
      // find all the toggled fanins of gate and push their paths onto the stack
      num_toggled_fanins = 0;
      for (i = 0; i < gate->getFaninGateNum(); i++){
        fanin = gate->getFaninGate(i);
        if ( fanin->isToggled() ){
          num_toggled_fanins++;
          path_stack.push(path);
          (path_stack.top()).push_back(fanin);
        }
      }
      // now, check if there were no toggled fanins, indicating that a PI caused this gate to toggle
      if(num_toggled_fanins == 0){
        // @NOTE: We check here whether an input net is connected to a PI, but it may not be necessary to check
        //        We could just assume and add the path to the paths vector
        // What if more than 1 PI is connected? We would need to know which one toggled. We can find out in extractPaths.
        paths.push_back(path);
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
}

string PowerOpt::getPathString (GateVector paths)
{
    string out_pin;
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
