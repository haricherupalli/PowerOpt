#include "PowerOpt_pre.txt" // made a txt to avoid re-compilation  during make

namespace POWEROPT {

int PowerOpt::st_cycle_num = 0;

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
  iss.str("");
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

static string bin2hex (string binary_str)
{
  bitset<16> set(binary_str);
  stringstream ss;
  ss << hex << uppercase << set.to_ulong();
  return ss.str();
}

static string bin2hex32 (string binary_str)
{
  bitset<32> set(binary_str);
  stringstream ss;
  ss << hex << uppercase << set.to_ulong();
  return ss.str();
}

static bool hasX(string binary_str)
{
  return (binary_str.find("X") != string::npos);
}

//static bool replace_substr (std::string& str,const std::string& from, const std::string& to)
//{
//  size_t start_pos = str.find(from);
//  bool found = false;
//  size_t from_length = from.length();
//  size_t to_length = to.length();
//  while (start_pos != string::npos)
//  {
//    str.replace(start_pos, from.length(), to);
//    size_t adjustment =  (to_length>from_length)?(to_length - from_length):0;
//    start_pos = str.find(from, start_pos+ adjustment +1);
//    found = true;
//  }
//  return found;
///*  size_t start_pos = str.find(from);
//  if (start_pos == std::string::npos)
//    return false;
//  str.replace(start_pos, from.length(), to);
//  return true;*/
//
//}

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
        if (line.find("-clusterNamesFile") != string::npos)
            clusterNamesFile = getTokenS(line, "-clusterNamesFile");
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
    ptServerCmdTcl = homeDir + "/src/ptserver_cmds.tcl";
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
    vcdCheckStartCycle = -1;
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
    conservative_state = 0;
    subnegFlag = false;
    sim_units = 0;
    maintain_toggle_profiles = 0;
    inp_ind_branches = 0;
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
    global_curr_cycle = 0;

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
//        if (line.find("-clusterNamesFile") != string::npos)
//            clusterNamesFile = getTokenS(line, "-clusterNamesFile");
        if (line.find("-simInitFile ") != string::npos)
            simInitFile = getTokenS(line, "-simInitFile ");
        if (line.find("-dmemInitFile ") != string::npos)
            dmemInitFile = getTokenS(line, "-dmemInitFile ");
        if (line.find("-staticPGInfoFile ") != string::npos)
            staticPGInfoFile = getTokenS(line, "-staticPGInfoFile ");
        if (line.find("-dbgSelectGates ") != string::npos)
            dbgSelectGatesFile = getTokenS(line, "-dbgSelectGates ");
        if (line.find("-pmemFile ") != string::npos)
            pmem_file_name = getTokenS(line, "-pmemFile ");
        if (line.find("-ignoreNetsFile ") != string::npos)
            ignore_nets_file_name = getTokenS(line, "-ignoreNetsFile ");
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
        if (line.find("-vcdo ") != string::npos)
            vcdOutFile = getTokenS(line, "-vcdo ");
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
        if (line.find ("print_processor_state_profile_every_cycle") != string::npos)
            print_processor_state_profile_every_cycle = getTokenI(line, "-print_processor_state_profile_every_cycle");
        if (line.find("-IMOI ") != string::npos)
            internal_module_of_interest = getTokenS(line,"-IMOI");
        if (line.find("-preprocess") != string::npos)
            is_preprocess = true;
        if (line.find("-postprocess") != string::npos)
            is_postprocess = true;
        if (line.find("-dump_uts") != string::npos)
            is_dump_uts = true;
        if (line.find("-dump_units") != string::npos)
            is_dump_units = true;
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
        if (line.find("-design ") != string::npos)
            design = getTokenS(line,"-design ");
        if (line.find("-conservative_state ") != string::npos)
            conservative_state = getTokenI(line,"-conservative_state ");
        if (line.find("-inp_ind_branches ") != string::npos)
            inp_ind_branches = getTokenI(line,"-inp_ind_branches ");
        if (line.find("-inputValueFile ") != string::npos)
            inputValueFile = getTokenS(line,"-inputValueFile ");
        if (line.find("-outputValueFile ") != string::npos)
            outputValueFile = getTokenS(line,"-outputValueFile ");
        if (line.find("-outDir ") != string::npos)
            outDir = getTokenS(line,"-outDir ");
        if (line.find("-subneg") != string::npos)
            subnegFlag = true; subnegState= 0;
        if (line.find("-sim_units") != string::npos)
            sim_units = getTokenI(line,"-sim_units ");
        if (line.find("-maintain_toggle_profiles") != string::npos)
            maintain_toggle_profiles = getTokenI(line,"-maintain_toggle_profiles ");
        if (line.find("-one_pins_file") != string::npos)
            one_pins_file_name = getTokenS(line,"-one_pins_file ");
        if (line.find("-zero_pins_file") != string::npos)
            zero_pins_file_name = getTokenS(line,"-zero_pins_file ");
    }

    assert(num_sim_cycles > 0 );
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


//  FILE * VCDfile;
//  string VCDfilename = vcdPath + "/" + vcdFile[0];
//  VCDfile = fopen(VCDfilename.c_str(), "r");
//  if(VCDfile == NULL){
//    printf("ERROR, could not open VCD file: %s\n",VCDfilename.c_str());
//    exit(1);
//  }
//  fclose(VCDfile);
}

void PowerOpt::openFiles()
{
  if (outDir.empty()) outDir = ".";
  string dir_name = outDir+"/PowerOpt/";
  DIR* dir = opendir(dir_name.c_str());
  string system_cmd = "mkdir -p "+dir_name;
  if (dir) closedir(dir);
  else system(system_cmd.c_str());
  cout << " OPENING FILES and DIRECTORY IS " << dir_name << endl;
  if (exeOp == 15 || exeOp == 16 || exeOp == 17)
  {
    toggle_info_file.open             ( (outDir+string("/PowerOpt/toggle_info"                  )).c_str()                ) ;
    toggled_nets_file.open            ( (outDir+string("/PowerOpt/toggled_nets"                 )).c_str()                ) ;
    unique_not_toggle_gate_sets.open  ( (outDir+string("/PowerOpt/unique_not_toggle_gate_sets"  )).c_str()                ) ;
    unique_toggle_gate_sets.open      ( (outDir+string("/PowerOpt/unique_toggle_gate_sets"      )).c_str()                ) ;
    slack_profile_file.open           ( (outDir+string("/PowerOpt/slack_profile"                )).c_str()                ) ;
    net_gate_maps.open                ( (outDir+string("/PowerOpt/net_gate_maps"                )).c_str()                ) ;
    net_pin_maps.open                 ( (outDir+string("/PowerOpt/net_pin_maps"                 )).c_str()                ) ;
    toggle_counts_file.open           ( (outDir+string("/PowerOpt/toggle_counts"                )).c_str()                ) ;
    histogram_toggle_counts_file.open ( (outDir+string("/PowerOpt/histogram_toggle_counts_file" )).c_str()                ) ;
    dead_end_info_file.open           ( (outDir+string("/PowerOpt/dead_end_info_file"           )).c_str() , fstream::app ) ;
  }
  missed_nets.open                  ( (outDir+string("/PowerOpt/Missed_nets")).c_str(), fstream::app ) ;
  toggle_profile_file.open          ( (outDir+string("/PowerOpt/toggle_profile_file"    )).c_str()   ) ;
  //debug_file.open                   ( (outDir+string("/PowerOpt/debug_file"             )).c_str()   ) ;
  debug_file.open                   ("/dev/null") ;
  pmem_request_file.open            ( (outDir+string("/PowerOpt/pmem_request_file"      )).c_str()   ) ;
  dmem_request_file.open            ( (outDir+string("/PowerOpt/dmem_request_file"      )).c_str()   ) ;
  output_value_file.open            ( (outDir+string("/PowerOpt/output_value_file"      )).c_str()   ) ;
  fanins_file.open                  ( (outDir+string("/PowerOpt/fanins_file"            )).c_str()   ) ;
  processor_state_profile_file.open ( (outDir+string("/PowerOpt/processor_state_profile")).c_str()   ) ;
  dmem_contents_file.open           ( (outDir+string("/PowerOpt/dmem_contents_file"     )).c_str()   ) ;
  pmem_contents_file.open           ( (outDir+string("/PowerOpt/pmem_contents_file"     )).c_str()   ) ;
  debug_file_second.open            ( (outDir+string("/PowerOpt/debug_file_second"      )).c_str()   ) ;
  //debug_file_second.open            ("/dev/null") ;
  units_file.open                   ( (outDir+string("/PowerOpt/UNITS_INFO"             )).c_str()   ) ;
  all_toggled_gates_file.open       ( (outDir+string("/PowerOpt/all_toggled_gates_file" )).c_str()   ) ;
  net_toggle_info_file.open         ( (outDir+string("/PowerOpt/net_toggle_info_file"   )).c_str()   ) ;

  //vcdOutFile = outDir+"/PowerOpt/"+vcdOutFile;
  vcd_file.open((outDir+string("/PowerOpt/")+vcdOutFile).c_str());
  Gate * dummy_g; Net* dummy_n; system_state* dummy_ss;
  dummy_g->openFiles(outDir);
  dummy_n->openFiles(outDir);
  dummy_ss->openFiles(outDir);
  //debug_file_second.rdbuf(NULL);
}

void PowerOpt::closeFiles()
{
  if (exeOp == 15 || exeOp == 16 || exeOp == 17)
  {
    toggle_info_file.close();
    toggled_nets_file.close();
    unique_not_toggle_gate_sets.close();
    unique_toggle_gate_sets.close();
    slack_profile_file.close();
    net_gate_maps.close();
    net_pin_maps.close();
    toggle_counts_file.close();
    histogram_toggle_counts_file.close();
    dead_end_info_file.close();
  }
  missed_nets.close();
  units_file.close();
  all_toggled_gates_file.close();
  net_toggle_info_file.close();
  toggle_profile_file.close();
  debug_file.close();
  pmem_request_file.close();
  dmem_request_file.close();
  fanins_file.close();
  processor_state_profile_file.close();
  dmem_contents_file.close();
  pmem_contents_file.close();
  debug_file_second.close();
}


void PowerOpt::fill_reg_cells_list()
{
  ifstream reg_list_file;
  string reg_cells_file = getRegCellsFile();
  reg_list_file.open(reg_cells_file.c_str());
  if (reg_list_file.is_open())
  {
    string line;
    while (getline(reg_list_file, line))
    {
      reg_cells_set.insert(line);
    }
  }
  else
  {
    cout << "[PowerOpt] List of regs not read. Is the file there where it should be? " << endl;
    exit(0);
  }
}

void PowerOpt::DefParse()
{

  string line;
  bool in_components = false;
  bool in_pins = false;
  bool in_nets = false;
  bool reading_a_net = false;
  ifstream def_file;
  def_file.open(defFile.c_str());
  vector<string> tokens;
  int num_components;
  int num_pins;
  int num_nets;
  string net_name;
  vector<pair < string, string> > cell_name_and_pin;
  bool first_plus_in_net = true;

  if (def_file.is_open())
  {
    while (getline(def_file, line))
    {
      //debug_file << line << endl;
      trim(line);
      if (line.size() > 11 && line.compare(0, 10, "COMPONENTS") == 0) {
        tokenize(line, ' ', tokens);
        num_components = strtoull(tokens[1].c_str(), NULL, 10);
        tokens.clear();
        in_components = true;
      }
      if (line.size() > 5 && line.compare(0, 4, "PINS") == 0) {
        tokenize(line, ' ', tokens);
        num_pins = strtoull(tokens[1].c_str(), NULL, 10);
        tokens.clear();
        in_pins = true;
      }
      if (line.size() > 5 && line.compare(0, 4, "NETS") == 0) {
        tokenize(line, ' ', tokens);
        num_nets = strtoull(tokens[1].c_str(), NULL, 10);
        tokens.clear();
        in_nets = true;
      }
      if (line.size() > 4 && line.compare(0, 3, "END") == 0) {
        if (line.compare(4, 10, "COMPONENTS") == 0) {
          //cout << line << endl;
          in_components = false;
        }
        if (line.compare(4, 4, "PINS") == 0) {
          in_pins = false;
        }
        if (line.compare(4, 4, "NETS") == 0) {
          in_nets = false;
        }
      }
      if (in_components == true) {
        if (line[0] == '-') {
          tokenize(line, ' ', tokens);
          string cell_name = tokens[1];
          string cell_type = tokens[2];
          trim(cell_name); trim(cell_type);
          cell_name_and_type.insert(make_pair(cell_name, cell_type));
          tokens.clear();
        }
      }
      if (in_pins == true) {
        if (line[0] == '-') {
          tokenize(line, ' ', tokens);
          string pin_name = tokens[1];
          string net_name = tokens[4];
          string direction = tokens[7];
          pin_name_net_direction.insert(make_pair(pin_name, make_pair(net_name, direction)));
          tokens.clear();
        }
      }
      if (in_nets == true) {
        //cout << line << endl;
        if (line[0] == '-') {
          tokenize(line, ' ', tokens);
          net_name = tokens[1];
          reading_a_net = true;
          first_plus_in_net = true;
          tokens.clear();
        }
        else if (line[0] == '+' || line[0] == ';') {
          reading_a_net = false;
          if (first_plus_in_net == true) {
            first_plus_in_net = false;
            assert(!net_name.empty());
            //assert(!cell_name_and_pin.empty()); // CAN BE AN UNCONNECTED NET
            net_all_connected.insert(make_pair(net_name, cell_name_and_pin));
            //debug_file << " INSERTING " << net_name << endl;
            cell_name_and_pin.clear();
          }
        }
        else if (reading_a_net == true) {
          bool in_parantheses = false;
          string word_in_parantheses;
          for (int i = 0; i < line.size(); i++)
          {
            if (line[i] == '(') {
              in_parantheses = true;
            }
            else if (line[i] == ')') {
              in_parantheses = false;
              trim(word_in_parantheses);
              vector<string> my_tokens;
              tokenize(word_in_parantheses, ' ', my_tokens);
              assert(my_tokens.size() == 2);
              string cell_name = my_tokens[0];
              string pin_name = my_tokens[1];
              if (cell_name != "PIN")
                assert(cell_name_and_type.find(cell_name) != cell_name_and_type.end());
              else
                assert(pin_name_net_direction.find(pin_name) != pin_name_net_direction.end());
              cell_name_and_pin.push_back(make_pair (cell_name, pin_name));
              word_in_parantheses.clear();
            }
            else if (in_parantheses == true) {
              word_in_parantheses.append(1,line[i]);
            }
          }
        }
      }
    }
  }

  cout << " NUM CELLS " << cell_name_and_type.size() << endl;
  cout << " NUM PINS " << pin_name_net_direction.size() << endl;
  cout << " NUM NETS " << net_all_connected.size() << endl;


  // GATES
  map<string, string>::iterator it = cell_name_and_type.begin();
  int gateIndex = 0;
  for ( ; it!= cell_name_and_type.end(); it++)
  {
    string gate_name = it->first;
    string cell_name = it->second;
    gateNameIdMap[gate_name] = gateIndex;
    bool FFFlag = false;
    if (reg_cells_set.find(cell_name) != reg_cells_set.end()) { FFFlag = true; }
    //if (cell_name == "PLTIELO_X1" || cell_name == "PLTIEHI_X1") continue;
    Gate* g = new Gate(gateIndex++, FFFlag, gate_name, cell_name, 0.0, 0, 0, 0, 0);
    addGate(g);
  }
  // PINS -> PORT
  int padIndex = 0;
  map<string, pair<string, string> >::iterator it2 = pin_name_net_direction.begin();
  for (; it2 != pin_name_net_direction.end() ; it2++)
  {
    string pad_name = it2->first;
    string pad_net = it2->second.first;
    string pad_direction = it2->second.second;
    padNameIdMap[pad_name] = padIndex;
    PadType padType = InputOutput;
    if (pad_direction == "INPUT")
    {
      padType = PrimiaryInput;
    }
    else if (pad_direction == "OUTPUT")
    {
      padType = PrimiaryOutput;
    }
    else assert(0);
    Pad* pad = new Pad(padIndex++, pad_name , padType , 0, 0, 0 , 0 );
    addPad(pad);
  }
  // NETS and TERMINALS
  map<string, vector<pair<string, string> > >::iterator it3 = net_all_connected.begin(); // net, cell, cell_pin
  int netIndex = 0;
  int termIndex = 0;
  for (; it3 != net_all_connected.end(); it3++)
  {
    string net_name = it3->first;
    if (netNameException(net_name)) continue;
    Net * n = new Net(netIndex++, net_name);
    addNet(n);
    vector<pair<string, string> >& all_connected = it3->second;
    for (int i = 0; i != all_connected.size(); i++)
    {
      string gate_name = all_connected[i].first;
      if (gate_name != "PIN") // GATE
      {
        string term_name = all_connected[i].second;
        string terminal_full_name = gate_name+"/"+term_name;
        Gate * g = m_gates[gateNameIdMap[gate_name]];
        Terminal* term = NULL;
        if (terminals_info.find(terminal_full_name) == terminals_info.end())
        {
          terminalNameIdMap.insert(make_pair(terminal_full_name, termIndex));
          terminals_info[terminal_full_name] = make_pair(gate_name, term_name);
          term = new Terminal(termIndex++, term_name);
          addTerminal(term);
          term->setGate(g);
          term->addNet(n);
          term->computeFullName();
          //debug_file << "NET : " << net_name << " TERM : " << term->getFullName() << endl;
        } else {
          assert(0); // there is no way you can come across the same terminal on a different net
          //term = terms[terminalNameIdMap[terminal_full_name]];
        }
        assert(term != NULL);
        // CONNECTIVITY
        PinType pinType = getPinType(term_name);
        if (pinType == OUTPUT)
        {
          term->setPinType(OUTPUT);
          g->addFanoutTerminal(term);
          n->addInputTerminal(term);
          n->setDriverGate(g);
        }
        else if (pinType == INPUT)
        {
          term->setPinType(INPUT);
          g->addFaninTerminal(term);
          n->addOutputTerminal(term);
          n->addFanoutGate(g);
        }
        else assert(0);
      }
      else // PIN
      {
        string pad_name = all_connected[i].second;
        Pad*pad = m_pads[padNameIdMap[pad_name]];
        pad->addNet(n);
        n->addPad(pad);

        // TODO ADD DRIVER GATE
      }
    }
  }
}

bool PowerOpt::netNameException(string net_name)
{
  if (net_name == "VDD" || net_name == "GND" || net_name == "VSS" ) return true;
}

PinType PowerOpt::getPinType(string term_name)
{
//  if (term_name == "ZN" || term_name == "Z" || term_name == "Q" ) return OUTPUT;
//  if (term_name == "I" || term_name == "D" || term_name == "CP" || term_name == "CDN" || term_name == "SDN" ) return INPUT;
// TSMC ARM
  if (term_name == "QN" ||
      term_name == "Y" ||
      term_name == "ECK" ||
      term_name == "Q" ||
      term_name == "S" ||
      term_name == "CO" ||
      term_name == "RBL") return OUTPUT;
  if ( term_name == "A" ||
       term_name == "A0" ||
       term_name == "A0N" ||
       term_name == "A1" ||
       term_name == "A1N" ||
       term_name == "A2" ||
       term_name == "AN" ||
       term_name == "B" ||
       term_name == "B0" ||
       term_name == "B0N" ||
       term_name == "B1" ||
       term_name == "B1N" ||
       term_name == "B2" ||
       term_name == "BN" ||
       term_name == "C" ||
       term_name == "C0" ||
       term_name == "C1" ||
       term_name == "CI" ||
       term_name == "CK" ||
       term_name == "CKN" ||
       term_name == "D" ||
       term_name == "D0" ||
       term_name == "D1" ||
       term_name == "E" ||
       term_name == "G" ||
       term_name == "GN" ||
       term_name == "OE" ||
       term_name == "RN" ||
       term_name == "RWL" ||
       term_name == "WBL" ||
       term_name == "WWL" ||
       term_name == "S0" ||
       term_name == "S1" ||
       term_name == "SE" ||
       term_name == "SI" ||
       term_name == "SN" ) return INPUT;
}

void PowerOpt::print_processor_state_profile(int cycle_num, bool reg, bool pos_edge)
{

  if (pos_edge)
  {
    processor_state_profile_file <<  "PRINTING PROFILE FOR CYCLE " << cycle_num << "(p)" << endl;
  }
  else
  {
    processor_state_profile_file <<  "PRINTING PROFILE FOR CYCLE " << cycle_num << "(n)" << endl;
  }
//  for (int i = 0; i < terms.size(); i++)
//  {
//    Terminal * term = terms[i];
//    processor_state_profile_file << term->getFullName() << ":" << term->getSimValue() << endl ;
//  }
  for (int i = 0; i < m_gates.size(); i++)
  {
    Gate* gate = m_gates[i];
    if (reg == true)
      if (gate->getFFFlag() == false) continue;
    gate->print_terms(processor_state_profile_file);
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
  map<int, xbitset>::iterator it;
  for (it = DMemory.begin(); it != DMemory.end(); it ++)
  {
     int address = it->first;
     xbitset value = it->second;
     dmem_contents_file << address << ":" << value.to_string() << endl;
  }
}

void PowerOpt::print_fanin_cone(designTiming* T)
{
  for (int i = 0; i < getGateNum(); i ++)
  {
    Gate* g = m_gates[i];
    int parent_topo_id = g->getTopoId();
    fanins_file << i << "(" << g->getName() << ")  (" << g->getCellName() << ") " <<  " (" << parent_topo_id << ") " << endl;
    for (int l = 0; l < g->getFaninTerminalNum(); ++l)
    {
      Terminal * terminal = g->getFaninTerminal(l);
      Net* net = terminal->getNet(0);
      Gate* gate = net->getDriverGate();
      if (gate != 0) {
        int id = gate->getTopoId();
        //if (!g->getFFFlag()) assert(id < parent_topo_id);
        fanins_file << " . \t  Gate is -->" << gate->getName() << " (" << terminal->getName()  << ") " << " (" << id << ") " <<  endl;
/*        string gate_name_from_pt = T->getFaninGateAtTerm(terminal->getFullName());
        if (gate->getName() != gate_name_from_pt) {
          cout << gate->getName() << " : " << gate_name_from_pt << endl;
          //assert(gate->getName() == gate_name_from_pt);
        }*/
      }
      else if (net->getName() != terminal->getFullName()) {
        Pad* pad = net->getPad(0);
        int id = pad->getTopoId();
        //if (!g->getFFFlag()) assert(id < parent_topo_id);
        fanins_file <<  " . \t  Input Pad is --> " << pad->getName() << " (" << terminal->getName() << ") " <<  " (" << id << ") " << endl;
        //string pad_name_from_pt = T->getFaninPortAtTerm(terminal->getFullName());
        string pad_name = pad->getName();
        if (design == "flat_no_clk_gt")
        {
          replace_substr(pad_name, "\\[", "_" );
          replace_substr(pad_name, "\\]", "_" );
        }
        //assert(pad_name == pad_name_from_pt);
      }
      else { // constant nets
        fanins_file << " . \t  Constant --> " << terminal->getSimValue() << " (" << terminal->getName() << ") "  << endl;
        //string value_from_pt = T->getTerminalConstValue(terminal->getFullName());
        //assert(terminal->getSimValue() == value_from_pt);
      }
    }
  }

  for (int i = 0; i < m_pads.size(); i++)
  {
    Pad* pad = m_pads[i];
    int parent_topo_id = pad->getTopoId();
    if (pad->getType() == PrimiaryInput) continue;
    else
    {
      fanins_file << "[PAD] " << i << "(" << pad->getName() << ")  (" << parent_topo_id << ") " << endl;
      if (pad->getNetNum() != 1) continue;
      Net* net = pad->getNet(0);
      Gate* gate = net->getDriverGate();
      if (gate != NULL)
        fanins_file << " . \t  Gate is -->" << gate->getName() << endl;
    }

  }
  cout << " COMPARISON AGAINST PT WORKS " << endl;
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
         //debug_file << "Adding : " << node->getName() << endl;
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
         //debug_file << "Adding : " << node->getName() << endl;
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
        //debug_file << "Adding : " << node->getName() << endl;
        graph->addSink(node);
        graph->addNode(node);
      }
      else if (gate->getIsTIE())
      {
        node->setGate(gate);
        node->setIsSource(true);
        node->setIsSink(false);
        graph->addSource(node);
        graph->addWfNode(node);
        graph->addNode(node);
      }
      else // Other gates are nodes
      {
        node->setGate(gate);
        node->setIsSink(false);
        node->setIsSource(false);
        //node->etIsNode(true);
        graph->addNode(node);
      }
      //debug_file << " Gate " << gate->getName() << "'s Node is " << node  << endl;
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
      xbitset value_bs(value);
      DMemory.insert(make_pair(addr, value_bs));
    }
  }

}

void PowerOpt::readInputValueFile()
{
  ifstream input_value_file;
  input_value_file.open(inputValueFile.c_str());
  cout << "Reading input_value_file" << endl;
  if (input_value_file.is_open())
  {
    string line;
    getline(input_value_file, line);
    trim(line);
    num_inputs = atoi(line.c_str());
    while (getline(input_value_file, line))
    {
      trim(line);
      inputs.push_back(line);
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
      assert(tokens.size() == 2);
      string name = tokens[0];// term or pad name
      string value;
      value = tokens[1];
      trim(name);
      trim(value);
      if (value == "x") value = "X";
      // apply to the terminal
      map<string, int>::iterator it = terminalNameIdMap.find(name);
      if (it != terminalNameIdMap.end())
      {
        Terminal* term = terms[it->second];
        term->setSimValue(value, sim_wf);
        //debug_file << term->getFullName() << " : " << value << endl;
      }
      else //if (padNameIdMap.find(name) != padNameIdMap.end())
      {
        it = padNameIdMap.find(name);
        assert(it != padNameIdMap.end());
        Pad* pad = m_pads[it->second];
        pad->setSimValue(value, sim_wf);
        //debug_file << pad->getName() << " : "  << value << endl;
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

void PowerOpt::runSimulation(bool wavefront, int cycle_num, bool pos_edge)
{
  if (pos_edge)
  {
    cout << "Simulating cycle " << cycle_num << "(p)" << endl;
    debug_file << "Simulating cycle " << cycle_num << "(p)" << endl;
  }
  else
  {
    cout << "Simulating cycle " << cycle_num << "(n)" << endl;
    debug_file << "Simulating cycle " << cycle_num << "(n)" << endl;
  }
  if (design == "CORTEXM0PLUS")
  {
    Pad* HCLK_pad = m_pads[padNameIdMap["HCLK"]];
    string HCLK_value = HCLK_pad->getSimValue();
    if (HCLK_value == "1") HCLK_pad-> setSimValue("0", sim_wf);
    else if (HCLK_value == "0") HCLK_pad-> setSimValue("1", sim_wf);
    else { cout << " HCLK : " <<  HCLK_value << endl; assert(0); }
    Pad* SCLK_pad = m_pads[padNameIdMap["SCLK"]];
    string SCLK_value = SCLK_pad->getSimValue();
    if (SCLK_value == "1") SCLK_pad-> setSimValue("0", sim_wf);
    else if (SCLK_value == "0") SCLK_pad-> setSimValue("1", sim_wf);
    else { cout << " SCLK : " <<  SCLK_value << endl; assert(0); }
  }
  else if (design == "PLASTICARM")
  {
    Pad* I_CLKSYS_pad = m_pads[padNameIdMap["I_CLKSYS"]];
    string I_CLKSYS_value = I_CLKSYS_pad->getSimValue();
    if (I_CLKSYS_value == "1") I_CLKSYS_pad-> setSimValue("0", sim_wf);
    else if (I_CLKSYS_value == "0") I_CLKSYS_pad-> setSimValue("1", sim_wf);
    else { cout << " I_CLKSYS : " <<  I_CLKSYS_value << endl; assert(0); }
  }
  clearSimVisited();
  if(wavefront)
  {
    int gate_count = 0;
    int toggled_count = 0;
    //debug_file << "Cycle : " << cycle_num << endl;
    //missed_nets << "Cycle : " << cycle_num << endl;
    debug_file << "Sim_wf size is " << sim_wf.size() << endl;
    while (!sim_wf.empty())
    {
      GNode* node = sim_wf.top(); sim_wf.pop();
      gate_count ++;
      if ((node->getIsPad() == false) && (node->getIsSink() == false)) // Nothing needed for pads
      {
        if (node->getVisited() == true) continue; // This code is necessary to not re-evaluate gates that were already evaluated. A node can be pushed multiple times into sim_wf.
        node->setVisited(true);

        //debug_file << node->getName() << " : "  ;
        debug_file << "Sim_wf size is " << sim_wf.size() << endl;
        sim_visited.push(node);
        Gate* gate = node->getGate();
        bool toggled = gate->computeVal(sim_wf); // here we push gates into sim_wf
        if (toggled && !gate->getIsClkTree())
        {
          toggled_count++;
          if (maintain_toggle_profiles == 1) gate->updateToggleProfile(cycle_num);
          //cycle_toggled_indices.push_back(gate->getId());
          if (cycle_num > 8)
            all_toggled_gates.insert(gate->getId());
          cycle_toggled_indices.push_back(gate->getId());
          int cluster_id = gate->getClusterId();
          //debug_file << node->getName() << endl;
          //if (cluster_id != 0) { }
          if (clusterNamesFile != "")
          {
            map<int, Cluster* >::iterator it = clusters.find(cluster_id);
            if (it != clusters.end())
            {
              Cluster * cluster = it->second;
              cluster->setActive(true);
            }
          }
          //addVCDNetVal(gate->getFanoutTerminal(0)->getNet(0));
          //writeVCDNet(gate->getFanoutTerminal(0)->getNet(0));
        }
        else
        {
          //missed_nets << "Not Toggled : " << node->getName() << endl;
        }
        if (toggled && gate->getIsClkTree())
        {
          //debug_file << "clock_tree_cell : " <<  node->getName() << " : " <<  node->getGate()->getFanoutTerminal(0)->getNet(0)->getName() << " : " <<  node->getSimValue()<< endl;
          //addVCDNetVal(gate->getFanoutTerminal(0)->getNet(0));
          //writeVCDNet(gate->getFanoutTerminal(0)->getNet(0),cycle_num);
        }
      }
    }
    if (pos_edge)
      debug_file_second << "In cycle " << cycle_num << " (p) " << gate_count << " gates were simulated and " << toggled_count << " toggled."  << endl;
    else
      debug_file_second << "In cycle " << cycle_num << " (n) " << gate_count << " gates were simulated and " << toggled_count << " toggled."  << endl;
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
  if (!pos_edge)
  {
    map<int, Cluster* >::iterator it;
    //debug_file << "At Time " << cycle_num  << " Clusters that are ON : ";
    for (it = clusters.begin(); it != clusters.end(); it++)
    {
      if (it->second->getActive() && pmem_addr < 12288)
      {
        int id = it->second->getId();
        //debug_file << id << ", ";
        PMemory[pmem_addr]->domain_activity[id] = true;
        PMemory[pmem_addr]->executed = true;
      }
    }
    //debug_file << endl;
    resetClustersActive();
  }
}

void PowerOpt::resetClustersActive()
{
  map<int, Cluster* >::iterator it;
  for (it = clusters.begin(); it != clusters.end(); it++)
  {
    it->second->setActive(false);
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

void PowerOpt::readConstantTerminals() // These are gate pins that are constant in the design. Like logic 1 and logic 0.
{
  ifstream one_pins_file, zero_pins_file;
  one_pins_file.open(one_pins_file_name.c_str()); // full path or include in cmd file.
  zero_pins_file.open(zero_pins_file_name.c_str());
  string line;
  if (one_pins_file.is_open())
  {
    while (getline(one_pins_file, line))
    {
      trim(line);
      vector<string> tokens;
      tokenize(line, ':', tokens);
      string gate_name = tokens[0];
      string term_name = "";
      string pad_name = gate_name; // not sure if it is a pad or a gate yet;

      if (tokens.size() > 1)
      {
        assert(tokens.size() == 2);
        term_name = tokens[1];
      }

      string full_term_name = gate_name + "/" + term_name;

      if (gate_name.find("Logic1") != string::npos || gate_name.find("Logic0") != string::npos) continue;
      if (terminalNameIdMap.find(full_term_name) != terminalNameIdMap.end())
      {
        Terminal* term = terms[terminalNameIdMap[full_term_name]];
        term->setSimValue("1");
      }
      else if (gateNameIdMap.find(gate_name) != gateNameIdMap.end())
      {
        int id = gateNameIdMap.at(gate_name);
        Gate * gate = m_gates[id];
        int num_terms = terms.size();

        Terminal* term = new Terminal(num_terms, term_name);
        //Net *n = new Net(nets.size()+1, term_name); // set the net name to be same as term name

        term->setGate(gate);
        term->computeFullName();
        term->setPinType(INPUT);
        Net *n = new Net(nets.size(), term->getFullName(), true); // set the net name to be same as term name
        //debug_file << "Creating one Net with id " << n->getId() << " and name " << n->getName() << endl;
        n->addInputTerminal(term);
        n->setTopoSortMarked(true);
        term->addNet(n);
        term->setSimValue("1");

        assert(term->getId() == terms.size());
        assert(n->getId() == nets.size());
        addTerminal(term);
        addNet(n);

        string name = term->getFullName();
        //if (terminalNameIdMap.find(name) != terminalNameIdMap.end()) assert(0);
        gate->addFaninTerminal(term);
        terminalNameIdMap[name] = term->getId();
        netNameIdMap[n->getName()] = n->getId();
      }
      else if (padNameIdMap.find(pad_name) != padNameIdMap.end())
      {
        int id = padNameIdMap.at(pad_name);
        Pad* pad = m_pads.at(id);
        int num_nets_of_pad = pad->getNetNum();
        if (num_nets_of_pad == 0)
        {
          Net* n = new Net(nets.size(), pad->getName(), true);
          pad->addNet(n);
          n->addPad(pad);
        }
        pad->setSimValue("1");
      }
      else assert(0);
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
      string term_name = "";
      string pad_name = gate_name;

      if (tokens.size() > 1)
      {
        assert(tokens.size() == 2);
        term_name = tokens[1];
      }

      string full_term_name = gate_name + "/" + term_name;
      if (gate_name.find("Logic1") != string::npos || gate_name.find("Logic0") != string::npos) continue;
      if (terminalNameIdMap.find(full_term_name) != terminalNameIdMap.end())
      {
        Terminal* term = terms[terminalNameIdMap[full_term_name]];
        term->setSimValue("0");
      }
      else if (gateNameIdMap.find(gate_name) != gateNameIdMap.end())
      {
        int id = gateNameIdMap.at(gate_name);
        Gate * gate = m_gates[id];
        int num_terms = terms.size();
        Terminal* term = new Terminal(num_terms, term_name);

        term->setGate(gate);
        term->computeFullName();
        term->setPinType(INPUT);
        Net *n = new Net(nets.size(), term->getFullName(), true); // set the net name to be same as term name
        //debug_file << "Creating zero Net with id " << n->getId() << " and name " << n->getName() << endl;
        n->addInputTerminal(term);
        n->setTopoSortMarked(true);
        term->addNet(n);
        term->setSimValue("0");

        assert(term->getId() == terms.size());
        assert(n->getId() == nets.size());
        addTerminal(term);
        addNet(n);

        string name = term->getFullName();
        //if (terminalNameIdMap.find(name) != terminalNameIdMap.end()) assert(0);
        gate->addFaninTerminal(term);
        terminalNameIdMap[name] = term->getId();
        netNameIdMap[n->getName()] = n->getId();
      }
      else if (padNameIdMap.find(pad_name) != padNameIdMap.end())
      {
        int id = padNameIdMap.at(pad_name);
        Pad* pad = m_pads.at(id);
        int num_nets_of_pad = pad->getNetNum();
        if (num_nets_of_pad == 0)
        {
          Net* n = new Net(nets.size(), pad->getName(), true);
          pad->addNet(n);
          n->addPad(pad);
        }
        pad->setSimValue("0");
//        if (pad_name == "HADDR[31]")
//        {
//          cout << pad_name << endl;
//        }

      }
      else assert(0);
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

void PowerOpt::updateRegOutputs(int cycle_num)
{
//  Pad* dco_clk_pad = m_pads[padNameIdMap["dco_clk"]];
//  string dco_clk_value = dco_clk_pad->getSimValue();
//  if (dco_clk_value != "1") return;
  //cout << " Updating Reg Outputs " << endl;
  list<Gate*>::iterator it;
  int toggled_count = 0;
  for(it = m_regs.begin(); it != m_regs.end(); it++)
  {
    Gate* ff_gate = *it;
    bool toggled =  ff_gate->transferDtoQ(sim_wf);
    if (toggled)
    {
      toggled_count++;
      if (maintain_toggle_profiles == 1) ff_gate->updateToggleProfile(cycle_num);
      cycle_toggled_indices.push_back(ff_gate->getId());
      if (cycle_num > 1)
        all_toggled_gates.insert(ff_gate->getId());
    }
  }
  debug_file << " Num toggled FF's in cycle " << cycle_num << " is " << toggled_count << endl;
}

void PowerOpt::readPmemFile()
{
  FILE * pmem_file = fopen(pmem_file_name.c_str(), "r");
  if(pmem_file != NULL)
  {
     string line;
     unsigned int hex;
     cout << "Reading PMEM ..." ;
     int addr = 0;
     while ((fscanf(pmem_file, "%x", &hex) != EOF)) {
       //vector<bool> power_domain_activity_annotation (cluster_id, false);
       //const vector<bool>& reference = power_domain_activity_annotation;
       Instr* instr = new Instr(hex);
       //instr->domain_activity.resize(4, false);
       PMemory.insert(make_pair(addr, instr));
       addr++;
     }
     cout << " done" << endl;
  } else {
    cout << "ERROR: Cannot open pmem file!" << endl;
  }
}

void PowerOpt::readIgnoreNetsFile()
{
  ifstream ignore_nets_file;
  ignore_nets_file.open(ignore_nets_file_name.c_str());
  if (ignore_nets_file.is_open())
  {
    string line;
    while(getline(ignore_nets_file, line))
    {
      trim(line);
      ignore_nets.insert(line);
    }
  }
}

void PowerOpt::readStaticPGInfo()
{
  ifstream static_pg_info_file;
  static_pg_info_file.open(staticPGInfoFile.c_str());
  cout << "Reading Static PG Info" << endl;

  if (static_pg_info_file.is_open())
  {
    string line;
    while (getline(static_pg_info_file, line))
    {
      trim(line);
      vector<string> tokens;
      tokenize(line, ':', tokens);
      int addr = atoi(tokens[0].c_str());
      Instr* instr = PMemory[addr];
      for (int i = 1; i < tokens.size(); i++)
      {
        if (tokens[i].find("ON") != string::npos)
          instr->domain_activity[i-1] = true; // i-1 is offset from the file
        else if (tokens[i].find("OFF") != string::npos)
          instr->domain_activity[i-1] = false; // i-1 is offset from the file
        else if (tokens[i].find("Executed") != string::npos)
          instr->executed = true;
        else assert(0);
      }
    }
  }
}


string PowerOpt::getHaddr()
{

}

string PowerOpt::getPmemAddr()
{
  // HERE
  // return in if.
/*  string all_x="XXXXXXXXXXXXXX";
  if (subnegFlag == true)
  {
    if(subnegState ==0)
    {
      subnegState++;
    }
    else if (subnegState == 1)
    {
      subnegState++;
    }
    else if (subnegState == 2)
    {
      subnegState++;
    }
    else if (subnegState == 3)
    {
      subnegState=0;
    }
  return all_x;
  }*/





  string pmem_addr_13_val , pmem_addr_12_val , pmem_addr_11_val , pmem_addr_10_val , pmem_addr_9_val  , pmem_addr_8_val  , pmem_addr_7_val  , pmem_addr_6_val  , pmem_addr_5_val  , pmem_addr_4_val  , pmem_addr_3_val  , pmem_addr_2_val  , pmem_addr_1_val  , pmem_addr_0_val;
  if (design =="flat_no_clk_gt")
  {
    pmem_addr_13_val = m_pads[padNameIdMap.at("pmem_addr_13_")]->getSimValue();
    pmem_addr_12_val = m_pads[padNameIdMap.at("pmem_addr_12_")]->getSimValue();
    pmem_addr_11_val = m_pads[padNameIdMap.at("pmem_addr_11_")]->getSimValue();
    pmem_addr_10_val = m_pads[padNameIdMap.at("pmem_addr_10_")]->getSimValue();
    pmem_addr_9_val  = m_pads[padNameIdMap.at("pmem_addr_9_")]->getSimValue();
    pmem_addr_8_val  = m_pads[padNameIdMap.at("pmem_addr_8_")]->getSimValue();
    pmem_addr_7_val  = m_pads[padNameIdMap.at("pmem_addr_7_")]->getSimValue();
    pmem_addr_6_val  = m_pads[padNameIdMap.at("pmem_addr_6_")]->getSimValue();
    pmem_addr_5_val  = m_pads[padNameIdMap.at("pmem_addr_5_")]->getSimValue();
    pmem_addr_4_val  = m_pads[padNameIdMap.at("pmem_addr_4_")]->getSimValue();
    pmem_addr_3_val  = m_pads[padNameIdMap.at("pmem_addr_3_")]->getSimValue();
    pmem_addr_2_val  = m_pads[padNameIdMap.at("pmem_addr_2_")]->getSimValue();
    pmem_addr_1_val  = m_pads[padNameIdMap.at("pmem_addr_1_")]->getSimValue();
    pmem_addr_0_val  = m_pads[padNameIdMap.at("pmem_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    pmem_addr_13_val = m_pads[padNameIdMap.at("pmem_addr[13]")]->getSimValue();
    pmem_addr_12_val = m_pads[padNameIdMap.at("pmem_addr[12]")]->getSimValue();
    pmem_addr_11_val = m_pads[padNameIdMap.at("pmem_addr[11]")]->getSimValue();
    pmem_addr_10_val = m_pads[padNameIdMap.at("pmem_addr[10]")]->getSimValue();
    pmem_addr_9_val  = m_pads[padNameIdMap.at("pmem_addr[9]")]->getSimValue();
    pmem_addr_8_val  = m_pads[padNameIdMap.at("pmem_addr[8]")]->getSimValue();
    pmem_addr_7_val  = m_pads[padNameIdMap.at("pmem_addr[7]")]->getSimValue();
    pmem_addr_6_val  = m_pads[padNameIdMap.at("pmem_addr[6]")]->getSimValue();
    pmem_addr_5_val  = m_pads[padNameIdMap.at("pmem_addr[5]")]->getSimValue();
    pmem_addr_4_val  = m_pads[padNameIdMap.at("pmem_addr[4]")]->getSimValue();
    pmem_addr_3_val  = m_pads[padNameIdMap.at("pmem_addr[3]")]->getSimValue();
    pmem_addr_2_val  = m_pads[padNameIdMap.at("pmem_addr[2]")]->getSimValue();
    pmem_addr_1_val  = m_pads[padNameIdMap.at("pmem_addr[1]")]->getSimValue();
    pmem_addr_0_val  = m_pads[padNameIdMap.at("pmem_addr[0]")]->getSimValue();
  }
  else if (design  == "CORTEXM0PLUS")
  {
    string  HADDR_31_val, HADDR_30_val, HADDR_29_val, HADDR_28_val, HADDR_27_val, HADDR_26_val, HADDR_25_val, HADDR_24_val, HADDR_23_val, HADDR_22_val, HADDR_21_val, HADDR_20_val, HADDR_19_val, HADDR_18_val, HADDR_17_val, HADDR_16_val, HADDR_15_val, HADDR_14_val, HADDR_13_val, HADDR_12_val, HADDR_11_val, HADDR_10_val, HADDR_9_val, HADDR_8_val, HADDR_7_val, HADDR_6_val, HADDR_5_val, HADDR_4_val, HADDR_3_val, HADDR_2_val, HADDR_1_val, HADDR_0_val;
    HADDR_31_val = m_pads[padNameIdMap.at("HADDR[31]")]->getSimValue();
    HADDR_30_val = m_pads[padNameIdMap.at("HADDR[30]")]->getSimValue();
    HADDR_29_val = m_pads[padNameIdMap.at("HADDR[29]")]->getSimValue();
    HADDR_28_val = m_pads[padNameIdMap.at("HADDR[28]")]->getSimValue();

    if (HADDR_31_val + HADDR_30_val + HADDR_29_val + HADDR_28_val == "0000" ) // PMEM IS SELECTED // HSEL
    {
//      string HWRITE = m_pads[padNameIdMap.at("HWRITE")]->getSimValue();
//      assert(HWRITE == "0");
      HADDR_27_val = m_pads[padNameIdMap.at("HADDR[27]")]->getSimValue();
      HADDR_26_val = m_pads[padNameIdMap.at("HADDR[26]")]->getSimValue();
      HADDR_25_val = m_pads[padNameIdMap.at("HADDR[25]")]->getSimValue();
      HADDR_24_val = m_pads[padNameIdMap.at("HADDR[24]")]->getSimValue();
      HADDR_23_val = m_pads[padNameIdMap.at("HADDR[23]")]->getSimValue();
      HADDR_22_val = m_pads[padNameIdMap.at("HADDR[22]")]->getSimValue();
      HADDR_21_val = m_pads[padNameIdMap.at("HADDR[21]")]->getSimValue();
      HADDR_20_val = m_pads[padNameIdMap.at("HADDR[20]")]->getSimValue();
      HADDR_19_val = m_pads[padNameIdMap.at("HADDR[19]")]->getSimValue();
      HADDR_18_val = m_pads[padNameIdMap.at("HADDR[18]")]->getSimValue();
      HADDR_17_val = m_pads[padNameIdMap.at("HADDR[17]")]->getSimValue();
      HADDR_16_val = m_pads[padNameIdMap.at("HADDR[16]")]->getSimValue();
      HADDR_15_val = m_pads[padNameIdMap.at("HADDR[15]")]->getSimValue();
      HADDR_14_val = m_pads[padNameIdMap.at("HADDR[14]")]->getSimValue();
      HADDR_13_val = m_pads[padNameIdMap.at("HADDR[13]")]->getSimValue();
      HADDR_12_val = m_pads[padNameIdMap.at("HADDR[12]")]->getSimValue();
      HADDR_11_val = m_pads[padNameIdMap.at("HADDR[11]")]->getSimValue();
      HADDR_10_val = m_pads[padNameIdMap.at("HADDR[10]")]->getSimValue();
      HADDR_9_val = m_pads[padNameIdMap.at("HADDR[9]")]->getSimValue();
      HADDR_8_val = m_pads[padNameIdMap.at("HADDR[8]")]->getSimValue();
      HADDR_7_val = m_pads[padNameIdMap.at("HADDR[7]")]->getSimValue();
      HADDR_6_val = m_pads[padNameIdMap.at("HADDR[6]")]->getSimValue();
      HADDR_5_val = m_pads[padNameIdMap.at("HADDR[5]")]->getSimValue();
      HADDR_4_val = m_pads[padNameIdMap.at("HADDR[4]")]->getSimValue();
      HADDR_3_val = m_pads[padNameIdMap.at("HADDR[3]")]->getSimValue();
      HADDR_2_val = m_pads[padNameIdMap.at("HADDR[2]")]->getSimValue();
      HADDR_1_val = m_pads[padNameIdMap.at("HADDR[1]")]->getSimValue();
      HADDR_0_val = m_pads[padNameIdMap.at("HADDR[0]")]->getSimValue();
      string pmem_addr_str = HADDR_19_val+ HADDR_18_val+ HADDR_17_val+ HADDR_16_val+ HADDR_15_val+ HADDR_14_val+ HADDR_13_val+ HADDR_12_val+ HADDR_11_val+ HADDR_10_val+ HADDR_9_val+ HADDR_8_val+ HADDR_7_val+ HADDR_6_val+ HADDR_5_val+ HADDR_4_val+ HADDR_3_val+ HADDR_2_val;
      return pmem_addr_str;
    }
    return "";
  }
  else
  {
    assert(0);
  }
    string pmem_addr_str = pmem_addr_13_val+ pmem_addr_12_val+ pmem_addr_11_val+ pmem_addr_10_val+ pmem_addr_9_val + pmem_addr_8_val + pmem_addr_7_val + pmem_addr_6_val + pmem_addr_5_val + pmem_addr_4_val + pmem_addr_3_val + pmem_addr_2_val + pmem_addr_1_val + pmem_addr_0_val ;
    return pmem_addr_str;
}

string PowerOpt::getDmemAddr()
{
  string dmem_addr_12_val , dmem_addr_11_val , dmem_addr_10_val , dmem_addr_9_val  , dmem_addr_8_val  , dmem_addr_7_val  , dmem_addr_6_val  , dmem_addr_5_val  , dmem_addr_4_val  , dmem_addr_3_val  , dmem_addr_2_val  , dmem_addr_1_val  , dmem_addr_0_val;
  if (design == "flat_no_clk_gt")
  {
    dmem_addr_12_val = m_pads[padNameIdMap.at("dmem_addr_12_")]->getSimValue();
    dmem_addr_11_val = m_pads[padNameIdMap.at("dmem_addr_11_")]->getSimValue();
    dmem_addr_10_val = m_pads[padNameIdMap.at("dmem_addr_10_")]->getSimValue();
    dmem_addr_9_val  = m_pads[padNameIdMap.at("dmem_addr_9_")]->getSimValue();
    dmem_addr_8_val  = m_pads[padNameIdMap.at("dmem_addr_8_")]->getSimValue();
    dmem_addr_7_val  = m_pads[padNameIdMap.at("dmem_addr_7_")]->getSimValue();
    dmem_addr_6_val  = m_pads[padNameIdMap.at("dmem_addr_6_")]->getSimValue();
    dmem_addr_5_val  = m_pads[padNameIdMap.at("dmem_addr_5_")]->getSimValue();
    dmem_addr_4_val  = m_pads[padNameIdMap.at("dmem_addr_4_")]->getSimValue();
    dmem_addr_3_val  = m_pads[padNameIdMap.at("dmem_addr_3_")]->getSimValue();
    dmem_addr_2_val  = m_pads[padNameIdMap.at("dmem_addr_2_")]->getSimValue();
    dmem_addr_1_val  = m_pads[padNameIdMap.at("dmem_addr_1_")]->getSimValue();
    dmem_addr_0_val  = m_pads[padNameIdMap.at("dmem_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    dmem_addr_12_val = m_pads[padNameIdMap.at("dmem_addr[12]")]->getSimValue();
    dmem_addr_11_val = m_pads[padNameIdMap.at("dmem_addr[11]")]->getSimValue();
    dmem_addr_10_val = m_pads[padNameIdMap.at("dmem_addr[10]")]->getSimValue();
    dmem_addr_9_val  = m_pads[padNameIdMap.at("dmem_addr[9]")]->getSimValue();
    dmem_addr_8_val  = m_pads[padNameIdMap.at("dmem_addr[8]")]->getSimValue();
    dmem_addr_7_val  = m_pads[padNameIdMap.at("dmem_addr[7]")]->getSimValue();
    dmem_addr_6_val  = m_pads[padNameIdMap.at("dmem_addr[6]")]->getSimValue();
    dmem_addr_5_val  = m_pads[padNameIdMap.at("dmem_addr[5]")]->getSimValue();
    dmem_addr_4_val  = m_pads[padNameIdMap.at("dmem_addr[4]")]->getSimValue();
    dmem_addr_3_val  = m_pads[padNameIdMap.at("dmem_addr[3]")]->getSimValue();
    dmem_addr_2_val  = m_pads[padNameIdMap.at("dmem_addr[2]")]->getSimValue();
    dmem_addr_1_val  = m_pads[padNameIdMap.at("dmem_addr[1]")]->getSimValue();
    dmem_addr_0_val  = m_pads[padNameIdMap.at("dmem_addr[0]")]->getSimValue();
  }
  else if (design == "CORTEXM0PLUS")
  {
    string  HADDR_31_val, HADDR_30_val, HADDR_29_val, HADDR_28_val, HADDR_27_val, HADDR_26_val, HADDR_25_val, HADDR_24_val, HADDR_23_val, HADDR_22_val, HADDR_21_val, HADDR_20_val, HADDR_19_val, HADDR_18_val, HADDR_17_val, HADDR_16_val, HADDR_15_val, HADDR_14_val, HADDR_13_val, HADDR_12_val, HADDR_11_val, HADDR_10_val, HADDR_9_val, HADDR_8_val, HADDR_7_val, HADDR_6_val, HADDR_5_val, HADDR_4_val, HADDR_3_val, HADDR_2_val, HADDR_1_val, HADDR_0_val;
    HADDR_31_val = m_pads[padNameIdMap.at("HADDR[31]")]->getSimValue();
    HADDR_30_val = m_pads[padNameIdMap.at("HADDR[30]")]->getSimValue();
    HADDR_29_val = m_pads[padNameIdMap.at("HADDR[29]")]->getSimValue();
    HADDR_28_val = m_pads[padNameIdMap.at("HADDR[28]")]->getSimValue();
    if (HADDR_31_val + HADDR_30_val + HADDR_29_val + HADDR_28_val == "0010" ) // PMEM IS SELECTED // HSEL
    {
      HADDR_27_val = m_pads[padNameIdMap.at("HADDR[27]")]->getSimValue();
      HADDR_26_val = m_pads[padNameIdMap.at("HADDR[26]")]->getSimValue();
      HADDR_25_val = m_pads[padNameIdMap.at("HADDR[25]")]->getSimValue();
      HADDR_24_val = m_pads[padNameIdMap.at("HADDR[24]")]->getSimValue();
      HADDR_23_val = m_pads[padNameIdMap.at("HADDR[23]")]->getSimValue();
      HADDR_22_val = m_pads[padNameIdMap.at("HADDR[22]")]->getSimValue();
      HADDR_21_val = m_pads[padNameIdMap.at("HADDR[21]")]->getSimValue();
      HADDR_20_val = m_pads[padNameIdMap.at("HADDR[20]")]->getSimValue();
      HADDR_19_val = m_pads[padNameIdMap.at("HADDR[19]")]->getSimValue();
      HADDR_18_val = m_pads[padNameIdMap.at("HADDR[18]")]->getSimValue();
      HADDR_17_val = m_pads[padNameIdMap.at("HADDR[17]")]->getSimValue();
      HADDR_16_val = m_pads[padNameIdMap.at("HADDR[16]")]->getSimValue();
      HADDR_15_val = m_pads[padNameIdMap.at("HADDR[15]")]->getSimValue();
      HADDR_14_val = m_pads[padNameIdMap.at("HADDR[14]")]->getSimValue();
      HADDR_13_val = m_pads[padNameIdMap.at("HADDR[13]")]->getSimValue();
      HADDR_12_val = m_pads[padNameIdMap.at("HADDR[12]")]->getSimValue();
      HADDR_11_val = m_pads[padNameIdMap.at("HADDR[11]")]->getSimValue();
      HADDR_10_val = m_pads[padNameIdMap.at("HADDR[10]")]->getSimValue();
      HADDR_9_val = m_pads[padNameIdMap.at("HADDR[9]")]->getSimValue();
      HADDR_8_val = m_pads[padNameIdMap.at("HADDR[8]")]->getSimValue();
      HADDR_7_val = m_pads[padNameIdMap.at("HADDR[7]")]->getSimValue();
      HADDR_6_val = m_pads[padNameIdMap.at("HADDR[6]")]->getSimValue();
      HADDR_5_val = m_pads[padNameIdMap.at("HADDR[5]")]->getSimValue();
      HADDR_4_val = m_pads[padNameIdMap.at("HADDR[4]")]->getSimValue();
      HADDR_3_val = m_pads[padNameIdMap.at("HADDR[3]")]->getSimValue();
      HADDR_2_val = m_pads[padNameIdMap.at("HADDR[2]")]->getSimValue();
      HADDR_1_val = m_pads[padNameIdMap.at("HADDR[1]")]->getSimValue();
      HADDR_0_val = m_pads[padNameIdMap.at("HADDR[0]")]->getSimValue();
      string dmem_addr_str = HADDR_19_val+ HADDR_18_val+ HADDR_17_val+ HADDR_16_val+ HADDR_15_val+ HADDR_14_val+ HADDR_13_val+ HADDR_12_val+ HADDR_11_val+ HADDR_10_val+ HADDR_9_val+ HADDR_8_val+ HADDR_7_val+ HADDR_6_val+ HADDR_5_val+ HADDR_4_val+ HADDR_3_val+ HADDR_2_val;
      return dmem_addr_str;
    }
    return "";
  }
  else
  {
    assert(0);
  }

    string dmem_addr_str =  dmem_addr_12_val+ dmem_addr_11_val+ dmem_addr_10_val+ dmem_addr_9_val + dmem_addr_8_val + dmem_addr_7_val + dmem_addr_6_val + dmem_addr_5_val + dmem_addr_4_val + dmem_addr_3_val + dmem_addr_2_val + dmem_addr_1_val + dmem_addr_0_val ;
    return dmem_addr_str;
}

string PowerOpt::getDmemDin()
{
  string dmem_din_15_val , dmem_din_14_val , dmem_din_13_val , dmem_din_12_val , dmem_din_11_val , dmem_din_10_val , dmem_din_9_val  , dmem_din_8_val  , dmem_din_7_val  , dmem_din_6_val  , dmem_din_5_val  , dmem_din_4_val  , dmem_din_3_val  , dmem_din_2_val  , dmem_din_1_val  , dmem_din_0_val;
  if (design == "flat_no_clk_gt")
  {
    dmem_din_15_val = m_pads[padNameIdMap.at("dmem_din_15_")]->getSimValue();
    dmem_din_14_val = m_pads[padNameIdMap.at("dmem_din_14_")]->getSimValue();
    dmem_din_13_val = m_pads[padNameIdMap.at("dmem_din_13_")]->getSimValue();
    dmem_din_12_val = m_pads[padNameIdMap.at("dmem_din_12_")]->getSimValue();
    dmem_din_11_val = m_pads[padNameIdMap.at("dmem_din_11_")]->getSimValue();
    dmem_din_10_val = m_pads[padNameIdMap.at("dmem_din_10_")]->getSimValue();
    dmem_din_9_val  = m_pads[padNameIdMap.at("dmem_din_9_")]->getSimValue();
    dmem_din_8_val  = m_pads[padNameIdMap.at("dmem_din_8_")]->getSimValue();
    dmem_din_7_val  = m_pads[padNameIdMap.at("dmem_din_7_")]->getSimValue();
    dmem_din_6_val  = m_pads[padNameIdMap.at("dmem_din_6_")]->getSimValue();
    dmem_din_5_val  = m_pads[padNameIdMap.at("dmem_din_5_")]->getSimValue();
    dmem_din_4_val  = m_pads[padNameIdMap.at("dmem_din_4_")]->getSimValue();
    dmem_din_3_val  = m_pads[padNameIdMap.at("dmem_din_3_")]->getSimValue();
    dmem_din_2_val  = m_pads[padNameIdMap.at("dmem_din_2_")]->getSimValue();
    dmem_din_1_val  = m_pads[padNameIdMap.at("dmem_din_1_")]->getSimValue();
    dmem_din_0_val  = m_pads[padNameIdMap.at("dmem_din_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    dmem_din_15_val = m_pads[padNameIdMap.at("dmem_din[15]")]->getSimValue();
    dmem_din_14_val = m_pads[padNameIdMap.at("dmem_din[14]")]->getSimValue();
    dmem_din_13_val = m_pads[padNameIdMap.at("dmem_din[13]")]->getSimValue();
    dmem_din_12_val = m_pads[padNameIdMap.at("dmem_din[12]")]->getSimValue();
    dmem_din_11_val = m_pads[padNameIdMap.at("dmem_din[11]")]->getSimValue();
    dmem_din_10_val = m_pads[padNameIdMap.at("dmem_din[10]")]->getSimValue();
    dmem_din_9_val  = m_pads[padNameIdMap.at("dmem_din[9]")]->getSimValue();
    dmem_din_8_val  = m_pads[padNameIdMap.at("dmem_din[8]")]->getSimValue();
    dmem_din_7_val  = m_pads[padNameIdMap.at("dmem_din[7]")]->getSimValue();
    dmem_din_6_val  = m_pads[padNameIdMap.at("dmem_din[6]")]->getSimValue();
    dmem_din_5_val  = m_pads[padNameIdMap.at("dmem_din[5]")]->getSimValue();
    dmem_din_4_val  = m_pads[padNameIdMap.at("dmem_din[4]")]->getSimValue();
    dmem_din_3_val  = m_pads[padNameIdMap.at("dmem_din[3]")]->getSimValue();
    dmem_din_2_val  = m_pads[padNameIdMap.at("dmem_din[2]")]->getSimValue();
    dmem_din_1_val  = m_pads[padNameIdMap.at("dmem_din[1]")]->getSimValue();
    dmem_din_0_val  = m_pads[padNameIdMap.at("dmem_din[0]")]->getSimValue();
  }
  else if (design == "CORTEXM0PLUS")
  {
    string  HWDATA_31_val, HWDATA_30_val, HWDATA_29_val, HWDATA_28_val, HWDATA_27_val, HWDATA_26_val, HWDATA_25_val, HWDATA_24_val, HWDATA_23_val, HWDATA_22_val, HWDATA_21_val, HWDATA_20_val, HWDATA_19_val, HWDATA_18_val, HWDATA_17_val, HWDATA_16_val, HWDATA_15_val, HWDATA_14_val, HWDATA_13_val, HWDATA_12_val, HWDATA_11_val, HWDATA_10_val, HWDATA_9_val, HWDATA_8_val, HWDATA_7_val, HWDATA_6_val, HWDATA_5_val, HWDATA_4_val, HWDATA_3_val, HWDATA_2_val, HWDATA_1_val, HWDATA_0_val;
    HWDATA_31_val = m_pads[padNameIdMap.at("HWDATA[31]")]->getSimValue();
    HWDATA_30_val = m_pads[padNameIdMap.at("HWDATA[30]")]->getSimValue();
    HWDATA_29_val = m_pads[padNameIdMap.at("HWDATA[29]")]->getSimValue();
    HWDATA_28_val = m_pads[padNameIdMap.at("HWDATA[28]")]->getSimValue();
    HWDATA_27_val = m_pads[padNameIdMap.at("HWDATA[27]")]->getSimValue();
    HWDATA_26_val = m_pads[padNameIdMap.at("HWDATA[26]")]->getSimValue();
    HWDATA_25_val = m_pads[padNameIdMap.at("HWDATA[25]")]->getSimValue();
    HWDATA_24_val = m_pads[padNameIdMap.at("HWDATA[24]")]->getSimValue();
    HWDATA_23_val = m_pads[padNameIdMap.at("HWDATA[23]")]->getSimValue();
    HWDATA_22_val = m_pads[padNameIdMap.at("HWDATA[22]")]->getSimValue();
    HWDATA_21_val = m_pads[padNameIdMap.at("HWDATA[21]")]->getSimValue();
    HWDATA_20_val = m_pads[padNameIdMap.at("HWDATA[20]")]->getSimValue();
    HWDATA_19_val = m_pads[padNameIdMap.at("HWDATA[19]")]->getSimValue();
    HWDATA_18_val = m_pads[padNameIdMap.at("HWDATA[18]")]->getSimValue();
    HWDATA_17_val = m_pads[padNameIdMap.at("HWDATA[17]")]->getSimValue();
    HWDATA_16_val = m_pads[padNameIdMap.at("HWDATA[16]")]->getSimValue();
    HWDATA_15_val = m_pads[padNameIdMap.at("HWDATA[15]")]->getSimValue();
    HWDATA_14_val = m_pads[padNameIdMap.at("HWDATA[14]")]->getSimValue();
    HWDATA_13_val = m_pads[padNameIdMap.at("HWDATA[13]")]->getSimValue();
    HWDATA_12_val = m_pads[padNameIdMap.at("HWDATA[12]")]->getSimValue();
    HWDATA_11_val = m_pads[padNameIdMap.at("HWDATA[11]")]->getSimValue();
    HWDATA_10_val = m_pads[padNameIdMap.at("HWDATA[10]")]->getSimValue();
    HWDATA_9_val = m_pads[padNameIdMap.at("HWDATA[9]")]->getSimValue();
    HWDATA_8_val = m_pads[padNameIdMap.at("HWDATA[8]")]->getSimValue();
    HWDATA_7_val = m_pads[padNameIdMap.at("HWDATA[7]")]->getSimValue();
    HWDATA_6_val = m_pads[padNameIdMap.at("HWDATA[6]")]->getSimValue();
    HWDATA_5_val = m_pads[padNameIdMap.at("HWDATA[5]")]->getSimValue();
    HWDATA_4_val = m_pads[padNameIdMap.at("HWDATA[4]")]->getSimValue();
    HWDATA_3_val = m_pads[padNameIdMap.at("HWDATA[3]")]->getSimValue();
    HWDATA_2_val = m_pads[padNameIdMap.at("HWDATA[2]")]->getSimValue();
    HWDATA_1_val = m_pads[padNameIdMap.at("HWDATA[1]")]->getSimValue();
    HWDATA_0_val = m_pads[padNameIdMap.at("HWDATA[0]")]->getSimValue();
    string dmem_data_str = HWDATA_31_val+ HWDATA_30_val+ HWDATA_29_val+ HWDATA_28_val+ HWDATA_27_val+ HWDATA_26_val+ HWDATA_25_val+ HWDATA_24_val+ HWDATA_23_val+ HWDATA_22_val+ HWDATA_21_val+ HWDATA_20_val+ HWDATA_19_val+ HWDATA_18_val+ HWDATA_17_val+ HWDATA_16_val+ HWDATA_15_val+ HWDATA_14_val+ HWDATA_13_val+ HWDATA_12_val+ HWDATA_11_val+ HWDATA_10_val+ HWDATA_9_val+ HWDATA_8_val+ HWDATA_7_val+ HWDATA_6_val+ HWDATA_5_val+ HWDATA_4_val+ HWDATA_3_val+ HWDATA_2_val+ HWDATA_1_val+ HWDATA_0_val;
    return dmem_data_str;

  }
  else
  {
    assert(0);
  }

  string dmem_din_str = dmem_din_15_val+ dmem_din_14_val+ dmem_din_13_val+ dmem_din_12_val+ dmem_din_11_val+ dmem_din_10_val+ dmem_din_9_val + dmem_din_8_val + dmem_din_7_val + dmem_din_6_val + dmem_din_5_val + dmem_din_4_val + dmem_din_3_val + dmem_din_2_val + dmem_din_1_val + dmem_din_0_val ;
    //string dmem_din_str = dmem_din_0_val+ dmem_din_1_val+ dmem_din_2_val+ dmem_din_3_val+ dmem_din_4_val+ dmem_din_5_val+ dmem_din_6_val+ dmem_din_7_val+ dmem_din_8_val+ dmem_din_9_val+ dmem_din_10_val+ dmem_din_11_val+ dmem_din_12_val+ dmem_din_13_val+ dmem_din_14_val+ dmem_din_15_val;

    return dmem_din_str;
}

string PowerOpt::getDmemLow()
{
  string dmem_din_7_val , dmem_din_6_val , dmem_din_5_val , dmem_din_4_val , dmem_din_3_val , dmem_din_2_val , dmem_din_1_val , dmem_din_0_val ;
  if (design == "flat_no_clk_gt")
  {
    dmem_din_7_val  = m_pads[padNameIdMap.at("dmem_din_7_")]->getSimValue();
    dmem_din_6_val  = m_pads[padNameIdMap.at("dmem_din_6_")]->getSimValue();
    dmem_din_5_val  = m_pads[padNameIdMap.at("dmem_din_5_")]->getSimValue();
    dmem_din_4_val  = m_pads[padNameIdMap.at("dmem_din_4_")]->getSimValue();
    dmem_din_3_val  = m_pads[padNameIdMap.at("dmem_din_3_")]->getSimValue();
    dmem_din_2_val  = m_pads[padNameIdMap.at("dmem_din_2_")]->getSimValue();
    dmem_din_1_val  = m_pads[padNameIdMap.at("dmem_din_1_")]->getSimValue();
    dmem_din_0_val  = m_pads[padNameIdMap.at("dmem_din_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    dmem_din_7_val  = m_pads[padNameIdMap.at("dmem_din[7]")]->getSimValue();
    dmem_din_6_val  = m_pads[padNameIdMap.at("dmem_din[6]")]->getSimValue();
    dmem_din_5_val  = m_pads[padNameIdMap.at("dmem_din[5]")]->getSimValue();
    dmem_din_4_val  = m_pads[padNameIdMap.at("dmem_din[4]")]->getSimValue();
    dmem_din_3_val  = m_pads[padNameIdMap.at("dmem_din[3]")]->getSimValue();
    dmem_din_2_val  = m_pads[padNameIdMap.at("dmem_din[2]")]->getSimValue();
    dmem_din_1_val  = m_pads[padNameIdMap.at("dmem_din[1]")]->getSimValue();
    dmem_din_0_val  = m_pads[padNameIdMap.at("dmem_din[0]")]->getSimValue();
  }
  else
  {
    assert(0);
  }

    string dmem_din_str = dmem_din_7_val + dmem_din_6_val + dmem_din_5_val + dmem_din_4_val + dmem_din_3_val + dmem_din_2_val + dmem_din_1_val + dmem_din_0_val ;
    return dmem_din_str;
}

string PowerOpt::getDmemHigh()
{
  string dmem_din_15_val , dmem_din_14_val , dmem_din_13_val , dmem_din_12_val , dmem_din_11_val , dmem_din_10_val , dmem_din_9_val  , dmem_din_8_val;
  if (design == "flat_no_clk_gt")
  {
    dmem_din_15_val = m_pads[padNameIdMap.at("dmem_din_15_")]->getSimValue();
    dmem_din_14_val = m_pads[padNameIdMap.at("dmem_din_14_")]->getSimValue();
    dmem_din_13_val = m_pads[padNameIdMap.at("dmem_din_13_")]->getSimValue();
    dmem_din_12_val = m_pads[padNameIdMap.at("dmem_din_12_")]->getSimValue();
    dmem_din_11_val = m_pads[padNameIdMap.at("dmem_din_11_")]->getSimValue();
    dmem_din_10_val = m_pads[padNameIdMap.at("dmem_din_10_")]->getSimValue();
    dmem_din_9_val  = m_pads[padNameIdMap.at("dmem_din_9_")]->getSimValue();
    dmem_din_8_val  = m_pads[padNameIdMap.at("dmem_din_8_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    dmem_din_15_val = m_pads[padNameIdMap.at("dmem_din[15]")]->getSimValue();
    dmem_din_14_val = m_pads[padNameIdMap.at("dmem_din[14]")]->getSimValue();
    dmem_din_13_val = m_pads[padNameIdMap.at("dmem_din[13]")]->getSimValue();
    dmem_din_12_val = m_pads[padNameIdMap.at("dmem_din[12]")]->getSimValue();
    dmem_din_11_val = m_pads[padNameIdMap.at("dmem_din[11]")]->getSimValue();
    dmem_din_10_val = m_pads[padNameIdMap.at("dmem_din[10]")]->getSimValue();
    dmem_din_8_val  = m_pads[padNameIdMap.at("dmem_din[8]")]->getSimValue();
    dmem_din_9_val  = m_pads[padNameIdMap.at("dmem_din[9]")]->getSimValue();
  }
    string dmem_din_str = dmem_din_15_val+ dmem_din_14_val+ dmem_din_13_val+ dmem_din_12_val+ dmem_din_11_val+ dmem_din_10_val+ dmem_din_9_val + dmem_din_8_val;
    return dmem_din_str;
}

xbitset operator & (xbitset a, xbitset b)
{
  bitset<NUM_BITS> bs_x = (a.get_bs_x() & b.get_bs_val()) | (a.get_bs_val() & b.get_bs_x()) | (a.get_bs_x() & b.get_bs_x());
  bitset<NUM_BITS> bs_val = a.get_bs_val() & b.get_bs_val();
  return xbitset(bs_x, bs_val);
}

xbitset operator | (xbitset a, xbitset b)
{
  // Slightly buggy, as it ignores the fact that val can be a 1 when x is a 1. Current implementation makes that impossible however.
  bitset<NUM_BITS> bs_x = (a.get_bs_x() | b.get_bs_x()) & (~a.get_bs_val()) & (~b.get_bs_val()) ;
  bitset<NUM_BITS> bs_val = a.get_bs_val() | b.get_bs_val();
  return xbitset(bs_x, bs_val);
}


void PowerOpt::writeDmem(int cycle_num)
{
  if (dmem_write != true) return;
  if (hasX(dmem_addr_str)) return;
  xbitset Dmem_higher;
  xbitset Dmem_lower;
  xbitset Dmem_except_byte_00;
  xbitset Dmem_except_byte_01;
  xbitset Dmem_except_byte_10;
  xbitset Dmem_except_byte_11;
  string dmem_din_str = getDmemDin();
  xbitset dmem_din(dmem_din_str);
     if (design == "CORTEXM0PLUS")
     {
       switch(dmem_write_mode)
       {
         case 0:  // WRITE THE WHOLE WORD
            DMemory[dmem_addr] = dmem_din;
            dmem_request_file << " Writing Data (full) " << dmem_din_str << " at cycle " << cycle_num <<  endl;
            break;
         case 1: // WRITE LOWER 16
            Dmem_higher = DMemory[dmem_addr] & xbitset(4294901760); // upper 16 bits are 1
            DMemory[dmem_addr] = Dmem_higher | (dmem_din & xbitset(65535)); // pass lower bits of dmem_din
            dmem_request_file << "Writing Data (low 16) " << dmem_din_str << " at cycle " << cycle_num<< endl;
            break;
         case 2: // WRITE UPPER 16
            Dmem_lower = DMemory[dmem_addr] & xbitset(65535); // lower 16 bits are 1
            DMemory[dmem_addr] = Dmem_lower | (dmem_din & xbitset(4294901760)); // pass upper bits of dmem_din
            dmem_request_file << "Writing Data (high 16) " << dmem_din_str << " at cycle " << cycle_num<< endl;
            break;
         case 3:
            Dmem_except_byte_00 = DMemory[dmem_addr] & xbitset(4294967040); // upper 24 bits
            DMemory[dmem_addr] = Dmem_except_byte_00 | (dmem_din & xbitset(255)); // pass lower bits of dmem_din
            dmem_request_file << "Writing Data (byte 00) " << dmem_din_str << " at cycle " << cycle_num<< endl;
            break;
         case 4:
            Dmem_except_byte_01 = DMemory[dmem_addr] & xbitset(4294902015); // upper 16 bits and lower 8 bits
            DMemory[dmem_addr] = Dmem_except_byte_01 | (dmem_din & xbitset(65280)); // pass second lower 8 bits
            dmem_request_file << "Writing Data (byte 01) " << dmem_din_str << " at cycle " << cycle_num<< endl;
            break;
         case 5:
            Dmem_except_byte_10 = DMemory[dmem_addr] & xbitset(4278255615); // upper 8 bits and lower 16 bits
            DMemory[dmem_addr] = Dmem_except_byte_10 | (dmem_din & xbitset(16711680)); // pass third lower 8 bits
            dmem_request_file << "Writing Data (byte 10) " << dmem_din_str<< " at cycle " << cycle_num << endl;
            break;
         case 6:
            Dmem_except_byte_11 = DMemory[dmem_addr] & xbitset(16777215); // lower 24 bits
            DMemory[dmem_addr] = Dmem_except_byte_10 | (dmem_din & xbitset(4278190080)); // pass upper 8 bits
            dmem_request_file << "Writing Data (byte 11) " << dmem_din_str << " at cycle " << cycle_num<< endl;
            break;
         default : assert(0);
       }
       dmem_write = false;

       return ;
     }
     assert(0);
}


void PowerOpt::handleDmem(int cycle_num)
{
     string dmem_cen = m_pads[padNameIdMap["dmem_cen"]]->getSimValue();
     if (dmem_cen != "0") return;
     string dmem_wen_0, dmem_wen_1;
     if (design =="flat_no_clk_gt")
     {
       dmem_wen_0 = m_pads[padNameIdMap["dmem_wen_0_"]]->getSimValue();
       dmem_wen_1 = m_pads[padNameIdMap["dmem_wen_1_"]]->getSimValue();
     }
     else if (design == "modified_9_hier")
     {
       dmem_wen_0 = m_pads[padNameIdMap["dmem_wen[0]"]]->getSimValue();
       dmem_wen_1 = m_pads[padNameIdMap["dmem_wen[1]"]]->getSimValue();
     }
     string dmem_wen = dmem_wen_1 + dmem_wen_0;
     unsigned int dmem_wen_int = strtoull(dmem_wen.c_str(), NULL, 2);
     string dmem_addr_str = getDmemAddr();
     unsigned int dmem_addr = strtoull(dmem_addr_str.c_str(), NULL, 2);
     string dmem_din_str = getDmemDin();
     xbitset dmem_din(dmem_din_str);
     xbitset Dmem_lower;
     xbitset Dmem_higher;
     dmem_request_file << " Accessing Dmem with address : " << hex << dmem_addr << dec << " and write mode  " << dmem_wen_int << ". Size of Memory is " << DMemory.size()  <<  " And cycle num is " << cycle_num << endl;
     pmem_request_file << " Accessing Dmem with address : " << hex << dmem_addr << dec << " and write mode  " << dmem_wen_int << ". Size of Memory is " << DMemory.size()  <<  " And cycle num is " << cycle_num << endl;


     switch (dmem_wen_int)
     {
       case 0:  // WRITE BOTH BYTES
         DMemory[dmem_addr] = dmem_din;
         dmem_request_file << "Writing Data (full) " << dmem_din_str << endl;
         break;
       case 1: // WRITE HIGH BYTE
         Dmem_lower = DMemory[dmem_addr] & xbitset(255);// 11111111
         DMemory[dmem_addr] = Dmem_lower | (dmem_din & xbitset(65280) );// 1111111100000000
         dmem_request_file << "Writing Data (high) " << dmem_din_str << endl;
         break;
       case 2: // WRITE LOW BYTE
         Dmem_higher = DMemory[dmem_addr] & xbitset(65280);
         DMemory[dmem_addr] = Dmem_higher | (dmem_din & xbitset(255) );
         dmem_request_file << "Writing Data (low) " << dmem_din_str << endl;
         break;
       case 3: // READ
         //sendData(DMemory[dmem_addr].to_string());
         dmem_data = DMemory[dmem_addr].to_string();
         send_data = true;
         break;
     }

}

void PowerOpt::sendHRData(string data_str)
{
  std::reverse(data_str.begin(), data_str.end());
  m_pads[padNameIdMap.at("HRDATA[31]")]->setSimValue(data_str.substr(31,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[30]")]->setSimValue(data_str.substr(30,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[29]")]->setSimValue(data_str.substr(29,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[28]")]->setSimValue(data_str.substr(28,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[27]")]->setSimValue(data_str.substr(27,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[26]")]->setSimValue(data_str.substr(26,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[25]")]->setSimValue(data_str.substr(25,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[24]")]->setSimValue(data_str.substr(24,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[23]")]->setSimValue(data_str.substr(23,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[22]")]->setSimValue(data_str.substr(22,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[21]")]->setSimValue(data_str.substr(21,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[20]")]->setSimValue(data_str.substr(20,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[19]")]->setSimValue(data_str.substr(19,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[18]")]->setSimValue(data_str.substr(18,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[17]")]->setSimValue(data_str.substr(17,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[16]")]->setSimValue(data_str.substr(16,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[15]")]->setSimValue(data_str.substr(15,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[14]")]->setSimValue(data_str.substr(14,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[13]")]->setSimValue(data_str.substr(13,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[12]")]->setSimValue(data_str.substr(12,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[11]")]->setSimValue(data_str.substr(11,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[10]")]->setSimValue(data_str.substr(10,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[9]")]->setSimValue(data_str.substr(9,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[8]")]->setSimValue(data_str.substr(8,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[7]")]->setSimValue(data_str.substr(7,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[6]")]->setSimValue(data_str.substr(6,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[5]")]->setSimValue(data_str.substr(5,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[4]")]->setSimValue(data_str.substr(4,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[3]")]->setSimValue(data_str.substr(3,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[2]")]->setSimValue(data_str.substr(2,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[1]")]->setSimValue(data_str.substr(1,1), sim_wf);
  m_pads[padNameIdMap.at("HRDATA[0]")]->setSimValue(data_str.substr(0,1), sim_wf);
}

void PowerOpt::handleHaddrBools(int cycle_num)
{
  if (design != "CORTEXM0PLUS") return ;
  if (send_instr == true)
  {
    sendHRData(pmem_instr);
    send_instr = false;
  }
  else if (send_data == true)
  {
    sendHRData(dmem_data);
    send_data = false;

  }
  else if (periph_selected)
  {
    string data_str ;
    if (send_input == true && input_count < inputs.size())
    {
       data_str = inputs[input_count];
       input_count++;
       sendHRData(data_str);
       send_input= false;
       debug_file_second << " sending data " << data_str << " input_count is " << input_count <<  endl;
       cout << " sending data " << data_str << " input_count is " << input_count <<  endl;
       pmem_request_file << " sending data " << data_str << " input_count is " << input_count <<  endl;
    }
    //else assert(0);
  }
  else
  {
    string allX = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    string all0 = "00000000000000000000000000000000";
    sendHRData(all0);
  }


}

string PowerOpt::getRegVal(string regName, int start_bit, int end_bit)
{
  stringstream ss;
  string returnVal;
  if (design  == "CORTEXM0PLUS")
  {
    for (int i = end_bit; i >= start_bit; i--)
    {
      ss << "u_top_u_sys_u_core_"<< regName;
      ss << "_reg_" << i << "_";
      string reg_name = ss.str();
      string val;
      string term_name = reg_name+"/Q";
      map<string, int>::iterator it = terminalNameIdMap.find(term_name);
      if (it == terminalNameIdMap.end())
      {
        term_name = reg_name+"/QN";
        it = terminalNameIdMap.find(term_name);
        if (it == terminalNameIdMap.end())
        {
          val = "0";
        }
        else
        {
          val = terms[it->second]->getSimValue();
          if (val == "1") val = "0";
          else if (val == "0") val = "1";
          else if (val =="X") val = "X";
          else assert(0);
        }
      }
      else val = terms[it->second]->getSimValue();
      if (regName == "op_q" && i == 8)
      {
        if (val == "1") val = "0";
        else if (val == "0") val = "1";
        else if (val =="X") val = "X";
        else assert(0);
      }
      returnVal += val;
      ss.str("");
    }
  }
  return returnVal;

}

string PowerOpt::getPortVal(string portName, int start_bit, int end_bit, char separator)
{
    assert(design == "CORTEXM0PLUS" || design == "PLASTICARM");
    string returnVal;
    for (int i = end_bit; i >= start_bit; i--)
    {
       stringstream ss;
       ss << portName << "[" << i << "]";
       string port_name = ss.str();
       string val = m_pads[padNameIdMap.at(port_name)] ->getSimValue();
       returnVal += val;
    }
    return returnVal;
}

string PowerOpt::getNetVal(string netName, int start_bit, int end_bit, char separator)
{
    assert(design == "CORTEXM0PLUS" || design == "PLASTICARM");
    string returnVal;
    for (int i = end_bit; i >= start_bit; i--)
    {
       stringstream ss;
       ss << netName << "[" << i << "]";
       string net_name = ss.str();
       string val = nets[netNameIdMap.at(net_name)] ->getSimValue();
       returnVal += val;
    }
    return returnVal;
}

bool PowerOpt::handleHaddr(int cycle_num)
{
    if (design != "CORTEXM0PLUS") return false;
    string  HADDR_31_val, HADDR_30_val, HADDR_29_val, HADDR_28_val, HADDR_27_val, HADDR_26_val, HADDR_25_val, HADDR_24_val, HADDR_23_val, HADDR_22_val, HADDR_21_val, HADDR_20_val, HADDR_19_val, HADDR_18_val, HADDR_17_val, HADDR_16_val, HADDR_15_val, HADDR_14_val, HADDR_13_val, HADDR_12_val, HADDR_11_val, HADDR_10_val, HADDR_9_val, HADDR_8_val, HADDR_7_val, HADDR_6_val, HADDR_5_val, HADDR_4_val, HADDR_3_val, HADDR_2_val, HADDR_1_val, HADDR_0_val;
    HADDR_31_val = m_pads[padNameIdMap.at("HADDR[31]")]->getSimValue();
    HADDR_30_val = m_pads[padNameIdMap.at("HADDR[30]")]->getSimValue();
    HADDR_29_val = m_pads[padNameIdMap.at("HADDR[29]")]->getSimValue();
    HADDR_28_val = m_pads[padNameIdMap.at("HADDR[28]")]->getSimValue();
    string HADDR_upper_nibble = HADDR_31_val + HADDR_30_val + HADDR_29_val + HADDR_28_val;
    string HADDR = getPortVal("HADDR", 0, 31,  '[');
    string HRDATA = getPortVal("HRDATA", 0, 31, '[');
    string HWDATA = getPortVal("HWDATA", 0, 31, '[');
    string op_q = getRegVal("op_q", 0,  15);
    if (hasX(HADDR) && HADDR_upper_nibble != "0010")
    {
      if (cycle_num > 5)
      {
        cout << "HADDR has X" << endl;
        pmem_request_file << "HADDR has X" << endl;
        pmem_request_file << " HADDR is " << HADDR << " and HRDATA is " << HRDATA << " op_q is " << op_q << " at cycle " << cycle_num << endl;
        debug_file_second << " HADDR is " << HADDR << " at cycle " << cycle_num << endl;
        debug_file_second << " HRDATA is " << HRDATA << " at cycle " << cycle_num << endl;
        debug_file_second << " HWDATA is " << HWDATA << " at cycle " << cycle_num << endl;
        return true;
      }
      return false;
    }
    else
    {
      debug_file << " HADDR is " << HADDR << " and HRDATA is " << HRDATA << " op_q is " << op_q << " at cycle " << cycle_num << endl;
      if (hasX(HADDR))
      {
        pmem_request_file << " HADDR is " << HADDR << " and HRDATA is " << HRDATA << " op_q is " << op_q << " at cycle " << cycle_num << endl;
      }
      else
      {
        pmem_request_file << " HADDR is " << bin2hex32(HADDR) << " and HRDATA is " << HRDATA << " op_q is " << op_q << " at cycle " << cycle_num << endl;
      }
      //debug_file << " HADDR is " << HADDR << " and HRDATA is " << HRDATA << " at cycle " << cycle_num << endl;
      debug_file_second << " HADDR is " << HADDR << " at cycle " << cycle_num << endl;
      debug_file_second << " HRDATA is " << HRDATA << " at cycle " << cycle_num << endl;
      debug_file_second << " HWDATA is " << HWDATA << " at cycle " << cycle_num << endl;
      if (HADDR_upper_nibble == "0000") // PMEM
      {
        string pmem_addr_str = getPmemAddr();
        string HTRANS_1 = m_pads[padNameIdMap.at("HTRANS[1]")]->getSimValue();
        send_instr = true;
        if (HTRANS_1 == "1" && pmem_addr_str != "") // READ DATA NOW
        {
          pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);// binary to int
          if (PMemory.find(pmem_addr) != PMemory.end())
          {
            instr = PMemory[pmem_addr]->instr;
            pmem_instr = bitset<32>(instr).to_string();
          }
          //pmem_instr = binary(instr);
          //else
          //pmem_instr = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
          //string pmem_instr_hex = bin2hex(pmem_instr);
          //pmem_request_file << " PMEM ADDR IS " << pmem_addr_str << " (" << bin2hex32(pmem_addr_str) << ") INSTR IS " << pmem_instr << " (" << bin2hex32(pmem_instr) << ") at cycle " << cycle_num <<  endl;
          return 0;
        }
        else return 0;
      }
      if (HADDR_upper_nibble == "0010") // DMEM
      {
        send_data = true;
        dmem_data = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
        //dmem_data = "00000000000000000000000000000000";
        //string HSEL = m_pads[padNameIdMap.at("HSEL")]->getSimValue();
        debug_file_second << " RAM " << endl;
        string HTRANS_1 = m_pads[padNameIdMap.at("HTRANS[1]")]->getSimValue();
        string HREADY = m_pads[padNameIdMap.at("HREADY")]->getSimValue();
        string HWRITE = m_pads[padNameIdMap.at("HWRITE")]->getSimValue();
        bool AHB_ACCESS = (HTRANS_1 == "1" && HREADY == "1");
        bool AHB_WRITE = AHB_ACCESS && (HWRITE == "1");
        bool RAM_WRITE = AHB_WRITE;// SKIPPING MODELING BUFFERED WRITES
        bool RAM_READ = AHB_ACCESS && (HWRITE == "0");// SKIPPING MODELING BUFFERED WRITES
        string HADDR_1_val = m_pads[padNameIdMap.at("HADDR[1]")]->getSimValue();
        string HADDR_0_val = m_pads[padNameIdMap.at("HADDR[0]")]->getSimValue();
        string HSIZE_1_val = m_pads[padNameIdMap.at("HSIZE[1]")]->getSimValue();
        string HSIZE_0_val = m_pads[padNameIdMap.at("HSIZE[0]")]->getSimValue();
        dmem_addr_str = getDmemAddr();
        dmem_addr = strtoull(dmem_addr_str.c_str(), NULL, 2);
        if (RAM_WRITE == true)
        {
          dmem_write = true;
          //if (hasX(dmem_addr_str)) { cout << " WRITING TO ADDRESS WITH X" << endl; }
          if (HSIZE_1_val == "1") // WRITE THE WHOLE WORD
          {
            dmem_write_mode = 0;
          }
          else if (HSIZE_1_val == "0" && HSIZE_0_val == "1") // WRITE HALF WORD 16 bits
          {
            if (HADDR_1_val == "0") // WRITE LOWER 16
            {
              dmem_write_mode = 1;
            }
            else if  (HADDR_1_val == "1") // UPPER 16
            {
              dmem_write_mode = 2;
            }
            else assert(0);
          }
          else if (HSIZE_1_val == "0" && HSIZE_0_val == "0") // WRITE BYTE
          {
            if (HADDR_1_val == "0" && HADDR_0_val == "0") // WRITE BYTE 00
            {
              dmem_write_mode = 3;
            }
            else if  (HADDR_1_val == "0" && HADDR_0_val == "1") // WRITE BYTE 01
            {
              dmem_write_mode = 4;
            }
            else if (HADDR_1_val == "1" && HADDR_0_val == "0") // WRITE BYTE 10
            {
              dmem_write_mode = 5;
            }
            else if (HADDR_1_val == "1" && HADDR_0_val == "1")
            {
              dmem_write_mode = 6;
            }
            else assert(0);

          }
          else assert(0);
        }
        else if (RAM_READ == true)
        {
          if ( hasX(dmem_addr_str) || DMemory.find(dmem_addr) == DMemory.end())
            //dmem_data = "00000000000000000000000000000000";
            dmem_data = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
          else
            dmem_data = DMemory[dmem_addr].to_string();
        }
      }
      if (HADDR_upper_nibble == "0100") // PERIPH
      {
        periph_selected = true;
        if (HADDR == "01000000000000000001000000000100")
          send_input = true;
        if (HADDR == "01000000000000000001000000001100")
          read_output = true;
      }
      else
      {
        return false;
        //if ( cycle_num > 1) assert(0);
      }
    }
    return false;
}

bool PowerOpt::readOutputs()
{
  if (!read_output || design != "CORTEXM0PLUS")  return false;
  string HWDATA = getPortVal("HWDATA", 0, 31, '[');
  //string HWDATA_hex = bin2hex(HWDATA);
  output_value_file << HWDATA << endl;
  read_output = false;

}


bool PowerOpt::sendInputs_new()
{
  static int i = 0;
  if (send_input)
  {
    string data_str ;
    if (i < inputs.size())
    {
      data_str = inputs[i]; i++;
    }
    else assert(0);
    sendHRData(data_str);
    send_input= false;
    debug_file_second << " sending data " << data_str << " i is " << i <<  endl;
  }
}

bool PowerOpt::sendInputs()
{
  static int i = 0;
	string per_enable_val  = m_pads[padNameIdMap.at("per_en")]->getSimValue();
  if (per_enable_val != "1") return false;
  string per_we_0_val, per_we_1_val;
  if (design == "flat_no_clk_gt")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we_1_")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we[1]")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we[0]")]->getSimValue();
  }
  else assert(0);

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "00") return false;

  string per_addr_13_val , per_addr_12_val , per_addr_11_val , per_addr_10_val , per_addr_9_val  , per_addr_8_val  , per_addr_7_val  , per_addr_6_val  , per_addr_5_val  , per_addr_4_val  , per_addr_3_val  , per_addr_2_val  , per_addr_1_val  , per_addr_0_val;
  if (design == "flat_no_clk_gt")
  {
    per_addr_13_val = m_pads[padNameIdMap.at("per_addr_13_")]->getSimValue();
    per_addr_12_val = m_pads[padNameIdMap.at("per_addr_12_")]->getSimValue();
    per_addr_11_val = m_pads[padNameIdMap.at("per_addr_11_")]->getSimValue();
    per_addr_10_val = m_pads[padNameIdMap.at("per_addr_10_")]->getSimValue();
    per_addr_9_val  = m_pads[padNameIdMap.at("per_addr_9_")]->getSimValue();
    per_addr_8_val  = m_pads[padNameIdMap.at("per_addr_8_")]->getSimValue();
    per_addr_7_val  = m_pads[padNameIdMap.at("per_addr_7_")]->getSimValue();
    per_addr_6_val  = m_pads[padNameIdMap.at("per_addr_6_")]->getSimValue();
    per_addr_5_val  = m_pads[padNameIdMap.at("per_addr_5_")]->getSimValue();
    per_addr_4_val  = m_pads[padNameIdMap.at("per_addr_4_")]->getSimValue();
    per_addr_3_val  = m_pads[padNameIdMap.at("per_addr_3_")]->getSimValue();
    per_addr_2_val  = m_pads[padNameIdMap.at("per_addr_2_")]->getSimValue();
    per_addr_1_val  = m_pads[padNameIdMap.at("per_addr_1_")]->getSimValue();
    per_addr_0_val  = m_pads[padNameIdMap.at("per_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_addr_13_val = m_pads[padNameIdMap.at("per_addr[13]")]->getSimValue();
    per_addr_12_val = m_pads[padNameIdMap.at("per_addr[12]")]->getSimValue();
    per_addr_11_val = m_pads[padNameIdMap.at("per_addr[11]")]->getSimValue();
    per_addr_10_val = m_pads[padNameIdMap.at("per_addr[10]")]->getSimValue();
    per_addr_9_val  = m_pads[padNameIdMap.at("per_addr[9]")]->getSimValue();
    per_addr_8_val  = m_pads[padNameIdMap.at("per_addr[8]")]->getSimValue();
    per_addr_7_val  = m_pads[padNameIdMap.at("per_addr[7]")]->getSimValue();
    per_addr_6_val  = m_pads[padNameIdMap.at("per_addr[6]")]->getSimValue();
    per_addr_5_val  = m_pads[padNameIdMap.at("per_addr[5]")]->getSimValue();
    per_addr_4_val  = m_pads[padNameIdMap.at("per_addr[4]")]->getSimValue();
    per_addr_3_val  = m_pads[padNameIdMap.at("per_addr[3]")]->getSimValue();
    per_addr_2_val  = m_pads[padNameIdMap.at("per_addr[2]")]->getSimValue();
    per_addr_1_val  = m_pads[padNameIdMap.at("per_addr[1]")]->getSimValue();
    per_addr_0_val  = m_pads[padNameIdMap.at("per_addr[0]")]->getSimValue();
  }
  else assert(0);

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000010000") return false; // per_addrs is 0010
  string data_str;
  cout << " Sending Inputs for i " << i << endl;
  //pmem_request_file << " Sending Inputs for i " << i << endl;
  data_str = inputs[i];
  sendPerDout(data_str);
  sendIRQX();
  //sendDbgX();
  pmem_request_file << " In Case " << i << endl;
/*  switch (i)
  {
    case 0:
            //data_str = "0000000001001101";
            //data_str = "1000000000000000";
            data_str = "XXXXXXXX00000000";
            sendPerDout(data_str);
            pmem_request_file << "In Case 0 " << endl;
            break;
    case 1:
            //data_str = "0000000000000000";
            data_str = "XXXXXXXX00000000";
            sendPerDout(data_str);
            pmem_request_file << "In Case 1 " << endl;
            break;
    case 2:
            //data_str = "0010100000000000";
            data_str = "XXXXXXXX00000000";
            sendPerDout(data_str);
            pmem_request_file << "In Case 2 " << endl;
            break;
    case 3:
            //data_str = "0000000000000000";
            data_str = "XXXXXXXX00000000";
            sendPerDout(data_str);
            pmem_request_file << "In Case 3 " << endl;
            break;
    default : break;
  }*/
  i++;
  if (i == num_inputs) return true;
  return false;
}

void PowerOpt::recvInputs1(int cycle_num, bool wavefront)
{
	string per_enable_val  = m_pads[padNameIdMap.at("per_en")]->getSimValue();
  if (per_enable_val != "1") return ;

  string per_we_0_val, per_we_1_val;
  if (design == "flat_no_clk_gt")
  {
    per_we_1_val  = m_pads[padNameIdMap.at("per_we_1_")]->getSimValue();
    per_we_0_val  = m_pads[padNameIdMap.at("per_we_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_we_1_val  = m_pads[padNameIdMap.at("per_we[1]")]->getSimValue();
    per_we_0_val  = m_pads[padNameIdMap.at("per_we[0]")]->getSimValue();
  }
  else assert(0);

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return ;

  string per_addr_13_val , per_addr_12_val , per_addr_11_val , per_addr_10_val , per_addr_9_val  , per_addr_8_val  , per_addr_7_val  , per_addr_6_val  , per_addr_5_val  , per_addr_4_val  , per_addr_3_val  , per_addr_2_val  , per_addr_1_val  , per_addr_0_val ;
  if (design == "flat_no_clk_gt")
  {
    string per_addr_13_val = m_pads[padNameIdMap.at("per_addr_13_")]->getSimValue();
    string per_addr_12_val = m_pads[padNameIdMap.at("per_addr_12_")]->getSimValue();
    string per_addr_11_val = m_pads[padNameIdMap.at("per_addr_11_")]->getSimValue();
    string per_addr_10_val = m_pads[padNameIdMap.at("per_addr_10_")]->getSimValue();
    string per_addr_9_val  = m_pads[padNameIdMap.at("per_addr_9_")]->getSimValue();
    string per_addr_8_val  = m_pads[padNameIdMap.at("per_addr_8_")]->getSimValue();
    string per_addr_7_val  = m_pads[padNameIdMap.at("per_addr_7_")]->getSimValue();
    string per_addr_6_val  = m_pads[padNameIdMap.at("per_addr_6_")]->getSimValue();
    string per_addr_5_val  = m_pads[padNameIdMap.at("per_addr_5_")]->getSimValue();
    string per_addr_4_val  = m_pads[padNameIdMap.at("per_addr_4_")]->getSimValue();
    string per_addr_3_val  = m_pads[padNameIdMap.at("per_addr_3_")]->getSimValue();
    string per_addr_2_val  = m_pads[padNameIdMap.at("per_addr_2_")]->getSimValue();
    string per_addr_1_val  = m_pads[padNameIdMap.at("per_addr_1_")]->getSimValue();
    string per_addr_0_val  = m_pads[padNameIdMap.at("per_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    string per_addr_13_val = m_pads[padNameIdMap.at("per_addr[13]")]->getSimValue();
    string per_addr_12_val = m_pads[padNameIdMap.at("per_addr[12]")]->getSimValue();
    string per_addr_11_val = m_pads[padNameIdMap.at("per_addr[11]")]->getSimValue();
    string per_addr_10_val = m_pads[padNameIdMap.at("per_addr[10]")]->getSimValue();
    string per_addr_9_val  = m_pads[padNameIdMap.at("per_addr[9]")]->getSimValue();
    string per_addr_8_val  = m_pads[padNameIdMap.at("per_addr[8]")]->getSimValue();
    string per_addr_7_val  = m_pads[padNameIdMap.at("per_addr[7]")]->getSimValue();
    string per_addr_6_val  = m_pads[padNameIdMap.at("per_addr[6]")]->getSimValue();
    string per_addr_5_val  = m_pads[padNameIdMap.at("per_addr[5]")]->getSimValue();
    string per_addr_4_val  = m_pads[padNameIdMap.at("per_addr[4]")]->getSimValue();
    string per_addr_3_val  = m_pads[padNameIdMap.at("per_addr[3]")]->getSimValue();
    string per_addr_2_val  = m_pads[padNameIdMap.at("per_addr[2]")]->getSimValue();
    string per_addr_1_val  = m_pads[padNameIdMap.at("per_addr[1]")]->getSimValue();
    string per_addr_0_val  = m_pads[padNameIdMap.at("per_addr[0]")]->getSimValue();
  }
  else assert(0);

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000011000") return ; // per_addrs is 0010

  recv_inputs = true;
}

void PowerOpt::recvInputs2(int cycle_num, bool wavefront)
{
  if (!recv_inputs) return;
  string per_din_15_val , per_din_14_val , per_din_13_val , per_din_12_val , per_din_11_val , per_din_10_val , per_din_9_val  , per_din_8_val  , per_din_7_val  , per_din_6_val  , per_din_5_val  , per_din_4_val  , per_din_3_val  , per_din_2_val  , per_din_1_val  , per_din_0_val;
  if (design == "flat_no_clk_gt")
  {
    per_din_15_val = m_pads[padNameIdMap.at("per_din_15_")]->getSimValue();
    per_din_14_val = m_pads[padNameIdMap.at("per_din_14_")]->getSimValue();
    per_din_13_val = m_pads[padNameIdMap.at("per_din_13_")]->getSimValue();
    per_din_12_val = m_pads[padNameIdMap.at("per_din_12_")]->getSimValue();
    per_din_11_val = m_pads[padNameIdMap.at("per_din_11_")]->getSimValue();
    per_din_10_val = m_pads[padNameIdMap.at("per_din_10_")]->getSimValue();
    per_din_9_val  = m_pads[padNameIdMap.at("per_din_9_")]->getSimValue();
    per_din_8_val  = m_pads[padNameIdMap.at("per_din_8_")]->getSimValue();
    per_din_7_val  = m_pads[padNameIdMap.at("per_din_7_")]->getSimValue();
    per_din_6_val  = m_pads[padNameIdMap.at("per_din_6_")]->getSimValue();
    per_din_5_val  = m_pads[padNameIdMap.at("per_din_5_")]->getSimValue();
    per_din_4_val  = m_pads[padNameIdMap.at("per_din_4_")]->getSimValue();
    per_din_3_val  = m_pads[padNameIdMap.at("per_din_3_")]->getSimValue();
    per_din_2_val  = m_pads[padNameIdMap.at("per_din_2_")]->getSimValue();
    per_din_1_val  = m_pads[padNameIdMap.at("per_din_1_")]->getSimValue();
    per_din_0_val  = m_pads[padNameIdMap.at("per_din_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_din_15_val = m_pads[padNameIdMap.at("per_din[15]")]->getSimValue();
    per_din_14_val = m_pads[padNameIdMap.at("per_din[14]")]->getSimValue();
    per_din_13_val = m_pads[padNameIdMap.at("per_din[13]")]->getSimValue();
    per_din_12_val = m_pads[padNameIdMap.at("per_din[12]")]->getSimValue();
    per_din_11_val = m_pads[padNameIdMap.at("per_din[11]")]->getSimValue();
    per_din_10_val = m_pads[padNameIdMap.at("per_din[10]")]->getSimValue();
    per_din_9_val  = m_pads[padNameIdMap.at("per_din[9]")]->getSimValue();
    per_din_8_val  = m_pads[padNameIdMap.at("per_din[8]")]->getSimValue();
    per_din_7_val  = m_pads[padNameIdMap.at("per_din[7]")]->getSimValue();
    per_din_6_val  = m_pads[padNameIdMap.at("per_din[6]")]->getSimValue();
    per_din_5_val  = m_pads[padNameIdMap.at("per_din[5]")]->getSimValue();
    per_din_4_val  = m_pads[padNameIdMap.at("per_din[4]")]->getSimValue();
    per_din_3_val  = m_pads[padNameIdMap.at("per_din[3]")]->getSimValue();
    per_din_2_val  = m_pads[padNameIdMap.at("per_din[2]")]->getSimValue();
    per_din_1_val  = m_pads[padNameIdMap.at("per_din[1]")]->getSimValue();
    per_din_0_val  = m_pads[padNameIdMap.at("per_din[0]")]->getSimValue();
  }
  else assert(0);

  string per_din_str = per_din_15_val+ per_din_14_val+ per_din_13_val+ per_din_12_val+ per_din_11_val+ per_din_10_val+ per_din_9_val + per_din_8_val + per_din_7_val + per_din_6_val + per_din_5_val + per_din_4_val + per_din_3_val + per_din_2_val + per_din_1_val + per_din_0_val ;

  cout << " Output value is " << per_din_str <<  endl;
  recv_inputs = false;
}


void PowerOpt::checkCorruption(int i)
{


}



bool PowerOpt::handleBranches_ARM(int cycle_num)
{
  //string op_q = getRegVal("op_q", 0,  15);
  string op_q_upper_nibble = getRegVal("op_q", 12, 15);
  string op_q_upper_second_nibble = getRegVal("op_q", 8, 11);
  instr_name = "";

  if (op_q_upper_nibble == "1101") // conditional branch
  {
         if (op_q_upper_second_nibble == "0000") { instr_name = "BEQ" ; }
    else if (op_q_upper_second_nibble == "0001") { instr_name = "BNE" ; }
    else if (op_q_upper_second_nibble == "0010") { instr_name = "BCS" ; }
    else if (op_q_upper_second_nibble == "0011") { instr_name = "BCC" ; }
    else if (op_q_upper_second_nibble == "0100") { instr_name = "BMI" ; }
    else if (op_q_upper_second_nibble == "0101") { instr_name = "BPL" ; }
    else if (op_q_upper_second_nibble == "0110") { instr_name = "BVS" ; }
    else if (op_q_upper_second_nibble == "0111") { instr_name = "BVC" ; }
    else if (op_q_upper_second_nibble == "1000") { instr_name = "BHI" ; }
    else if (op_q_upper_second_nibble == "1001") { instr_name = "BLS" ; }
    else if (op_q_upper_second_nibble == "1010") { instr_name = "BGE" ; }
    else if (op_q_upper_second_nibble == "1011") { instr_name = "BLT" ; }
    else if (op_q_upper_second_nibble == "1100") { instr_name = "BGT" ; }
    else if (op_q_upper_second_nibble == "1101") { instr_name = "BLE" ; }
    pmem_request_file << " Instr is " << instr_name << " at cycle " << cycle_num << endl;

  }



}

bool PowerOpt::handleCondJumps(int cycle_num)
{
  bool can_skip = false;
  if(pmem_instr.compare(13,3,"100") == 0) // IS IT A JUMP?
  {
     string cond = pmem_instr.substr(10, 3);
     if (!cond.compare("000")) {      instr_name = "JNE/JNZ"     ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('Z'); }
     else if (!cond.compare("100")){  instr_name = "JEQ/JZ"  ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('Z'); }
     else if (!cond.compare("010")){  instr_name = "JNC/JLO" ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('C'); }
     else if (!cond.compare("110")){  instr_name = "JC/JHS"  ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('C'); }
     else if (!cond.compare("001")){  instr_name = "JN"      ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('N'); }
     else if (!cond.compare("101")){  instr_name = "JGE"     ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('V'); }
     else if (!cond.compare("011")){  instr_name = "JL"      ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('V'); }
     else if (!cond.compare("111")){  instr_name = "JMP"     ; cout << "Instr is " << instr_name << endl;  pmem_request_file << "Instr is " << instr_name << endl;  checkCorruption('E'); }
     else  instr_name = "";
     jump_detected = true;
     if (conservative_state == 1 && inp_ind_branches == 1)
     {
       system_state * sys_state = get_current_system_state(cycle_num);
       bool can_skip = get_conservative_state(sys_state);
       if (can_skip == false)
       {
         cout << "CONTINUING INP_IND SYSTEM STATE " << sys_state->get_state_short() << endl;
         pmem_request_file << "CONTINUING INP_IND SYSTEM STATE " << sys_state->get_state_short() << endl;
         //sys_state_queue.push(sys_state);
       }
       else
       {
         cout << "SKIPPING INP_IND SYSTEM STATE " << sys_state->get_state_short() << endl;
         pmem_request_file << "SKIPPING INP_IND SYSTEM STATE " << sys_state->get_state_short() << endl;
       }
     }
  }
  return can_skip;
}

void PowerOpt::compute_leakage_energy() // No caller function
{
  Instr* instr = PMemory[pmem_addr] ;
  assert(instr->executed == true);
  vector<bool> domain_activity = instr->domain_activity;
  for (int i = 0; i < domain_activity.size(); i++)
  {
    Cluster* cluster = clusters[i];
    float cluster_leakage_energy = cluster->getLeakageEnergy();
    if (domain_activity[i] == true)
    {
      total_leakage_energy += cluster_leakage_energy;
    }
    baseline_leakage_energy += cluster_leakage_energy;
  }
}

bool PowerOpt::readMem(int cycle_num, bool wavefront)
{
    string pmem_addr_str = getPmemAddr();
    if (design == "CORTEXM0PLUS")
    {
      string HTRANS_1 = m_pads[padNameIdMap.at("HTRANS[1]")]->getSimValue();
      if (HTRANS_1 == "1" && pmem_addr_str != "") // READ DATA NOW
      {
         pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);// binary to int
         instr = PMemory[pmem_addr]->instr;
         pmem_request_file << " PMEM ADDR IS " << pmem_addr << " INSTR IS " << instr << endl;
         //pmem_instr = binary(instr);
         pmem_instr = bitset<32> (instr).to_string();
         send_instr = true;
         return 0;
      }
      else return 0;
    }
    pmem_addr = strtoull(pmem_addr_str.c_str(), NULL, 2);// binary to int
    static string  last_addr_string;
    if (pmem_addr > 12287) instr = 0;
    //HERE2
    else if ( subnegFlag == true){
      instr = 2;
    }
    else instr = PMemory[pmem_addr]->instr;
    //if (pmem_addr == 12289) { pmem_addr = 3; instr = PMemory[3]->instr;}

    if ( subnegFlag == true)
    {
      if(subnegState ==0)
      {
        // sub instr
        //pmem_instr = "1000 src 1 0 01 dst"
        pmem_instr = "1000001010010010";
      }
      else if (subnegState == 1)
      {
        //NOP
        //pmem_instr = "0100 0011 0000 0011";
        pmem_instr = "XXXXXXXXXXXXXXXX";
      }
      else if (subnegState == 2)
      {
        //NOP
        //pmem_instr = "0100001100000011";
        pmem_instr = "XXXXXXXXXXXXXXXX";
      }
      else if (subnegState == 3)
      {
        //JN
        pmem_instr = "0011 00XXX XXX X010";
      }
    }
    else pmem_instr = binary(instr);
    bool can_skip = handleCondJumps(cycle_num);

    send_instr = true;
    string pc = getPC();
    int e_state = getEState();
    string e_state_str = binary(e_state);
    int i_state = getIState();
    string i_state_str = binary(i_state);
    int inst_type = getInstType();
    pmem_request_file << "At address " << pmem_addr << " ( " << pmem_addr_str << " ) and PC " << pc << " EState is " << e_state  << " IState is " << i_state <<  " Inst Type " << inst_type << " instruction is " << hex << instr << dec << " -> " << pmem_instr << " and cycle is " << cycle_num << endl;
    handleDmem(cycle_num);
    return can_skip;
}

void PowerOpt::sendIRQX()
{
  for (int i =0 ; i < 62; i ++)
  {
    stringstream ss;
    if (design == "flat_no_clk_gt") ss << "irq_" << i << "_";
    else if (design == "modified_9_hier") ss << "irq[" << i << "]";
    else assert(0);
    string port_name = ss.str();
    m_pads[padNameIdMap.at(port_name)] -> setSimValue("X", sim_wf);
  }
  m_pads[padNameIdMap.at("nmi")]->setSimValue("X", sim_wf);
}

void PowerOpt::sendDbgX()
{
  static int count = 0;
  if (count > 0) return;
  count++;
  if(design == "flat_no_clk_gt")
  {


  }
  else if (design == "modified_9_hier")
  {
//    m_pads[padNameIdMap.at("dbg_i2c_addr[6]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[5]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[4]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[3]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[2]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[1]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_addr[0]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[6]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[5]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[4]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[3]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[2]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[1]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_broadcast[0]")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_scl")] ->setSimValue("X",sim_wf);
//    m_pads[padNameIdMap.at("dbg_i2c_sda_in")] ->setSimValue("X",sim_wf);
    m_pads[padNameIdMap.at("dbg_uart_rxd")] ->setSimValue("X",sim_wf);
    m_pads[padNameIdMap.at("dbg_en")] ->setSimValue("1",sim_wf);

  }
  else assert(0);

}

void PowerOpt::sendPerDout(string data_str)
{
  if (design == "flat_no_clk_gt")
  {
    m_pads[padNameIdMap.at("per_dout_15_")] -> setSimValue(data_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_14_")] -> setSimValue(data_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_13_")] -> setSimValue(data_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_12_")] -> setSimValue(data_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_11_")] -> setSimValue(data_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_10_")] -> setSimValue(data_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_9_")]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_8_")]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_7_")]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_6_")]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_5_")]  -> setSimValue(data_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_4_")]  -> setSimValue(data_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_3_")]  -> setSimValue(data_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_2_")]  -> setSimValue(data_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_1_")]  -> setSimValue(data_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout_0_")]  -> setSimValue(data_str.substr(0 ,1), sim_wf);
  }
  else if (design == "modified_9_hier")
  {
    m_pads[padNameIdMap.at("per_dout[15]")] -> setSimValue(data_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[14]")] -> setSimValue(data_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[13]")] -> setSimValue(data_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[12]")] -> setSimValue(data_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[11]")] -> setSimValue(data_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[10]")] -> setSimValue(data_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[9]")]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[8]")]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[7]")]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[6]")]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[5]")]  -> setSimValue(data_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[4]")]  -> setSimValue(data_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[3]")]  -> setSimValue(data_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[2]")]  -> setSimValue(data_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[1]")]  -> setSimValue(data_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("per_dout[0]")]  -> setSimValue(data_str.substr(0 ,1), sim_wf);
  }
  else assert(0);
}

void PowerOpt::sendData (string data_str)
{
  dmem_request_file << " Reading Data " << data_str << endl;
  //cout << " Reading Data " << data_str << endl;
  // NOTE : inverting the assignments because to_string() of bitset assumes the same orientation of the bits as the constructor. to_string() does NOT invert the bit string for you.
  if (design == "flat_no_clk_gt")
  {
    m_pads[padNameIdMap.at("dmem_dout_15_")] -> setSimValue(data_str.substr(0 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_14_")] -> setSimValue(data_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_13_")] -> setSimValue(data_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_12_")] -> setSimValue(data_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_11_")] -> setSimValue(data_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_10_")] -> setSimValue(data_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_9_")]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_8_")]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_7_")]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_6_")]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_5_")]  -> setSimValue(data_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_4_")]  -> setSimValue(data_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_3_")]  -> setSimValue(data_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_2_")]  -> setSimValue(data_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_1_")]  -> setSimValue(data_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout_0_")]  -> setSimValue(data_str.substr(15,1), sim_wf);
  }
  else if (design == "modified_9_hier")
  {
    m_pads[padNameIdMap.at("dmem_dout[15]")] -> setSimValue(data_str.substr(0 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[14]")] -> setSimValue(data_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[13]")] -> setSimValue(data_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[12]")] -> setSimValue(data_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[11]")] -> setSimValue(data_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[10]")] -> setSimValue(data_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[9]")]  -> setSimValue(data_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[8]")]  -> setSimValue(data_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[7]")]  -> setSimValue(data_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[6]")]  -> setSimValue(data_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[5]")]  -> setSimValue(data_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[4]")]  -> setSimValue(data_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[3]")]  -> setSimValue(data_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[2]")]  -> setSimValue(data_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[1]")]  -> setSimValue(data_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("dmem_dout[0]")]  -> setSimValue(data_str.substr(15,1), sim_wf);
  }
  else if (design == "CORTEXM0PLUS")
  {
    std::reverse(data_str.begin(), data_str.end());
    m_pads[padNameIdMap.at("HRDATA[31]")] -> setSimValue(data_str.substr(31,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[30]")] -> setSimValue(data_str.substr(30,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[29]")] -> setSimValue(data_str.substr(29,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[28]")] -> setSimValue(data_str.substr(28,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[27]")] -> setSimValue(data_str.substr(27,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[26]")] -> setSimValue(data_str.substr(26,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[25]")] -> setSimValue(data_str.substr(25,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[24]")] -> setSimValue(data_str.substr(24,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[23]")] -> setSimValue(data_str.substr(23,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[22]")] -> setSimValue(data_str.substr(22,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[21]")] -> setSimValue(data_str.substr(21,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[20]")] -> setSimValue(data_str.substr(20,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[19]")] -> setSimValue(data_str.substr(19,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[18]")] -> setSimValue(data_str.substr(18,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[17]")] -> setSimValue(data_str.substr(17,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[16]")] -> setSimValue(data_str.substr(16,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[15]")] -> setSimValue(data_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[14]")] -> setSimValue(data_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[13]")] -> setSimValue(data_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[12]")] -> setSimValue(data_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[11]")] -> setSimValue(data_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[10]")] -> setSimValue(data_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[9]")]  -> setSimValue(data_str.substr(9,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[8]")]  -> setSimValue(data_str.substr(8,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[7]")]  -> setSimValue(data_str.substr(7,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[6]")]  -> setSimValue(data_str.substr(6,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[5]")]  -> setSimValue(data_str.substr(5,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[4]")]  -> setSimValue(data_str.substr(4,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[3]")]  -> setSimValue(data_str.substr(3,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[2]")]  -> setSimValue(data_str.substr(2,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[1]")]  -> setSimValue(data_str.substr(1,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[0]")]  -> setSimValue(data_str.substr(0,1), sim_wf);

  }
  else assert(0);
  send_data = false;
}

void PowerOpt::sendInstr(string instr_str)
{
  if (!send_instr) return;
  if (design == "flat_no_clk_gt")
  {
    m_pads[padNameIdMap.at("pmem_dout_15_")] -> setSimValue(instr_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_14_")] -> setSimValue(instr_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_13_")] -> setSimValue(instr_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_12_")] -> setSimValue(instr_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_11_")] -> setSimValue(instr_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_10_")] -> setSimValue(instr_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_9_")]  -> setSimValue(instr_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_8_")]  -> setSimValue(instr_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_7_")]  -> setSimValue(instr_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_6_")]  -> setSimValue(instr_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_5_")]  -> setSimValue(instr_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_4_")]  -> setSimValue(instr_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_3_")]  -> setSimValue(instr_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_2_")]  -> setSimValue(instr_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_1_")]  -> setSimValue(instr_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout_0_")]  -> setSimValue(instr_str.substr(0 ,1), sim_wf);
  }
  else if (design == "modified_9_hier")
  {
    m_pads[padNameIdMap.at("pmem_dout[15]")] -> setSimValue(instr_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[14]")] -> setSimValue(instr_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[13]")] -> setSimValue(instr_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[12]")] -> setSimValue(instr_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[11]")] -> setSimValue(instr_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[10]")] -> setSimValue(instr_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[9]")]  -> setSimValue(instr_str.substr(9 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[8]")]  -> setSimValue(instr_str.substr(8 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[7]")]  -> setSimValue(instr_str.substr(7 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[6]")]  -> setSimValue(instr_str.substr(6 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[5]")]  -> setSimValue(instr_str.substr(5 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[4]")]  -> setSimValue(instr_str.substr(4 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[3]")]  -> setSimValue(instr_str.substr(3 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[2]")]  -> setSimValue(instr_str.substr(2 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[1]")]  -> setSimValue(instr_str.substr(1 ,1), sim_wf);
    m_pads[padNameIdMap.at("pmem_dout[0]")]  -> setSimValue(instr_str.substr(0 ,1), sim_wf);
  }
  else if (design == "CORTEXM0PLUS")
  {
    std::reverse(instr_str.begin(), instr_str.end());
    m_pads[padNameIdMap.at("HRDATA[31]")] -> setSimValue(instr_str.substr(31,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[30]")] -> setSimValue(instr_str.substr(30,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[29]")] -> setSimValue(instr_str.substr(29,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[28]")] -> setSimValue(instr_str.substr(28,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[27]")] -> setSimValue(instr_str.substr(27,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[26]")] -> setSimValue(instr_str.substr(26,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[25]")] -> setSimValue(instr_str.substr(25,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[24]")] -> setSimValue(instr_str.substr(24,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[23]")] -> setSimValue(instr_str.substr(23,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[22]")] -> setSimValue(instr_str.substr(22,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[21]")] -> setSimValue(instr_str.substr(21,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[20]")] -> setSimValue(instr_str.substr(20,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[19]")] -> setSimValue(instr_str.substr(19,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[18]")] -> setSimValue(instr_str.substr(18,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[17]")] -> setSimValue(instr_str.substr(17,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[16]")] -> setSimValue(instr_str.substr(16,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[15]")] -> setSimValue(instr_str.substr(15,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[14]")] -> setSimValue(instr_str.substr(14,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[13]")] -> setSimValue(instr_str.substr(13,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[12]")] -> setSimValue(instr_str.substr(12,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[11]")] -> setSimValue(instr_str.substr(11,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[10]")] -> setSimValue(instr_str.substr(10,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[9]")]  -> setSimValue(instr_str.substr(9,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[8]")]  -> setSimValue(instr_str.substr(8,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[7]")]  -> setSimValue(instr_str.substr(7,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[6]")]  -> setSimValue(instr_str.substr(6,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[5]")]  -> setSimValue(instr_str.substr(5,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[4]")]  -> setSimValue(instr_str.substr(4,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[3]")]  -> setSimValue(instr_str.substr(3,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[2]")]  -> setSimValue(instr_str.substr(2,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[1]")]  -> setSimValue(instr_str.substr(1,1), sim_wf);
    m_pads[padNameIdMap.at("HRDATA[0]")]  -> setSimValue(instr_str.substr(0,1), sim_wf);

  }
  else assert(0);
  send_instr = false;
}

void PowerOpt::testReadAddr()
{
  if (design == "flat_no_clk_gt")
  {
    m_pads[padNameIdMap.at("pmem_addr_13_")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_12_")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_11_")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_10_")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_9_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_8_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_7_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_6_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_5_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_4_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_3_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_2_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_1_")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr_0_")]  -> setSimValue("0", sim_wf);
  }
  else if (design == "modified_9_hier")
  {
    m_pads[padNameIdMap.at("pmem_addr[13]")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[12]")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[11]")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[10]")] -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[9]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[8]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[7]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[6]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[5]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[4]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[3]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[2]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[1]")]  -> setSimValue("0", sim_wf);
    m_pads[padNameIdMap.at("pmem_addr[0]")]  -> setSimValue("0", sim_wf);
  }
  else assert(0);
}

void PowerOpt::initialRun()
{
  readConstantTerminals();
}

bool PowerOpt::check_peripherals()
{
  if (design == "flat_no_clk_gt" || design == "modified_9_hier")
  {
    string per_enable_val  = m_pads[padNameIdMap.at("per_en")]->getSimValue();
    if (per_enable_val != "1") return false;
  }
  else if (design == "CORTEXM0PLUS")
  {
    return true;

  }

  string per_we_0_val, per_we_1_val;
  if (design == "flat_no_clk_gt")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we_1_")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we[1]")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we[0]")]->getSimValue();
  }
  else if (design == "CORTEXM0PLUS")
  {
    cout << design << endl;
    return true;
  }
  else assert(0);

  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return false;

  string per_addr_13_val , per_addr_12_val , per_addr_11_val , per_addr_10_val , per_addr_9_val  , per_addr_8_val  , per_addr_7_val  , per_addr_6_val  , per_addr_5_val  , per_addr_4_val  , per_addr_3_val  , per_addr_2_val  , per_addr_1_val  , per_addr_0_val;
  if (design == "flat_no_clk_gt")
  {
     per_addr_13_val = m_pads[padNameIdMap.at("per_addr_13_")]->getSimValue();
     per_addr_12_val = m_pads[padNameIdMap.at("per_addr_12_")]->getSimValue();
     per_addr_11_val = m_pads[padNameIdMap.at("per_addr_11_")]->getSimValue();
     per_addr_10_val = m_pads[padNameIdMap.at("per_addr_10_")]->getSimValue();
     per_addr_9_val  = m_pads[padNameIdMap.at("per_addr_9_")]->getSimValue();
     per_addr_8_val  = m_pads[padNameIdMap.at("per_addr_8_")]->getSimValue();
     per_addr_7_val  = m_pads[padNameIdMap.at("per_addr_7_")]->getSimValue();
     per_addr_6_val  = m_pads[padNameIdMap.at("per_addr_6_")]->getSimValue();
     per_addr_5_val  = m_pads[padNameIdMap.at("per_addr_5_")]->getSimValue();
     per_addr_4_val  = m_pads[padNameIdMap.at("per_addr_4_")]->getSimValue();
     per_addr_3_val  = m_pads[padNameIdMap.at("per_addr_3_")]->getSimValue();
     per_addr_2_val  = m_pads[padNameIdMap.at("per_addr_2_")]->getSimValue();
     per_addr_1_val  = m_pads[padNameIdMap.at("per_addr_1_")]->getSimValue();
     per_addr_0_val  = m_pads[padNameIdMap.at("per_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_addr_13_val = m_pads[padNameIdMap.at("per_addr[13]")]->getSimValue();
    per_addr_12_val = m_pads[padNameIdMap.at("per_addr[12]")]->getSimValue();
    per_addr_11_val = m_pads[padNameIdMap.at("per_addr[11]")]->getSimValue();
    per_addr_10_val = m_pads[padNameIdMap.at("per_addr[10]")]->getSimValue();
    per_addr_9_val  = m_pads[padNameIdMap.at("per_addr[9]")]->getSimValue();
    per_addr_8_val  = m_pads[padNameIdMap.at("per_addr[8]")]->getSimValue();
    per_addr_7_val  = m_pads[padNameIdMap.at("per_addr[7]")]->getSimValue();
    per_addr_6_val  = m_pads[padNameIdMap.at("per_addr[6]")]->getSimValue();
    per_addr_5_val  = m_pads[padNameIdMap.at("per_addr[5]")]->getSimValue();
    per_addr_4_val  = m_pads[padNameIdMap.at("per_addr[4]")]->getSimValue();
    per_addr_3_val  = m_pads[padNameIdMap.at("per_addr[3]")]->getSimValue();
    per_addr_2_val  = m_pads[padNameIdMap.at("per_addr[2]")]->getSimValue();
    per_addr_1_val  = m_pads[padNameIdMap.at("per_addr[1]")]->getSimValue();
    per_addr_0_val  = m_pads[padNameIdMap.at("per_addr[0]")]->getSimValue();
  }
  else assert(0);


  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000001100") return false; // per_addrs is 000c
  //if (per_addr_str != "00000000010100") return false; // per_addrs is 000c
  return true;
}

bool PowerOpt::check_sim_end(int& i, bool wavefront)
{
  if (design == "PLASTICARM") return false;
  if (design == "CORTEXM0PLUS")
  {
    static bool read_test_pass = false;
    static bool read_test_complete = false;
    static bool test_pass = false;
    static bool test_complete = false;
    string HADDR = getPortVal("HADDR", 0, 31, '[');
    string HWRITE = m_pads[padNameIdMap.at("HWRITE")]->getSimValue();
    if (read_test_pass == true)
    {
      string HWDATA = getPortVal("HWDATA", 0, 31, '[');
      if (HWDATA == "00000010000000100000001000000010")
      {
        test_pass = true;
      }
      read_test_pass = false;
    }
    else if (read_test_complete == true)
    {
      string HWDATA = getPortVal("HWDATA", 0, 31, '[');
      if (HWDATA == "00000001000000010000000100000001")
      {
        test_complete = true;
        if (test_pass == true)
        {
          cout << "SIM SUCCESS" << endl;
        }
        return true;
      }
      read_test_complete = false;
    }
    if (HADDR == "01000000000000000000000000001000" && HWRITE == "1")
    {
      read_test_pass = true;
    }
    else if (HADDR == "01000000000000000000000000000100" && HWRITE == "1")
    {
      read_test_complete = true;
    }
    return false;
  }
	string per_enable_val  = m_pads[padNameIdMap.at("per_en")]->getSimValue();
  if (per_enable_val != "1") return false;

  string per_we_0_val, per_we_1_val;
  if (design == "flat_no_clk_gt")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we_1_")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
     per_we_1_val  = m_pads[padNameIdMap.at("per_we[1]")]->getSimValue();
     per_we_0_val  = m_pads[padNameIdMap.at("per_we[0]")]->getSimValue();
  }
  else assert(0);
  string per_we_str = per_we_1_val + per_we_0_val;
  if (per_we_str != "10") return false;

  string per_addr_13_val , per_addr_12_val , per_addr_11_val , per_addr_10_val , per_addr_9_val  , per_addr_8_val  , per_addr_7_val  , per_addr_6_val  , per_addr_5_val  , per_addr_4_val  , per_addr_3_val  , per_addr_2_val  , per_addr_1_val  , per_addr_0_val;
  if (design == "flat_no_clk_gt")
  {
    per_addr_13_val = m_pads[padNameIdMap.at("per_addr_13_")]->getSimValue();
    per_addr_12_val = m_pads[padNameIdMap.at("per_addr_12_")]->getSimValue();
    per_addr_11_val = m_pads[padNameIdMap.at("per_addr_11_")]->getSimValue();
    per_addr_10_val = m_pads[padNameIdMap.at("per_addr_10_")]->getSimValue();
    per_addr_9_val  = m_pads[padNameIdMap.at("per_addr_9_")]->getSimValue();
    per_addr_8_val  = m_pads[padNameIdMap.at("per_addr_8_")]->getSimValue();
    per_addr_7_val  = m_pads[padNameIdMap.at("per_addr_7_")]->getSimValue();
    per_addr_6_val  = m_pads[padNameIdMap.at("per_addr_6_")]->getSimValue();
    per_addr_5_val  = m_pads[padNameIdMap.at("per_addr_5_")]->getSimValue();
    per_addr_4_val  = m_pads[padNameIdMap.at("per_addr_4_")]->getSimValue();
    per_addr_3_val  = m_pads[padNameIdMap.at("per_addr_3_")]->getSimValue();
    per_addr_2_val  = m_pads[padNameIdMap.at("per_addr_2_")]->getSimValue();
    per_addr_1_val  = m_pads[padNameIdMap.at("per_addr_1_")]->getSimValue();
    per_addr_0_val  = m_pads[padNameIdMap.at("per_addr_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_addr_13_val = m_pads[padNameIdMap.at("per_addr[13]")]->getSimValue();
    per_addr_12_val = m_pads[padNameIdMap.at("per_addr[12]")]->getSimValue();
    per_addr_11_val = m_pads[padNameIdMap.at("per_addr[11]")]->getSimValue();
    per_addr_10_val = m_pads[padNameIdMap.at("per_addr[10]")]->getSimValue();
    per_addr_9_val  = m_pads[padNameIdMap.at("per_addr[9]")]->getSimValue();
    per_addr_8_val  = m_pads[padNameIdMap.at("per_addr[8]")]->getSimValue();
    per_addr_7_val  = m_pads[padNameIdMap.at("per_addr[7]")]->getSimValue();
    per_addr_6_val  = m_pads[padNameIdMap.at("per_addr[6]")]->getSimValue();
    per_addr_5_val  = m_pads[padNameIdMap.at("per_addr[5]")]->getSimValue();
    per_addr_4_val  = m_pads[padNameIdMap.at("per_addr[4]")]->getSimValue();
    per_addr_3_val  = m_pads[padNameIdMap.at("per_addr[3]")]->getSimValue();
    per_addr_2_val  = m_pads[padNameIdMap.at("per_addr[2]")]->getSimValue();
    per_addr_1_val  = m_pads[padNameIdMap.at("per_addr[1]")]->getSimValue();
    per_addr_0_val  = m_pads[padNameIdMap.at("per_addr[0]")]->getSimValue();
  }
  else assert(0);

  string per_addr_str = per_addr_13_val+ per_addr_12_val+ per_addr_11_val+ per_addr_10_val+ per_addr_9_val + per_addr_8_val + per_addr_7_val + per_addr_6_val + per_addr_5_val + per_addr_4_val + per_addr_3_val + per_addr_2_val + per_addr_1_val + per_addr_0_val ;
  if (per_addr_str != "00000000001100") return false;
  //if (per_addr_str != "00000000010100") return false;

  string per_din_0_val;
  if (design == "flat_no_clk_gt")
     per_din_0_val = m_pads[padNameIdMap.at("per_din_0_")]->getSimValue();
  else if (design == "modified_9_hier")
    per_din_0_val = m_pads[padNameIdMap.at("per_din[0]")]->getSimValue();
  else assert(0);
  //ToggleType per_din_0_tog_type = m_pads[padNameIdMap["per_din\\[0\\]"]]->getSimToggType(); // checking for FALL might be hard when starting simulation from a saved state.
  if (per_din_0_val != "0") return false;
  return true;
}

void PowerOpt::debug_per_din(int cycle_num)
{

  string per_din_15_val , per_din_14_val , per_din_13_val , per_din_12_val , per_din_11_val , per_din_10_val , per_din_9_val  , per_din_8_val  , per_din_7_val  , per_din_6_val  , per_din_5_val  , per_din_4_val  , per_din_3_val  , per_din_2_val  , per_din_1_val  , per_din_0_val;
  if (design == "flat_no_clk_gt")
  {
    per_din_15_val = m_pads[padNameIdMap.at("per_din_15_")]->getSimValue();
    per_din_14_val = m_pads[padNameIdMap.at("per_din_14_")]->getSimValue();
    per_din_13_val = m_pads[padNameIdMap.at("per_din_13_")]->getSimValue();
    per_din_12_val = m_pads[padNameIdMap.at("per_din_12_")]->getSimValue();
    per_din_11_val = m_pads[padNameIdMap.at("per_din_11_")]->getSimValue();
    per_din_10_val = m_pads[padNameIdMap.at("per_din_10_")]->getSimValue();
    per_din_9_val  = m_pads[padNameIdMap.at("per_din_9_")]->getSimValue();
    per_din_8_val  = m_pads[padNameIdMap.at("per_din_8_")]->getSimValue();
    per_din_7_val  = m_pads[padNameIdMap.at("per_din_7_")]->getSimValue();
    per_din_6_val  = m_pads[padNameIdMap.at("per_din_6_")]->getSimValue();
    per_din_5_val  = m_pads[padNameIdMap.at("per_din_5_")]->getSimValue();
    per_din_4_val  = m_pads[padNameIdMap.at("per_din_4_")]->getSimValue();
    per_din_3_val  = m_pads[padNameIdMap.at("per_din_3_")]->getSimValue();
    per_din_2_val  = m_pads[padNameIdMap.at("per_din_2_")]->getSimValue();
    per_din_1_val  = m_pads[padNameIdMap.at("per_din_1_")]->getSimValue();
    per_din_0_val  = m_pads[padNameIdMap.at("per_din_0_")]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    per_din_15_val = m_pads[padNameIdMap.at("per_din[15]")]->getSimValue();
    per_din_14_val = m_pads[padNameIdMap.at("per_din[14]")]->getSimValue();
    per_din_13_val = m_pads[padNameIdMap.at("per_din[13]")]->getSimValue();
    per_din_12_val = m_pads[padNameIdMap.at("per_din[12]")]->getSimValue();
    per_din_11_val = m_pads[padNameIdMap.at("per_din[11]")]->getSimValue();
    per_din_10_val = m_pads[padNameIdMap.at("per_din[10]")]->getSimValue();
    per_din_9_val  = m_pads[padNameIdMap.at("per_din[9]")]->getSimValue();
    per_din_8_val  = m_pads[padNameIdMap.at("per_din[8]")]->getSimValue();
    per_din_7_val  = m_pads[padNameIdMap.at("per_din[7]")]->getSimValue();
    per_din_6_val  = m_pads[padNameIdMap.at("per_din[6]")]->getSimValue();
    per_din_5_val  = m_pads[padNameIdMap.at("per_din[5]")]->getSimValue();
    per_din_4_val  = m_pads[padNameIdMap.at("per_din[4]")]->getSimValue();
    per_din_3_val  = m_pads[padNameIdMap.at("per_din[3]")]->getSimValue();
    per_din_2_val  = m_pads[padNameIdMap.at("per_din[2]")]->getSimValue();
    per_din_1_val  = m_pads[padNameIdMap.at("per_din[1]")]->getSimValue();
    per_din_0_val  = m_pads[padNameIdMap.at("per_din[0]")]->getSimValue();
  }
  else assert(0);

  string per_din_str = per_din_15_val+ per_din_14_val+ per_din_13_val+ per_din_12_val+ per_din_11_val+ per_din_10_val+ per_din_9_val + per_din_8_val + per_din_7_val + per_din_6_val + per_din_5_val + per_din_4_val + per_din_3_val + per_din_2_val + per_din_1_val + per_din_0_val ;

//  bitset<16> temp(per_din_str);
//  unsigned n = temp.to_ulong();
//  cout << " per_din is " << hex << n << dec << " at cycle " << cycle_num << endl;
}

bool PowerOpt::readHandShake()
{



}

void PowerOpt::checkConnectivity(designTiming* T)
{
/*  for (int i =0 ; i < nets.size(); i++)
  {
    Net * net = getNet(i);
    Terminal* inp_term = net->getInputTerminal();
    if (inp_term != 0)
    {
      Net* inp_term_net = inp_term->getNet(0);
      debug_file << net->getName() << " : " << inp_term->getFullName() << " : " << inp_term_net->getName() << endl;
      if (inp_term_net != net) assert(0);
    }
    else
    {
      debug_file << net->getName() << " : " << endl;
    }

  }*/
  for (int i = 0; i < m_pads.size(); i++)
  {
    Pad* pad = m_pads[i];
    if (pad->getType() == PrimiaryOutput) continue;
    if(pad->getNetNum() == 0) // some input pads are just floating
    {
      continue;
    }
    assert(pad->getNetNum() == 1);
    Net* net = pad->getNet(0);
    assert(net->getPadNum() == 1); // not necessarily true I guess
    Pad* net_pad = net->getPad(0);
    assert(net_pad == pad);
    //debug_file << pad->getName() << endl;

  }
  for (int i =0 ; i < nets.size(); i++)
  {
    Net * net = getNet(i);
    int terms_size = net->getTerminalNum();
    vector<string> terms_from_PowerOpt, ports_from_PowerOpt;
    bool is_constant = net->getIsConstant();
    if (is_constant == true) continue;
    // check that the term connected to the net is also connected back to the net
    for (int j =0; j < terms_size; j++)
    {
      Terminal* term = net->getTerminal(j);
      Net* term_net = term->getNet(0);
      terms_from_PowerOpt.push_back(term->getFullName());

      if (term_net != net) assert(0);
    }

    int ports_size = net->getPadNum();
    for (int j = 0; j < ports_size; j++)
    {
      Pad* pad = net->getPad(j);
      Net* pad_net = pad->getNet(0);
      ports_from_PowerOpt.push_back(pad->getName());

      assert(pad_net == net);
    }

    // check that the terms connected to the net are same as those in pt
    string net_name = net->getName();
    if (design == "modified_9_hier")
      replace_substr(net_name, "[", "\\[" ); // FOR PT SOCKET
    else if (design == "flat_no_clk_gt")
    {
      replace_substr(net_name, "\[", "_" ); // FOR COMPARISON
      replace_substr(net_name, "\]", "_" ); // FOR COMPARISON
    }
    string terms_from_pt_str = T->getTermsFromNet(net_name);
    //cout <<  " Net is " << net->getName() << " Terms_from_pt is " << terms_from_pt_str ;
    vector<string> terms_from_pt;
    tokenize(terms_from_pt_str, ' ', terms_from_pt);
    sort(terms_from_pt.begin(), terms_from_pt.end());
    sort(terms_from_PowerOpt.begin(), terms_from_PowerOpt.end());
    assert(terms_from_pt.size() == terms_from_PowerOpt.size());
    for(int j = 0 ; j < terms_from_pt.size(); j++)
    {
      assert(terms_from_pt[j] == terms_from_PowerOpt[j]);
    }

    // check that the ports connected to the net are same as those in pt
    string ports_from_pt_str = T->getPortsFromNet(net_name);
    vector<string> ports_from_pt;
    tokenize(ports_from_pt_str, ' ', ports_from_pt);
    sort(ports_from_pt.begin(), ports_from_pt.end());
    sort(ports_from_PowerOpt.begin(), ports_from_PowerOpt.end());
    assert(ports_from_pt.size() == ports_from_PowerOpt.size());
    //cout <<  "  Net is " << net->getName() << endl;
    for(int j = 0 ; j < ports_from_pt.size(); j++)
    {
      string port_from_PwrOpt = ports_from_PowerOpt[j];
      if (design == "modified_9_hier")
      {
        replace_substr(port_from_PwrOpt, "\\", "" ); // FOR COMPARISON
      }
      else if (design == "flat_no_clk_gt")
      {
        replace_substr(port_from_PwrOpt, "\\[", "_" ); // FOR COMPARISON
        replace_substr(port_from_PwrOpt, "\\]", "_" ); // FOR COMPARISON
      }
      assert(ports_from_pt[j] == port_from_PwrOpt);
    }

  }
  cout << " ALL FINE"  << endl;
}

void PowerOpt::getSimValOfTerminal(string term_name, string& val)
{
  map<string, int>::iterator it = terminalNameIdMap.find(term_name);
  if (it != terminalNameIdMap.end()){ val = terms[it->second]->getSimValue();}
}

string PowerOpt::getPC()
{
  string pc_15_val="0";
  string pc_14_val="0";
  string pc_13_val="0";
  string pc_12_val="0";
  string pc_11_val="0";
  string pc_10_val="0";
  string pc_9_val ="0";
  string pc_8_val ="0";
  string pc_7_val ="0";
  string pc_6_val ="0";
  string pc_5_val ="0";
  string pc_4_val ="0";
  string pc_3_val ="0";
  string pc_2_val ="0";
  string pc_1_val ="0";
  string pc_0_val ="0";
  if (design == "flat_no_clk_gt")
  {
    getSimValOfTerminal("frontend_0_pc_reg_15_/Q", pc_15_val );
    getSimValOfTerminal("frontend_0_pc_reg_14_/Q", pc_14_val );
    getSimValOfTerminal("frontend_0_pc_reg_13_/Q", pc_13_val );
    getSimValOfTerminal("frontend_0_pc_reg_12_/Q", pc_12_val );
    getSimValOfTerminal("frontend_0_pc_reg_11_/Q", pc_11_val );
    getSimValOfTerminal("frontend_0_pc_reg_10_/Q", pc_10_val );
    getSimValOfTerminal("frontend_0_pc_reg_9_/Q" , pc_9_val  );
    getSimValOfTerminal("frontend_0_pc_reg_8_/Q" , pc_8_val  );
    getSimValOfTerminal("frontend_0_pc_reg_7_/Q" , pc_7_val  );
    getSimValOfTerminal("frontend_0_pc_reg_6_/Q" , pc_6_val  );
    getSimValOfTerminal("frontend_0_pc_reg_5_/Q" , pc_5_val  );
    getSimValOfTerminal("frontend_0_pc_reg_4_/Q" , pc_4_val  );
    getSimValOfTerminal("frontend_0_pc_reg_3_/Q" , pc_3_val  );
    getSimValOfTerminal("frontend_0_pc_reg_2_/Q" , pc_2_val  );
    getSimValOfTerminal("frontend_0_pc_reg_1_/Q" , pc_1_val  );
    getSimValOfTerminal("frontend_0_pc_reg_0_/Q" , pc_0_val  );
  }
  else if (design == "modified_9_hier")
  {
    getSimValOfTerminal("frontend_0/pc_reg_15_/Q", pc_15_val );
    getSimValOfTerminal("frontend_0/pc_reg_14_/Q", pc_14_val );
    getSimValOfTerminal("frontend_0/pc_reg_13_/Q", pc_13_val );
    getSimValOfTerminal("frontend_0/pc_reg_12_/Q", pc_12_val );
    getSimValOfTerminal("frontend_0/pc_reg_11_/Q", pc_11_val );
    getSimValOfTerminal("frontend_0/pc_reg_10_/Q", pc_10_val );
    getSimValOfTerminal("frontend_0/pc_reg_9_/Q" , pc_9_val  );
    getSimValOfTerminal("frontend_0/pc_reg_8_/Q" , pc_8_val  );
    getSimValOfTerminal("frontend_0/pc_reg_7_/Q" , pc_7_val  );
    getSimValOfTerminal("frontend_0/pc_reg_6_/Q" , pc_6_val  );
    getSimValOfTerminal("frontend_0/pc_reg_5_/Q" , pc_5_val  );
    getSimValOfTerminal("frontend_0/pc_reg_4_/Q" , pc_4_val  );
    getSimValOfTerminal("frontend_0/pc_reg_3_/Q" , pc_3_val  );
    getSimValOfTerminal("frontend_0/pc_reg_2_/Q" , pc_2_val  );
    getSimValOfTerminal("frontend_0/pc_reg_1_/Q" , pc_1_val  );
    getSimValOfTerminal("frontend_0/pc_reg_0_/Q" , pc_0_val  );
//    pc_15_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_15_/Q")]->getSimValue();
//    pc_14_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_14_/Q")]->getSimValue();
//    pc_13_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_13_/Q")]->getSimValue();
//    pc_12_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_12_/Q")]->getSimValue();
//    pc_11_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_11_/Q")]->getSimValue();
//    pc_10_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_10_/Q")]->getSimValue();
//    pc_9_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_9_/Q")]->getSimValue();
//    pc_8_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_8_/Q")]->getSimValue();
//    pc_7_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_7_/Q")]->getSimValue();
//    pc_6_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_6_/Q")]->getSimValue();
//    pc_5_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_5_/Q")]->getSimValue();
//    pc_4_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_4_/Q")]->getSimValue();
//    pc_3_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_3_/Q")]->getSimValue();
//    pc_2_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_2_/Q")]->getSimValue();
//    pc_1_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_1_/Q")]->getSimValue();
//    pc_0_val = terms[terminalNameIdMap.at("frontend_0/pc_reg_0_/Q")]->getSimValue();
  }

  string pc_str = pc_15_val + pc_14_val + pc_13_val+ pc_12_val+ pc_11_val+ pc_10_val+ pc_9_val + pc_8_val + pc_7_val + pc_6_val + pc_5_val + pc_4_val + pc_3_val + pc_2_val + pc_1_val + pc_0_val ;
  return pc_str;
}

bool PowerOpt::checkIfHung()
{
  if (design != "CORTEXM0PLUS") return false;
  if (instr == hung_check_last_instr)
    hung_check_count ++;
  else
  {
    hung_check_count = 0;
    hung_check_last_instr = instr;
  }

  if (hung_check_count > 20) {
    cout << " System hung" << endl;
    pmem_request_file << " System hung" << endl;
    return true;
  }
  return false;
}

void PowerOpt::SaveAllNetVals()
{
  for (int i = 0; i < getNetNum(); i++)
  {
    Net* net = getNet(i);
    string sim_val = net->getSimValue();
    bool toggled_state = false;
    net_val_toggle_info.insert(make_pair(net, make_pair(sim_val, toggled_state)));
  }
}

void PowerOpt::tbstuff(int cycle_num)
{
  if (design == "PLASTICARM")
  {
    if (cycle_num == 4)
    {
      SaveAllNetVals();
    }
    if (cycle_num == 8)
    {
      m_pads[padNameIdMap.at("I_NPOR")] -> setSimValue("1", sim_wf);
    }
  }
  if (design == "CORTEXM0PLUS") 
  {
    if (cycle_num == 2)
    {
      m_pads[padNameIdMap.at("HRESETn")] -> setSimValue("0", sim_wf);
    }
    if (cycle_num == 4)
    {
      SaveAllNetVals();
    }
    if (cycle_num == 8)
    {
      m_pads[padNameIdMap.at("HRESETn")] -> setSimValue("1", sim_wf);
    }
    if (cycle_num == 9)
    {
      m_pads[padNameIdMap.at("WICDSREQn")] -> setSimValue("0", sim_wf);
    }
  }
}

void PowerOpt::register_net_toggle(Net* net)
{
  if (st_cycle_num < 4) return;
  map<Net*, pair<string, bool> >:: iterator it = net_val_toggle_info.find(net);
  assert(it != net_val_toggle_info.end());
  net_val_toggle_info[net].second = true;
}

void PowerOpt::simulate()
{
   // Read file for initial values of reg-Q pins and input ports
  readSimInitFile();
  readDmemInitFile();
  readInputValueFile();
  pmem_instr = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
  input_count = 0;

  bool wavefront = true;
  bool done_inputs = false;
  bool read_output = false;
  bool dmem_write = false;
  bool brk = false;
  hung_check_count = 0;
  hung_check_last_instr = 0;
  jump_cycle = 0;
  print_processor_state_profile(0,false, true);
  cycle_toggled_indices.clear();
  while(true)
  {
    st_cycle_num++;
    tbstuff(st_cycle_num);
    //sendInputs_new();
    writeDmem(st_cycle_num);
    readOutputs();
    bool ret_val = handleHaddr(st_cycle_num);
    brk |= ret_val;
    brk |= checkIfHung();
    brk |= (st_cycle_num > num_sim_cycles);
    if (brk == true)
    {
      cout << " BREAKING " << endl;
      break;
    }

    if (design == "PLASTICARM")
    {
      string uRSD_CORE_rom_addr = getNetVal("uRSD_CORE_rom_addr", 0, 9, '[');
      pmem_request_file << "ROM addr is " << uRSD_CORE_rom_addr << " at cycle " << st_cycle_num << endl;
    }


    runSimulation(wavefront, st_cycle_num, false); // simulates the negative edge

    handleHaddrBools(st_cycle_num);

    if (print_processor_state_profile_every_cycle == 1) {
      print_processor_state_profile(st_cycle_num, false, false);
//      string HRDATA = getPortVal ("HRDATA", 0, 31, '[');
//      debug_file << " pmem_instr is " << pmem_instr << endl;
//      debug_file << " HRDATA is " << HRDATA << endl;
//      debug_file_second << " pmem_instr is " << pmem_instr << endl;
//      debug_file_second << " HRDATA is " << HRDATA << endl;
    }
    else if (print_processor_state_profile_every_cycle == 2) {
      print_processor_state_profile(st_cycle_num, true, false);
    }

    //updateFromMem();
    //recvInputs2(i, wavefront);

    runSimulation(wavefront, st_cycle_num, true); // simulates  the positive edge
    //writeVCDCycle(2*i+1); //  Since writeVCDCycle actually writes half a cycle ! Go through the function to see why;
    if (print_processor_state_profile_every_cycle == 1) {
      print_processor_state_profile(st_cycle_num, false, true);
//      string HRDATA = getPortVal ("HRDATA", 0, 31, '[');
//      debug_file << " pmem_instr is " << pmem_instr << endl;
//      debug_file << " HRDATA is " << HRDATA << endl;
//      debug_file_second << " pmem_instr is " << pmem_instr << endl;
//      debug_file_second << " HRDATA is " << HRDATA << endl;
    }
    else if (print_processor_state_profile_every_cycle == 2) {
      print_processor_state_profile(st_cycle_num, true, true);
    }
    updateRegOutputs(st_cycle_num); // print proc state is before flops get updated to match with VCS

    if (probeRegisters_ARM(st_cycle_num) == true && st_cycle_num > 5 )
    {
      cout << " ENDING SIMULATION DUE TO STATE CORRUPTION " << endl;
      break;
    }
    if (check_sim_end(st_cycle_num, wavefront) == true ) {
      cout << "ENDING SIMULATION" << endl;
      break ;
    }

    //debug_file_second << " R10 is " << getGPR(10)  << " R14 is " << getGPR(14) << " at cycle " << i << endl;
    //Gate* my_gate = m_gates[gateNameIdMap.at("u_top/u_sys/u_core/U92")];
  }
  global_curr_cycle = st_cycle_num;
  vcd_file.close();
}

void PowerOpt::simulate3()
{
  bool wavefront = true;
  bool done_inputs = false;
  while(!sys_state_queue.empty())
  {
    system_state* sys_state = sys_state_queue.front(); sys_state_queue.pop();

    if (conservative_state == 1)
    {
      string HADDR_str = sys_state->HADDR;
      int local_id = sys_state->local_id;
      map< string, map<int , system_state* > >::iterator hbcit = HADDR_branch_conservative_state.find(HADDR_str);
      if (hbcit == HADDR_branch_conservative_state.end())
      {
        map<int, system_state*> new_map;
        new_map.insert(make_pair(local_id, sys_state));
        HADDR_branch_conservative_state.insert(make_pair(HADDR_str, new_map));
        cout << " NEW HADDR " << HADDR_str << endl;
        pmem_request_file << " NEW HADDR " << HADDR_str << endl;
      }
      else
      {
        map<int, system_state*> & branch_state_map = hbcit->second;
        map<int, system_state*>::iterator bcit = branch_state_map.find(local_id);
        if (bcit == branch_state_map.end())
        {
          branch_state_map.insert(make_pair(local_id, sys_state));
        }
        else
        {
          system_state* stored_state = bcit->second;
          system_state & this_state = *sys_state;
          bool can_skip = stored_state->compare_and_update_state_ARM(this_state);
          if (can_skip == true)
          {
            cout << "SKIPPING !" << sys_state->get_state_short() << endl;
            pmem_request_file << "SKIPPING !" << sys_state->get_state_short() << endl;
            continue;
          }
          cout << " OLD HADDR : " << HADDR_str << " NOT SKIPPING" << sys_state->get_state_short() << endl;
          pmem_request_file << " OLD HADDR : " << HADDR_str << " NOT SKIPPING" << sys_state->get_state_short() << endl;
          // need to have a report on conservative state generation here
          sys_state = stored_state;
        }
      }
    }
    else
    {
      cout << " CONSERVATIVE STATE NOT MAINTAINTED " << endl;
    }

    map<int, string>::iterator it;
    for (it = sys_state->net_sim_value_map.begin(); it != sys_state->net_sim_value_map.end(); it++)
    {
       int net_id = it->first; string sim_val = it->second;
       nets[net_id]->setSimValue(sim_val);
    }

    sim_wf = sys_state->sim_wf;
    DMemory = sys_state->DMemory;
    int start_cycle = sys_state->cycle_num;
    input_count = sys_state->input_count;

    cout << "RESUMING " << sys_state->get_state_short() <<  endl;
    pmem_request_file << "------------------------------ RE-RUNNING FROM SAVED POINT ----------------" << endl;
    hung_check_count = 0;
    hung_check_last_instr = 0;

    bool brk = false;
    for (int st_cycle_num = start_cycle; st_cycle_num < num_sim_cycles; st_cycle_num++)
    {
      writeDmem(st_cycle_num);
      readOutputs();
      brk |= handleHaddr(st_cycle_num);
      brk |= checkIfHung();
      brk |= (st_cycle_num> num_sim_cycles);
      if (brk == true) break;
      runSimulation(wavefront, st_cycle_num, false); // simulates the negative edge
      handleHaddrBools(st_cycle_num);
      runSimulation(wavefront, st_cycle_num, true); // simulates  the positive edge
      updateRegOutputs(st_cycle_num);
      if (print_processor_state_profile_every_cycle == 1) {
        print_processor_state_profile(st_cycle_num, false, true);
        string HRDATA = getPortVal ("HRDATA", 0, 31, '[');
        debug_file << " pmem_instr is " << pmem_instr << endl;
        debug_file << " HRDATA is " << HRDATA << endl;
        debug_file_second << " pmem_instr is " << pmem_instr << endl;
        debug_file_second << " HRDATA is " << HRDATA << endl;
      }
      else if (print_processor_state_profile_every_cycle == 2) {
        print_processor_state_profile(st_cycle_num, true, true);
      }
      if (probeRegisters_ARM(st_cycle_num) == true && st_cycle_num > 1 ) {
        cout << " ENDING SIMULATION DUE TO STATE CORRUPTION " << endl;
        break;
      }
      if (check_sim_end(st_cycle_num, wavefront) == true ) {
        cout << "ENDING SIMULATION" << endl;
        break ;
      }

    }

  }

}


void PowerOpt::simulate2()
{
   // Read file for initial values of reg-Q pins and input ports
//  readSimInitFile();
//  readDmemInitFile();
  bool wavefront = true;
  bool done_inputs = false;
  jump_cycle = 0;
  cout << " simulate3() " << endl;
  while(!sys_state_queue.empty())
  {
    system_state* sys_state = sys_state_queue.front(); sys_state_queue.pop();
    string PC = sys_state->PC;
    if (hasX(PC)) continue;
    string PC_hex = bin2hex(PC);
    // FOR NOW WE ARE NOT GENERATING THE WORST SYSTEM STATE.
    if (conservative_state == 1)
    {
      map<string, system_state*>::iterator sit = PC_worst_system_state.find(PC);
      if (sit == PC_worst_system_state.end())
      {
        // NEW PC
        PC_worst_system_state[PC] = sys_state;
        cout << " NEW PC " << PC_hex << endl;
      }
      else
      {
        // COMPARE THE SYSTEM STATES
        system_state*  stored_state = sit->second;
        system_state & this_state = *sys_state;
        bool can_skip = stored_state->compare_and_update_state(this_state);
        if (can_skip == true)
        {
          cout << "SKIPPING ! " <<  sys_state->get_state_short() << endl;
          pmem_request_file  << "SKIPPING " << sys_state->get_state_short() << endl;
          sleep(1) ;
          continue;
        }
        cout << " OLD PC : " << PC_hex << " NOT SKIPPING." << sys_state->get_state_short() <<  endl;
        sys_state = stored_state; // USE THE STORED (AND UPDATED STATE) FOR SIMULATION
      }
    }
    else
    {
       cout << " CONSERVATIVE STATE NOT MAINTAINTED " << endl;
    }
    map<int, string>::iterator it;
    for (it = sys_state->net_sim_value_map.begin(); it != sys_state->net_sim_value_map.end(); it++)
    {
       int id = it->first; string sim_val = it->second;
       nets[id]->setSimValue(sim_val);
    }
    sim_wf = sys_state->sim_wf;
    DMemory = sys_state->DMemory;
    int start_cycle = sys_state->cycle_num;
    bool taken = sys_state->taken;
    bool not_taken = sys_state->not_taken;
/*    cout << "CHECKING " << start_cycle << endl;
    if (taken == true )
    {
      if (PC_taken_nottaken[PC].first == true)
      {
        continue;
      }
      PC_taken_nottaken[PC].first = true;
    }
    if (not_taken == true)
    {
      if (PC_taken_nottaken[PC].second == true)
      {
        //continue;
      }
      PC_taken_nottaken[PC].second = true;
    }*/

    cout << "RESUMING at " << start_cycle << endl;
    pmem_request_file << "------------------------------ RE-RUNNING FROM SAVED POINT ----------------" << endl;
    debug_file_second << "------------------------------ RE-RUNNING FROM SAVED POINT ---------------- at cycle " << start_cycle << endl;
    cycle_toggled_indices.clear();
    for (int i = start_cycle; i < num_sim_cycles; i++)
    {
/*      if (check_peripherals() == true) {
        cout << " STARTING SIMULATION NOW !! " << endl;
        print_dmem_contents(i);
        print_processor_state_profile(i, true);
      }*/
      bool brk = readMem(i, wavefront);// -->  handleCondJumps()
      brk |= checkIfHung();
      brk |= (global_curr_cycle >= num_sim_cycles);
      if (brk == true) break;
      runSimulation(wavefront, i, false); // simulates the negative edge
      writeVCDCycle(2*global_curr_cycle);

      if (!done_inputs)
      {
        done_inputs = sendInputs();
      }
      else
        recvInputs1(i, wavefront);

      updateFromMem();
      recvInputs2(i, wavefront);

      runSimulation(wavefront, i, true); // simulates  the positive edge
      writeVCDCycle(2*global_curr_cycle+1); //  Since writeVCDCycle actually writes half a cycle ! Go through the function to see why;
      updateRegOutputs(i);

      vector<int>& toggled_set_indices = cycle_toggled_indices;
      if (is_dead_end_check == true)
      {
        string toggled_gates_str;
        toggled_gates_str += m_gates[cycle_toggled_indices[0]]->getName();
        for (int j = 0; j< cycle_toggled_indices.size(); j++)
        {
          int id = cycle_toggled_indices[j] ;
          Gate* gate = m_gates[id];
          string gate_name = gate->getName();
          toggled_gates_str += ","+gate_name;
        }
        vector<int> live_toggled_set_indices;
        check_for_dead_ends(i, toggled_gates_str, live_toggled_set_indices);
        toggled_set_indices = live_toggled_set_indices;
      }

      if (sim_units == 1)
      {
        sort(toggled_set_indices.begin(), toggled_set_indices.end());
        if (!tree-> essr(toggled_set_indices, false))
          tree->insert(toggled_set_indices);
      }
      toggled_set_indices.clear(); cycle_toggled_indices.clear();

      //debug_file_second << " R10 is " << getGPR(10)  << " R14 is " << getGPR(14) << " at cycle " << i << endl;
      if (print_processor_state_profile_every_cycle == 1) {
        print_processor_state_profile(i, false, true);
      }
      else if (print_processor_state_profile_every_cycle == 2) {
        print_processor_state_profile(i, true, true);
      }
      if (probeRegisters(i) == true)
      {
        //print_processor_state_profile(i, true);
        dump_Dmemory();
        cout << "ENDING SIMULATION due to STATE CORRUPTION at " << i << endl;
        break;
      }

      if (check_sim_end(i, wavefront) == true ) {
        cout << "ENDING SIMULATION" << endl;
        break ;
      }
    }
  }
}

void PowerOpt::print_net_toggle_info()
{
  map<Net* , pair<string , bool> >::iterator it = net_val_toggle_info.begin();
 for (; it !=  net_val_toggle_info.end(); it++)
 {
   Net* net = it->first;
   bool toggled = it->second.second;
   if (toggled == true)
   {
     net_toggle_info_file << net->getName() << " Toggled" << endl;
   }
   else
   {
     string val = it->second.first;
     net_toggle_info_file << net->getName() << " Constant val : " << val  << endl;
   }

 }
}

void PowerOpt::simulation_post_processing(designTiming* T)
{
  cout << "Unique (subsetted, during insert) toggle groups count = " << tree->num_sets() << endl;
  tree->remove_subsets();
  cout << "Unique (subsetted) toggle groups count = " << tree->num_sets() << endl;

  //find_dynamic_slack_subset(T);

  dump_units();
  //dump_all_toggled_gates_file();
  print_net_toggle_info();

  print_dmem_contents(global_curr_cycle);

}

int PowerOpt::getIState()
{
  string  i_state_2,  i_state_1,  i_state_0;
  if (design == "flat_no_clk_gt")
  {
   i_state_2 = m_gates[gateNameIdMap["frontend_0_i_state_reg_2_"]]->getSimValue();
   i_state_1 = m_gates[gateNameIdMap["frontend_0_i_state_reg_1_"]]->getSimValue();
   i_state_0 = m_gates[gateNameIdMap["frontend_0_i_state_reg_0_"]]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
   i_state_2 = m_gates[gateNameIdMap["frontend_0/i_state_reg_2_"]]->getSimValue();
   i_state_1 = m_gates[gateNameIdMap["frontend_0/i_state_reg_1_"]]->getSimValue();
   i_state_0 = m_gates[gateNameIdMap["frontend_0/i_state_reg_0_"]]->getSimValue();
  }
  else assert(0);

  string i_state_str =  i_state_2 + i_state_1 + i_state_0;
  int i_state  = strtoull(i_state_str.c_str(), NULL, 2);
  return i_state;
}

int PowerOpt::getEState()
{
  string e_state_3 , e_state_2,  e_state_1,  e_state_0;
  if (design == "flat_no_clk_gt")
  {
   e_state_3 = m_gates[gateNameIdMap["frontend_0_e_state_reg_3_"]]->getSimValue();
   e_state_2 = m_gates[gateNameIdMap["frontend_0_e_state_reg_2_"]]->getSimValue();
   e_state_1 = m_gates[gateNameIdMap["frontend_0_e_state_reg_1_"]]->getSimValue();
   e_state_0 = m_gates[gateNameIdMap["frontend_0_e_state_reg_0_"]]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
   e_state_3 = m_gates[gateNameIdMap["frontend_0/e_state_reg_3_"]]->getSimValue();
   e_state_2 = m_gates[gateNameIdMap["frontend_0/e_state_reg_2_"]]->getSimValue();
   e_state_1 = m_gates[gateNameIdMap["frontend_0/e_state_reg_1_"]]->getSimValue();
   e_state_0 = m_gates[gateNameIdMap["frontend_0/e_state_reg_0_"]]->getSimValue();
  }
  else assert(0);

  string e_state_str = e_state_3 + e_state_2 + e_state_1 + e_state_0;
  int e_state  = strtoull(e_state_str.c_str(), NULL, 2);
  return e_state;
}


int PowerOpt::getInstType()
{
  string inst_type_2, inst_type_1, inst_type_0;
  if (design == "flat_no_clk_gt")
  {
    inst_type_2 = m_gates[gateNameIdMap["frontend_0_inst_type_reg_2_"]]->getSimValue();
    inst_type_1 = m_gates[gateNameIdMap["frontend_0_inst_type_reg_1_"]]->getSimValue();
    inst_type_0 = m_gates[gateNameIdMap["frontend_0_inst_type_reg_0_"]]->getSimValue();
  }
  else if (design == "modified_9_hier")
  {
    inst_type_2 = m_gates[gateNameIdMap["frontend_0/inst_type_reg_2_"]]->getSimValue();
    inst_type_1 = m_gates[gateNameIdMap["frontend_0/inst_type_reg_1_"]]->getSimValue();
    inst_type_0 = m_gates[gateNameIdMap["frontend_0/inst_type_reg_0_"]]->getSimValue();
  }
  else assert(0);

  string inst_type_str = inst_type_2 + inst_type_1 + inst_type_0;
  int inst_type = strtoull(inst_type_str.c_str(), NULL, 2);
  return inst_type;
}

string PowerOpt::getGPR(int num)
{
  stringstream ss;
  if (design == "flat_no_clk_gt")
  {
    string gpr_val_string = "";
    for (int id = 15; id >= 0 ; id--)
    {
      ss << "execution_unit_0_register_file_0_r" << num << "_reg_" << id << "_";
      string gpr_bit_string = ss.str();
      string val;
      map<string, int>::iterator it = gateNameIdMap.find(gpr_bit_string);
      if (it == gateNameIdMap.end())
        val = "0";
      else
        val = m_gates[it->second]->getSimValue();
      gpr_val_string = gpr_val_string + val;
      ss.str("");
    }
    //int gpr_val_int = strtoull(gpr_val_string.c_str(), NULL, 2);// bin to int
    return gpr_val_string;
  }
}

system_state* PowerOpt::get_current_system_state(int cycle_num)
{
  system_state * s_state;
  if (design == "CORTEXM0PLUS")
    s_state = new system_state(nets, sim_wf, DMemory,  cycle_num, getPortVal("HADDR", 0, 31, '['), getPortVal("HRDATA", 0, 31, '['), input_count);
  else
    s_state = new system_state(nets, sim_wf, DMemory,  cycle_num, getPC(), pmem_instr,  false );
  return s_state;
}

system_state* PowerOpt::get_current_system_state_at_branch(int cycle_num, int branch_id)
{
  system_state * s_state;
  if (design == "CORTEXM0PLUS")
    s_state = new system_state(nets, sim_wf, DMemory,  cycle_num, getPortVal("HADDR", 0, 31, '['), getPortVal("HRDATA", 0, 31, '['), input_count, branch_id);
  else
    s_state = new system_state(nets, sim_wf, DMemory,  cycle_num, getPC(), pmem_instr,  false );
  return s_state;
}

bool PowerOpt::get_conservative_state(system_state* sys_state)
{
  string PC = sys_state->PC;
  if (PC.find("X") != string::npos) { cout << "PC Corrupt at sys_state with ID : " << sys_state->global_id << endl; return true; }
  string PC_hex = bin2hex(PC);
  map<string, system_state*>::iterator sit = PC_worst_system_state.find(PC);
  if (sit == PC_worst_system_state.end())
  {
    // NEW PC
    PC_worst_system_state[PC] = sys_state;
    cout << " NEW PC " << PC_hex << endl;
  }
  else
  {
    // COMPARE THE SYSTEM STATES
    system_state & stored_state = *(sit->second);
    system_state & this_state = *sys_state;
    bool can_skip = stored_state.compare_and_update_state(this_state);
    if (can_skip == true)
    {
      cout << "SKIPPING ! : " << sys_state->get_state_short() << endl;
      pmem_request_file  << "SKIPPING " << sys_state->get_state_short() << endl;
      return true;
    }
    cout << " OLD PC : " << PC_hex << " NOT SKIPPING "  << sys_state->get_state_short() << endl;
    sys_state = & stored_state; // USE THE STORED (AND UPDATED STATE) FOR SIMULATION
  }
  return false;
}


bool PowerOpt::probeRegisters_ARM(int& cycle_num)
{
  if (design != "CORTEXM0PLUS") return false;
  handleBranches_ARM(cycle_num);
  string V ;
  string N ;
  string Z ;
  string C ;
  N = terms[terminalNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_/Q"]]->getSimValue();
  Z = terms[terminalNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_/Q"]]->getSimValue();
  C = terms[terminalNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_/Q"]]->getSimValue();
  V = terms[terminalNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_/Q"]]->getSimValue();
/*  string apsr_reg = getRegVal("apsr_q", 0, 3);
  pmem_request_file << " APSR is " << apsr_reg << " at cycle " << cycle_num << endl;*/

   bool state_corrupted = false;
   if (instr_name == "BEQ" || instr_name == "BNE")
   {
     if (Z == "X")
     {
       pmem_request_file << "Z is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);
     }
   }
   else if (instr_name == "BCS" || instr_name == "BCC")
   {
     if (C == "X")
     {
       pmem_request_file << "C is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);

     }

   }
   else if (instr_name == "BMI" || instr_name == "BPL")
   {
     if (N == "X")
     {
       pmem_request_file << "N is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);


     }

   }
   else if (instr_name == "BVS" || instr_name == "BVC")
   {
     if (V == "X")
     {
       pmem_request_file << "V is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);


     }

   }
   else if (instr_name == "BHI" || instr_name == "BLS")
   {

     if (C == "X")
     {
       state_corrupted = true;
       if ( Z == "X")
       {
         pmem_request_file << "C and Z are corrupt" << endl;
         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf); // C
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf); // Z
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);

         // POSSIBILITY 3
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_3);

         // POSSIBILITY 4
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_4);

       }
       else
       {
         pmem_request_file << "C (not Z) is corrupt" << endl;
         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);
       }
     }
     else if (Z == "X")
     {
       state_corrupted = true;
       pmem_request_file << "Z (not C) is corrupt" << endl;
       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);
     }
     else { } // DO NOTHING

/*     if (C == "X" || Z == "X")
     {
       pmem_request_file << "C or Z is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf); // C
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf); // Z
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);

       // POSSIBILITY 3
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_3);

       // POSSIBILITY 4
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_1_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_4);
     }*/

   }
   else if (instr_name == "BGE" || instr_name == "BLT")
   {

     if (N == "X")
     {
       state_corrupted = true;
       if  (V == "X")
       {
         pmem_request_file << "N and V are corrupt" << endl;
         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);

         // POSSIBILITY 3
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_3);

         // POSSIBILITY 4
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_4);
       }
       else
       {
         pmem_request_file << "N (not V) is corrupt" << endl;

         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);

       }
     }
     else if (V =="X")
     {
       pmem_request_file << "V (not N) is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);


     }
     else {} // DO NOTHING
/*     if (N == "X" || V == "X")
     {
       pmem_request_file << "N or V is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);

       // POSSIBILITY 3
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_3);

       // POSSIBILITY 4
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_4);

     }*/

   }
   else if (instr_name == "BGT" || instr_name == "BLE")
   {
     if ( Z == "X")
     {
       state_corrupted = true;
       if (N == "X")
       {
         if (V == "X") // ZNV
         {
           pmem_request_file << "Z and N and V are corrupt" << endl;
           // POSSIBILITY 1
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_1);

           // POSSIBILITY 2
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_2);

           // POSSIBILITY 3
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_3);

           // POSSIBILITY 4
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_4);

           // POSSIBILITY 5
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_5 = get_current_system_state_at_branch(cycle_num, 5) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_5->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_5->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_5);

           // POSSIBILITY 6
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_6 = get_current_system_state_at_branch(cycle_num, 6) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_6->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_6->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_6);

           // POSSIBILITY 7
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_7 = get_current_system_state_at_branch(cycle_num, 7) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_7->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_7->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_7);

           // POSSIBILITY 8
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_8 = get_current_system_state_at_branch(cycle_num, 8) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_8->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_8->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_8);
         }
         else // ZN
         {
           pmem_request_file << "Z and N (not V) are corrupt" << endl;
           // POSSIBILITY 1
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_1);

           // POSSIBILITY 2
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_2);

           // POSSIBILITY 3
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_3);

           // POSSIBILITY 4
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_4);

         }
       }
       else if (V == "X") // ZV
       {
           pmem_request_file << "Z and V (not N) are corrupt" << endl;
           // POSSIBILITY 1
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_1);

           // POSSIBILITY 2
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_2);

           // POSSIBILITY 3
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
           system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_3);

           // POSSIBILITY 4
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
           m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
           system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
           cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
           sys_state_queue.push(sys_state_4);


       }
       else // Z
       {
         pmem_request_file << "Z (not N and not V) is corrupt" << endl;
         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);
       }
     }
     else if (N == "X")
     {
       if (V == "X") // NV
       {
         pmem_request_file << "N and V (not Z) are corrupt" << endl;
         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);

         // POSSIBILITY 3
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_3);

         // POSSIBILITY 4
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_4);

       }
       else // N
       {
         pmem_request_file << "N (not V not Z) is corrupt" << endl;
         state_corrupted = true;

         // POSSIBILITY 1
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
         system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);

         // POSSIBILITY 2
         m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
         system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
         cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_2);


       }
     }
     else if (V == "X") // V
     {
       pmem_request_file << "V (not N not Z) is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);

     }
     else {}
/*     if (Z == "X"  || N == "X" || V == "X")
     {
       pmem_request_file << "Z or N or V is corrupt" << endl;
       state_corrupted = true;

       // POSSIBILITY 1
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_1 = get_current_system_state_at_branch(cycle_num, 1) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_1);

       // POSSIBILITY 2
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_2 = get_current_system_state_at_branch(cycle_num, 2) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_2->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_2);

       // POSSIBILITY 3
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_3 = get_current_system_state_at_branch(cycle_num, 3) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_3->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_3);

       // POSSIBILITY 4
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_4 = get_current_system_state_at_branch(cycle_num, 4) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_4->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_4);

       // POSSIBILITY 5
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_5 = get_current_system_state_at_branch(cycle_num, 5) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_5->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_5->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_5);

       // POSSIBILITY 6
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("0", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_6 = get_current_system_state_at_branch(cycle_num, 6) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_6->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_6->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_6);

       // POSSIBILITY 7
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("0", sim_wf);
       system_state* sys_state_7 = get_current_system_state_at_branch(cycle_num, 7) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_7->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_7->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_7);

       // POSSIBILITY 8
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_0_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_2_"]]->setSimValueReg("1", sim_wf);
       m_gates[gateNameIdMap["u_top_u_sys_u_core_apsr_q_reg_3_"]]->setSimValueReg("1", sim_wf);
       system_state* sys_state_8 = get_current_system_state_at_branch(cycle_num, 8) ;
       cout << "PUSHING SYSTEM STATE : " << sys_state_8->get_state_short() <<  endl;
       pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_8->get_state_short() <<  endl;
       sys_state_queue.push(sys_state_8);

     }*/

   }
   return state_corrupted;
}

bool PowerOpt::probeRegisters(int& cycle_num)
{
   string V ;
   string N ;
   string Z ;
   string C ;

  int fork_e_state, fork_inst_type;
  int e_state = getEState();
  int inst_type = getInstType();
  if (design == "flat_no_clk_gt")
  {
    V = m_gates[gateNameIdMap["execution_unit_0_register_file_0_r2_reg_8_"]]->getSimValue();
    N = m_gates[gateNameIdMap["execution_unit_0_register_file_0_r2_reg_2_"]]->getSimValue();
    Z = m_gates[gateNameIdMap["execution_unit_0_register_file_0_r2_reg_1_"]]->getSimValue();
    C = m_gates[gateNameIdMap["execution_unit_0_register_file_0_r2_reg_0_"]]->getSimValue();
    fork_e_state = 5; // actual e_state = 1011
    fork_inst_type = 5; // actual inst_type = 010
  }
  else if (design == "modified_9_hier")
  {
    V = m_gates[gateNameIdMap["execution_unit_0/register_file_0/r2_reg_8_"]]->getSimValue();
    N = m_gates[gateNameIdMap["execution_unit_0/register_file_0/r2_reg_2_"]]->getSimValue();
    Z = m_gates[gateNameIdMap["execution_unit_0/register_file_0/r2_reg_1_"]]->getSimValue();
    C = m_gates[gateNameIdMap["execution_unit_0/register_file_0/r2_reg_0_"]]->getSimValue();
    fork_e_state = 12; // No check needed
    fork_inst_type = 4; // No check needed
  }
  else assert(0);

  if (jump_detected == true)
  {
     jump_cycle ++;
     jump_detected = false;
  }

   bool state_corrupted = false;
    //debug_file << "E_state " << e_state << " Inst_type " << inst_type <<  " at " << cycle_num << endl;
   //if (jump_cycle == 4)// e_state == 1011 && inst_type == 010
   if ((e_state == fork_e_state) && (inst_type == fork_inst_type))// e_state == 1011 && inst_type == 010
   {
     if (instr_name == "JNE/JNZ" || instr_name == "JEQ/JZ" )
     {
       if (Z == "X")
       {
         pmem_request_file << "Z is corrupt\n" ;
         state_corrupted = true;
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_1_/Q"]]->setSimValue("0", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_1_/Q"]]->setSimValue("0", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 1" << endl;
         system_state* sys_state_1 = get_current_system_state(cycle_num) ;
         sys_state_1->not_taken = true;
         cout << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         pmem_request_file << "PUSHING SYSTEM STATE : " << sys_state_1->get_state_short() <<  endl;
         sys_state_queue.push(sys_state_1);
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_1_/Q"]]->setSimValue("1", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_1_/Q"]]->setSimValue("1", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 2" << endl;
         system_state* sys_state_2 = get_current_system_state(cycle_num) ;
         sys_state_2->taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         sys_state_queue.push(sys_state_2);
       }
     }
     else if (instr_name == "JNC/JLO" || instr_name == "JC/JHS" )
     {
       if (C == "X")
       {
         pmem_request_file << "C is corrupt\n" ;
         state_corrupted = true;
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_0_/Q"]]->setSimValue("0", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_0_/Q"]]->setSimValue("0", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 1" << endl;
         system_state* sys_state_1 = get_current_system_state(cycle_num) ;
         sys_state_1->not_taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         sys_state_queue.push(sys_state_1);
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_0_/Q"]]->setSimValue("1", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_0_/Q"]]->setSimValue("1", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 2" << endl;
         system_state* sys_state_2 = get_current_system_state(cycle_num) ;
         sys_state_2->taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         sys_state_queue.push(sys_state_2);
       }
     }
     else if (instr_name == "JN"     )
     {
       if (N == "X")
       {
         pmem_request_file << "N is corrupt\n" ;
         state_corrupted = true;
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_2_/Q"]]->setSimValue("0", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_2_/Q"]]->setSimValue("0", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 1" << endl;
         system_state* sys_state_1 = get_current_system_state(cycle_num) ;
         sys_state_1->not_taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         sys_state_queue.push(sys_state_1);
         if (design == "flat_no_clk_gt") {terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_2_/Q"]]->setSimValue("1", sim_wf);}
         else if (design == "modified_9_hier") {terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_2_/Q"]]->setSimValue("1", sim_wf);}
         else assert(0);
         cout << "SAVING STATE 2" << endl;
         system_state* sys_state_2 = get_current_system_state(cycle_num) ;
         sys_state_2->taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         sys_state_queue.push(sys_state_2);
       }
     }
     else if (instr_name == "JGE"  ||   instr_name == "JL"      )
     {
       if (V == "X" || N == "X")
       {
         pmem_request_file << "V or N is corrupt\n" ;
         state_corrupted = true;
         if (design == "flat_no_clk_gt") {
           terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_2_/Q"]]->setSimValue("0", sim_wf);
           terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_8_/Q"]]->setSimValue("0", sim_wf);
         }
         else if (design == "modified_9_hier") {
//           pmem_request_file << " Extra Sim at cycle " << cycle_num << endl;
//           runSimulation(true, cycle_num, false);
//           runSimulation(true, cycle_num++, true);
           terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_2_/Q"]]->setSimValue("0", sim_wf);
           terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_8_/Q"]]->setSimValue("0", sim_wf);
         }
         else assert(0);
         cout << "SAVING STATE 1" << endl;
         system_state* sys_state_1 = get_current_system_state(cycle_num) ;
         sys_state_1->not_taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_1->get_state_short() << endl;
         sys_state_queue.push(sys_state_1);
         if (design == "flat_no_clk_gt") {
           terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_2_/Q"]]->setSimValue("0", sim_wf);
           terms[terminalNameIdMap["execution_unit_0_register_file_0_r2_reg_8_/Q"]]->setSimValue("1", sim_wf);
           }
         else if (design == "modified_9_hier") {
           terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_2_/Q"]]->setSimValue("0", sim_wf);
           terms[terminalNameIdMap["execution_unit_0/register_file_0/r2_reg_8_/Q"]]->setSimValue("1", sim_wf);
           }
         else assert(0);
         cout << "SAVING STATE 2" << endl;
         system_state* sys_state_2 = get_current_system_state(cycle_num) ;
         sys_state_2->taken = true;
         cout << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         pmem_request_file << "PUSHING SYSTEM STATE"  << sys_state_2->get_state_short() << endl;
         sys_state_queue.push(sys_state_2);
       }
     }
   }

   if (state_corrupted) return true;
//   else
//   {
//     if (jump_detected == true)
//     {
//       jump_cycle ++;
//       jump_detected = false;
//
//     }
//   }
  return false;
}



void PowerOpt::updateFromMem()
{
  sendData(dmem_data);
  sendInstr(pmem_instr);
}

void PowerOpt::dumpPmem()
{
  pmem_contents_file << "PMEM Contents " << endl;
  for (int i = 0; i < PMemory.size(); i++)
  {
    pmem_contents_file << i << " : " << hex << PMemory[i]->instr << dec << " : ";
    vector<bool>& domain_activity = PMemory[i]->domain_activity;
    for (int j = 0; j < cluster_id; j++)
    {
      if (domain_activity[j] == true) pmem_contents_file << " ON : ";
      else pmem_contents_file << " OFF : ";
    }
    if (PMemory[i]->executed == true) pmem_contents_file << " Executed ";
    pmem_contents_file << endl;
  }
}

void PowerOpt::dump_Dmemory()
{
  debug_file << "DMEM Contents" << endl;

  map<int, xbitset >::iterator it = DMemory.begin();
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

  //vector<Gate*>::iterator it = m_gates.begin();
  while (graph->getWfSize() != 0)
  {
    GNode* node = graph->getWfNode();
    debug_file << "Removing : " << node->getName() << endl;
    graph->popWfNode();
    if (node->getVisited() == true)
      continue;
    nodes_topo.push_back(node);
    debug_file << " Setting Topo_id of Node : " << node->getName() << " as "  << count << endl;
    node->setTopoId(count++);
    node->setVisited(true);
    vector<Net*> nets;
    if (node->getIsPad() == true)
    {
      Pad* pad = node->getPad();
      if (pad->getNetNum() == 0) continue;
      nets.push_back(pad->getNet(0));
    }
    else
    {
      assert(node->getIsGate() == true);
      Gate* gate = node->getGate();
      //assert(gate->getFanoutTerminalNum() == 1);
      for (int i = 0; i < gate->getFanoutTerminalNum() ; i++)
      {
        nets.push_back(gate->getFanoutTerminal(i)->getNet(0));
      }
    }
    for (int j = 0; j < nets.size(); j++)
    {
      Net * net = nets[j];
      net->setTopoSortMarked(true);
      debug_file << "   Net is : " <<  net->getName() << endl;
      for (int i = 0; i < net->getTerminalNum(); i++)
      {
        Terminal * term = net->getTerminal(i);
        if (term->getPinType() == OUTPUT)
          continue;
        debug_file << "       Terminal is : " << term->getFullName() << endl;
        Gate* gate_nxt = term->getGate();
        GNode* node_nxt = gate_nxt->getGNode();
        if (gate_nxt->allInpNetsVisited() == true && node_nxt->getVisited() == false)
        {
          debug_file << "Adding : " << node_nxt->getName() << " from terminal : " << term->getFullName() << " and Net : " << net->getName() << endl;
          graph->addWfNode(node_nxt);// add to queue
        }
      }
    }
  }
}

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
   cluster_id = 0;
   float time_period = 1e-8;
   if (cluster_file_names.is_open()){
     string cluster_file_name;
     while ( getline(cluster_file_names, cluster_file_name))
     {
       ifstream cluster_file;
       trim(cluster_file_name);
       cluster_file.open(cluster_file_name.c_str());
       Cluster* cluster = new Cluster;
       cluster->setId(cluster_id);
       float cluster_leakage_energy = 0.0;
       if(cluster_file.is_open())
       {
         string gate_name;
         while (getline(cluster_file, gate_name))
         {
           Gate* gate = m_gates[gateNameIdMap[gate_name]];
           cluster_leakage_energy += libLeakageMap[gate->getCellName()]*time_period;
           gate->setClusterId(cluster_id);
           cluster->push_back(gate);
         }
         cluster_leakage_energy = cluster_leakage_energy*1e-9;
         cluster->setLeakageEnergy(cluster_leakage_energy);
       }
       clusters.insert(make_pair(cluster_id, cluster)); // bad coding. Might copy the entire vector into the map;
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
    map<int, Cluster* > :: iterator it;
    int total_cut_count = 0;
    for (it = clusters.begin(); it != clusters.end(); it++)
    {
        vector<Gate*> cluster = it->second->getMembers();;
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

void PowerOpt::print_const_nets()
{
    ofstream const_nets_file;
    const_nets_file.open("PowerOpt/const_nets_file");
    const_nets_file << " suppress_message PTE-018 " << endl;
    const_nets_file << " suppress_message PTE-003 " << endl;
    const_nets_file << " suppress_message PTE-060 " << endl;
    const_nets_file << " suppress_message PTE-017 " << endl;
    for (int i = 0; i < getNetNum(); i++)
    {
      Net * net = getNet(i);
      if (net->getToggled() == false && net->getSimValue() != "X") const_nets_file << " set_case_analysis " << net->getSimValue() << "  [ get_pins -of_objects [ get_net  " << net->getName() << " ]]" << endl;
    }
    const_nets_file.close();
}

void PowerOpt::print_nets()
{
  debug_file << "FINAL " << endl;
  for (int i = 0; i < getNetNum(); i++)
  {
    Net * net = getNet(i);
    debug_file << "net : " << net->getName() << endl;
    //if (net->getToggled() == false && net->getSimValue() != "X") debug_file << " NOT TOGGLED " << net->getName() << " val : " << net->getSimValue() << endl;
    //else if (net->getSimValue() == "X") debug_file << " IS X " << net->getName() << endl;
//    Gate* gate = net->getDriverGate();
//    if (net->getToggled() == true)
//      debug_file << "TOGGLED " <<  net->getName() << endl;
//    else
//      debug_file << "NOT TOGGLED " << net->getName() << endl;
  }
/*  Gate* gate = m_gates[gateNameIdMap["execution_unit_0/register_file_0/U372"]];
  debug_file << "Gate is " << gate->getName() << endl;
  for (int i = 0; i < gate->getFaninTerminalNum(); i ++)
  {
    Terminal * term = gate->getFaninTerminal(i);
    debug_file << term->getName() << " : " << term->getNetName() << endl;
  }*/
/*      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "S")
            S_val = term->getSimValue();
         else if (term->getName() == "I1")
            I1_val = term->getSimValue();
         else if (term->getName() == "I0")
            I0_val = term->getSimValue();
      }*/

  //Terminal* term = terms[terminalNameIdMap["execution_unit_0/register_file_0/U372/S"]];
/*  debug_file << " Term is " << term->getFullName() << endl;
  debug_file << term->getNetName() << endl;*/

}

void PowerOpt::print_regs()
{
  debug_file << " REGS : " << endl;
  list<Gate*>::iterator it;
  for(it = m_regs.begin(); it != m_regs.end(); it++)
  {
    Gate* ff_gate = *it;
    debug_file << ff_gate->getName() << endl;
  }
  debug_file << " DONE REGS " << endl;

  debug_file << " REG OUT_PINS : " << endl;
  for (it = m_regs.begin(); it!=m_regs.end(); it++)
  {
    Gate* ff_gate = *it;
    for (int i =0; i < ff_gate->getFanoutTerminalNum(); i++)
    {
      Terminal* term = ff_gate->getFanoutTerminal(i);
      debug_file << term->getFullName() << endl;
    }
  }
  debug_file << " DONE REG OUT_PINS : " << endl;
}

void PowerOpt::print_gates()
{
  debug_file << " GATES : " << endl;
  for (int i =0; i < getGateNum(); i++)
  {
    Gate* gate = m_gates[i];
    debug_file << gate->getName() << " : " << i << " NUM FANOUT TERMS : " << gate->getFanoutTerminalNum() <<  endl;
  }
  debug_file << " DONE GATES " << endl;

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
  debug_file << " TERMS : " << endl;
  for (int i = 0; i < getTerminalNum(); i++)
  {
    Terminal* term = terms[i];
    debug_file << i << " : " << term->getFullName() << " --> " ;
    term->printNets(debug_file);
    debug_file << endl;
  }
  debug_file << " DONE TERMS " << endl;
}

void PowerOpt::printTopoOrder()
{
  list<GNode*>::iterator it = nodes_topo.begin();
  for (it = nodes_topo.begin(); it != nodes_topo.end(); it ++)
  {
     GNode* node = *it;
     cout << node->getTopoId() << " : " <<  node->getName() << " : " << node->getType() << endl;
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
    //graph->print();
    cout << "Finished Top Sort" << endl;
    printTopoOrder();
    cout << " Checking topo order ... " ;
    checkTopoOrder(graph);
    cout << " done " << endl;

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

string PowerOpt::getVCDAbbrev(int id)
{
    char first_val = 33;
    char last_val = 126;
    int num_chars = last_val-first_val+1;
    string abbrev("");

    int val = id;
    do {
        char c = (char) (val % num_chars);
        abbrev += c+first_val;
        //cout << val << " " << c+first_val<< endl;
        val /= num_chars;

    } while (val > 0);

    reverse(abbrev.begin(),abbrev.end());
    return abbrev;
}

void PowerOpt::writeVCDInitial (ofstream& vcd_file)
{

    vcd_file << "$date" << endl;
    time_t t = time(0);
    struct tm * now = localtime(&t);
    string date_time_str = asctime(now);
    vcd_file << date_time_str;
    vcd_file << "$end" << endl;

    // Write version section
    vcd_file << "$version" << endl;
    //vcd_file << "PowerOpt version `\\_(o_O)_/`?" <<endl;
    vcd_file << "PowerOpt version 2.0" <<endl;
    vcd_file << "$end" << endl;

    // Write comment section
    vcd_file << "$comment" << endl;
    vcd_file << "" << endl;
    vcd_file << "$end" << endl;

    // Write timescale
    vcd_file << "$timescale 10ps $end" << endl;

    // Generate and write VCD abbreviations for each net in design
    vcd_file << "$scope module dut $end" << endl;
    for (int i = 0; i<nets.size(); ++i) {
	Net *n =getNet(i);
        string netName = n->getName();
        // VCD scalars cannot have [ or ] charcters. From VCS output VCD files it seems that _ characters are used to denote bit selection.
        size_t pos = 0;
        while ((pos = netName.find("\\[")) != string::npos) {
            netName.replace(pos,2,"_");
        }
        while ((pos = netName.find("\\]")) != string::npos) {
            netName.replace(pos,2,"_");
        }
        string abbrev = getVCDAbbrev(i);
        vcd_file << "$var wire 1 " << abbrev << " " << netName << " $end" << endl;
        n->setVCDAbbrev(abbrev);
    }
    // FIXME: Write any module definitions which matter mainly for hierarchical designs

    // Incomplete FIX: currently works only for flat netlist.

    for (int i = 0; i < m_gates.size(); i++)
    {
        Gate* gate = m_gates[i];
        string gate_name = gate->getName();
        vcd_file << "$scope module " << gate_name << " $end" << endl;
        int num_gate_fi_terms = gate->getFaninTerminalNum();
        for (int j = 0; j < num_gate_fi_terms; j++)
        {
            Terminal* term = gate->getFaninTerminal(j);
            Net * net = term->getNet(0);
            vcd_file << "$var wire 1 " << net->getVCDAbbrev() << " " << term->getName() << " $end" << endl ;
        }
        int num_gate_fo_terms = gate->getFanoutTerminalNum();
        for (int j = 0; j < num_gate_fo_terms; j++)
        {
            Terminal* term = gate->getFanoutTerminal(j);
            Net * net = term->getNet(0);
            vcd_file << "$var wire 1 " << net->getVCDAbbrev() << " " << term->getName() << " $end" << endl ;
        }
        vcd_file << "$upscope $end" << endl;
    }

    vcd_file << "$upscope $end" << endl;
    vcd_file << "$enddefinitions $end" << endl;


    // Write initial dump section
    vcd_cycle = 0;
    vcd_file << "#" << vcd_cycle << endl;
    vcd_file << "$dumpvars" << endl;
    for (int i = 0; i<nets.size(); ++i) {
        Net *n =getNet(i);
        vcd_file << n->getSimValue() << n->getVCDAbbrev() << endl;
    }
    vcd_file << "$end" << endl;




}

void PowerOpt::writeVCDBegin()
{
    if (!vcd_odd_file.is_open()) {
        cout << "WARNING: odd VCD Output file Cannot be opened!" << endl;
	//return;
    }
    if (!vcd_even_file.is_open()) {
        cout << "WARNING: even VCD Output file Cannot be opened!" << endl;
	//return;
    }

    //dual_stream vcd_file(vcd_odd_file, vcd_even_file);
    //
//    typedef tee_device<ofstream, ofstream> TeeDevice;
//    typedef stream<TeeDevice> TeeStream;
//    TeeDevice my_tee(vcd_odd_file, vcd_even_file);
//    TeeStream vcd_file(my_tee);
    //vcd_file = tee(vcd_odd_file, vcd_even_file);

    // Write date stection
    writeVCDInitial (vcd_odd_file);
    writeVCDInitial (vcd_even_file);
    writeVCDInitial (vcd_file);

    // Create structures to hold nets that toggled in the last two cycles
    vcd_writes_p1 = new map<string,Net*>();
    vcd_writes_0 = new map<string,Net*>();

}

// Adds the current simulation value of Net n to those that should be written to the VCD next cycle, assuming the value does not head post-processing (X->0 or X->1 requires the original X state)
void PowerOpt::addVCDNetVal(Net *n)
{
     vcd_writes_p1->insert(make_pair(n->getName(), n));
}


void PowerOpt::writeVCDLastCycle(int cycle_num)
{
    map<string, Net*> :: iterator n;
    for (n = vcd_writes_0->begin(); n != vcd_writes_0->end(); ++n)
    {
        string curr_value = n->second->getSimValue();  // p1
        writeVCDNets(n->second, cycle_num, curr_value, curr_value, curr_value);
    }
}


void PowerOpt::writeVCDCycle(int cycle_num)
{
    // For odd file, worst case is in odd cycles (i.e., X->X transitions are 0->1 tranistions during the odd cycles)
    // Even files has the worst case in the eycles
    // Iterate through each net and write out the correct value for the current write cycle
    string write_value_odd;
    string write_value_even;
    int write_cycle = cycle_num-1;
    map<string, Net*> :: iterator n;
    debug_file << " Cycle " << cycle_num << endl;
    for (n = vcd_writes_0->begin(); n != vcd_writes_0->end(); ++n)
    {
//        string write_value, old_value;
//        string curr_value = n->second->getSimValue();  // p1
//        //write_value = n->getOldSimVal(0); // 0
//        //old_value = n->getOldSimVal(1);  // m1
//
//        //Make sure that the sim values read correspond to consecutive cycles else, edit the values
//        if (vcd_writes_p1->find(n->second->getName()) == vcd_writes_p1->end())
//        {
//            write_value = curr_value; // did not actually toggle in p1 so we can just use curr_value
//            old_value = n->second->getOldSimVal(0);
//        }
//        else
//        {
//            write_value = n->second->getOldSimVal(0); // 0
//            old_value = n->second->getOldSimVal(1);  // m1
//        }
//        string actual_value = write_value;
////        write_value = n->second->getOldSimVal(0); // 0
////        old_value = n->second->getOldSimVal(1);  // m1
////        write_value = curr_value; // did not actually toggle in p1 so we can just use curr_value
////        old_value = n->second->getOldSimVal(0);
//
//        if (write_value == "X") {
//            if (write_cycle%2) {
//                // This is an odd cycle
//
//                // Write worst case to odd file
//                if (old_value == "1") {
//                    // write a "0"
//                    write_value_odd = "0";
//                } else {
//                    // old_value is a "0" or an "X", so write a "1"
//                    write_value_odd = "1";
//                }
//
//                // Write the value that will cause the worst case in the p1 cycle to the even file
//                if (curr_value == "1" || curr_value == "X") {
//                    // write a "0"
//                    write_value_even = "0";
//                } else {
//                    // curr_value is a "0", so write a "1"
//                    write_value_even = "1";
//                }
//
//            } else {
//                // This is an even cycle
//
//                // Write the value that will cause the worst case in the p1 cycle to the odd file
//                if (curr_value == "1" || curr_value == "X") {
//                    // write a "0"
//                    write_value_odd = "0";
//                } else {
//                    // curr_value is a "0", so write a "1"
//                    write_value_odd = "1";
//                }
//
//                // Write worst case to even file
//                if (old_value == "1") {
//                    // write a "0"
//                    write_value_even = "0";
//                } else {
//                    // old_value is a "0" an "X", so write a "1"
//                    write_value_even = "1";
//                }
//            }
//        } else {
//            write_value_odd = write_value_even = write_value;
//        }
//        n->second->write_value = write_value;
//        n->second->value_even = write_value_even;
//        n->second->value_odd = write_value_odd;
        string actual_value = n->second->getSimValue();
        writeVCDNets(n->second, write_cycle, write_value_odd, write_value_even, actual_value);
    }

//    debug_file_second << "#" <<  write_cycle*clockPeriod/2 << endl;
//    for ( int i = 0; i <  nets.size(); i++)
//    {
//      Net* n = nets[i];
//      debug_file_second << n->getName() << " : " << n->getOldSimVal(1) << " : " << n->getOldSimVal(0) << " : " << n->getSimValue() << " : " << n->write_value <<  " : " <<  n->value_even << " : " << n->value_odd << endl;
//    }

    //cout << "Sizes : " << vcd_writes_p1->size() << " : "  << vcd_writes_0->size() << endl;
    // update the past cycle VCD function
    free(vcd_writes_0);
    vcd_writes_0 = vcd_writes_p1;
    vcd_writes_p1 = new map<string, Net*>();
}

// Writes a given nets value at a given cycle to the provided file.
void PowerOpt::writeVCDNets(Net *n, int cycle_num, string value_odd, string value_even, string actual_value)
{
    // Catch any empty simulation values
    if (value_even.empty()) {
        //debug_file << "Empty even sim value for net: " << n->getName() << " for cycle: " << cycle_num << endl;
        //return;
    }
    if (value_odd.empty()) {
        //debug_file << "Empty odd sim value for net: " << n->getName() << " for cycle: " << cycle_num << endl;
        //return;
    }

    //cout << "*";
    if (vcd_cycle != cycle_num)
    {
      vcd_cycle = cycle_num;
      debug_file << n->getName() << " : " << n->getSimValue() << endl;
      vcd_file << "#" << cycle_num*clockPeriod/2 << endl;
    }
    vcd_file << actual_value << n->getVCDAbbrev() << endl;
    if (!vcd_odd_file.is_open() || !vcd_even_file.is_open()) { }
    else {
        if (vcd_cycle != cycle_num) {
            vcd_cycle = cycle_num;
            vcd_odd_file << "#" <<  cycle_num*clockPeriod/2 << endl;
            vcd_even_file << "#" <<  cycle_num*clockPeriod/2 << endl;
            //vcd_file << "#" << cycle_num*clockPeriod/2 << endl;
            debug_file_second << "#" << cycle_num*clockPeriod/2 << endl;
         }
        //cout << "n";
        vcd_odd_file << value_odd << n->getVCDAbbrev() << endl;
        vcd_even_file << value_even << n->getVCDAbbrev() << endl;
        //vcd_file << actual_value << n->getVCDAbbrev() << endl;
        //if (cycle_num*clockPeriod/2 == 130000)
          //debug_file_second << n->getName() << n->getDriverGate()->getName() <<  " : " << n->getOldSimVal(1) << " : " << n->getOldSimVal(0) << " : " << n->getSimValue() << " : " << actual_value <<  " : " <<  value_even << " : " << value_odd << endl;
    }

}
// Adds the current simulated value of Net n to the VCD file. Assumes that a toggle has just taken place.
//void PowerOpt::writeVCDNet(Net *n)
//{
//    if (vcd_file.is_open()) {
//        vcd_file << n->getSimValue() << n->getVCDAbbrev() << endl;
//    }
//}


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
{// PIN BASED DTA ANALYSIS
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

void PowerOpt::handle_toggled_nets_to_get_processor_state( vector < pair < string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
  for (int i = 0; i < toggled_nets.size(); i++)
  {
    string net_name = toggled_nets[i].first;
    string value = toggled_nets[i].second;
    replace_substr(net_name, "[", "\\[" ); // FOR PT SOCKET
    string top_net_name = T->getTopNetName(net_name);
    if (top_net_name == "")
    {
      cout << "Net Name is " << net_name << endl;
      continue;
    }
    if (netNameIdMap.find(top_net_name) != netNameIdMap.end())
    {
      Net* net = nets[netNameIdMap.at(top_net_name)];
      net->setSimValue(value);
    }
    else if (terminalNameIdMap.find(top_net_name) != terminalNameIdMap.end())
    {
      Terminal * term = terms[terminalNameIdMap.at(top_net_name)];
      term->setSimValue(value);
    }
    else
    {
      cout << " Top net Name is " << top_net_name << endl;
      assert(0);
    }
  }

  print_processor_state_profile(cycle_time, false, true);

}

void PowerOpt::handle_toggled_nets(vector< pair<string, string> > & toggled_nets, designTiming* T, int cycle_num, int cycle_time)
{
  cout << " At time " << cycle_time << " Toggled Nets size " << toggled_nets.size() << endl;
//  handle_toggled_nets_to_get_processor_state(toggled_nets, T, cycle_num, cycle_time);
//  clearToggled();
//  toggled_nets.clear();
//  return;
//  handle_toggled_nets_to_pins(toggled_nets, T, cycle_num, cycle_time);
//  return ;
  //static int pathID = 0;
  //net_gate_maps << "Cycle " << cycle_time << endl;
  if (exeOp == 22) return;
  if (toggled_nets.size())
  {
    cout << "Handling Toggled Nets"  << endl;
    //debug_file << " CYCLE : " << cycle_num << endl;
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
          //net_gate_maps << toggled_nets[i].first  << " --> " << cellstr << endl;
          if ( cellstr != "NULL_CELL" ) {
            if ( cellstr == "MULTI_DRIVEN_NET" ) {
              cout << "[ERROR] This net " << net_name << " is driven by multiple cells" << endl;
            }
            //map<string, Gate*>::iterator gate_lookup2 =  gate_name_dictionary.find(cellstr);
            int CurInstId = gateNameIdMap[cellstr];
            m_gates[CurInstId]->setToggled ( true, toggled_nets[i].second) ;
            m_gates[CurInstId]->incToggleCount();
            m_gates[CurInstId]->updateToggleProfile(cycle_num);
            //debug_file << cellstr << " : " <<  toggled_nets[i].first << " : " << toggled_nets[i].second << endl;
            if (toggled_nets[i].second == "x")
              X_valued_gates.insert(cellstr);
            else  X_valued_gates.erase(cellstr); // no problem if non-existent
          }
      }
      set<string> ::iterator it;
      for (it = X_valued_gates.begin(); it != X_valued_gates.end(); it ++)
      {
        string gate_name =  *it;
        Gate* gate = m_gates.at(gateNameIdMap.at(gate_name));
        for (int l = 0; l < gate->getFaninTerminalNum(); ++l)
        {
          Terminal * terminal = gate->getFaninTerminal(l);
          Net* net = terminal->getNet(0);
          Gate* fanin = net->getDriverGate();
          if (fanin != 0 && fanin->isToggled() == true)
          {
            if (gate->isToggled() == false) gate->incToggleCount();
            gate->setToggled(true, "x");
            gate->updateToggleProfile(cycle_num);
          }
        }
      }
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
  }
  //clearToggled();
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

/*        if (modules_of_interest.find(module_name) != modules_of_interest.end()){
          cout << "Interested in module " << module_name << endl;
          ignore_module = false;
        }
        else ignore_module = true;*/
      }
      else if (line.compare(0, 8, "$upscope") == 0) {
        hierarchy.pop_back();
        tokenize (line, ' ', line_contents);
        assert (line_contents[1] == "$end");
      }
      else if (!ignore_module && line.compare(0, 4, "$var") == 0) {
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
              //debug_file << true_net_name << endl;
            }
            else if (lc_size == 7)
            {
              true_net_name.append(line_contents[4]);
              true_net_name.append(line_contents[5]);
              net_dictionary.insert(pair<string, string> (abbrev, true_net_name));
              //debug_file << true_net_name << endl;
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
         if (time <= max_time && time >= min_time && (time %5000 == 0) )
         {
           cycle_num++;
           //debug_file_second  << " CYCLE : " << cycle_num  << " TIME : " << time << endl;
           valid_time_instant = true;
         }
         else
         {
            valid_time_instant = false;
            if (time > max_time)
            {
              cout << "TIME " << time << " EXCEEDED MAX TIME OF " <<  max_time << endl;
              break;
            }
         }
      }
      else if (valid_time_instant) { // parse for time values of interest
        //debug_file_second << line  << " :: " ;
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
                 Net * net =  nets.at(netNameIdMap.at(net_name));
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
          //assert (line.compare(0, 9, "$dumpvars") == 0); // I am sure that this line is $dumpvars
          //valid_time_instant = false; // this is just a dump of all vars with probably 'x' as values, so we don't care
        }
        else {
          abbrev = line.substr(1);// first character is the value
          //cout << " At valid time : " << time << endl;
          //debug_file_second << value << " : ";
          map<string, string> :: iterator it = net_dictionary.find(abbrev);
          if (it != net_dictionary.end())
          {
            string net_name = net_dictionary[abbrev];
            //debug_file_second << net_name;
            vector<string> tokens;
            tokenize(net_name, '/', tokens);
            if (tokens.size() == 2) net_name = tokens[1];
            if (netNameIdMap.find(net_name) != netNameIdMap.end() && terminalNameIdMap.find(net_name) == terminalNameIdMap.end() )
            {
              Net * net = nets.at(netNameIdMap.at(net_name));
              if(exeOp == 22)
              {
                net->setSimValue(value);
              }
              else
              {
                net->updateValue(value);
                //debug_file_second << endl;
                toggled_nets.push_back(make_pair(net_name, value));
              }
              //cout << net_name << " -- > "  << value << endl;
            }
            //else
              //debug_file_second << "   SKIPPED" << endl;
            // cout << " Toggled Net : " << net_name << endl;
          }
//        IMPORTANT: we miss eseveral nets because they belong to modules that are not of interest.
//        Every gate is a module in the vcd file.
          else
          {
            missed_nets << "Missed" <<  abbrev << endl;
            //debug_file_second << " MISSED " << endl;
          }
        }
        //debug_file_second << endl;
      }
      line_contents.clear();
    }
    // There is one inside the while loop
    //handle_toggled_nets(toggled_nets, T, cycle_num, time);// handle all toggled nets from previous cycle
    //handle_toggled_nets_new(toggled_nets, T, cycle_num, time); // FOR LAST CYCLE
    cout << "Total parsed cycles (all benchmarks) = " << cycle_num << endl;
    //cout << "Unique toggle groups count = " << toggled_terms_counts.size() << endl;
    cout << "Unique toggle groups count = " << toggled_sets_counts.size() << endl;
    set<int> ::iterator it;
    for (it = all_toggled_gates.begin(); it != all_toggled_gates.end(); it++)
    {
      string gate_name = m_gates.at(*it)->getName();
      toggle_info_file << gate_name << endl;
    }
//    cout << "Not Toggled gates size is " << not_toggled_gate_map.size() << endl;

    if (exeOp == 17) {
      cout << "Unique (subsetted, during insert) toggle groups count = " << tree->num_sets() << endl;
      tree->remove_subsets();
      cout << "Unique (subsetted) toggle groups count = " << tree->num_sets() << endl;
    }
    if (exeOp == 16 || exeOp == 15) {
      //cout << "Unique toggle groups count = " << toggled_gate_set.size() << endl;
      cout << "Unique toggle groups count = " << toggled_terms_counts.size() << endl;
    }
  }
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
    //toggle_info_file << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
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
         //if (cycle_num > 1 ) all_toggled_gates.insert(gate_name);
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
    //toggle_info_file << "************" << endl;
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

  cout << " DUMPING UNITS " << endl;
  list < pair < double,  vector <int> > >& collection = tree->get_collection();
  list < pair < double , vector < int > > > :: iterator it;
  for(it = collection.begin(); it != collection.end(); it++)
  {
    vector<int> & unit = it->second;
    double slack = it->first;
    units_file << slack << " : " ;
    for (int i =0; i < unit.size(); i++)
    {
      int index = unit[i];
      string gate_name = m_gates[index]->getName();
      units_file << gate_name << ", " ;
    }
    units_file << endl;
  }

}

void PowerOpt::dump_all_toggled_gates_file()
{
  set<int>::iterator it;
  for ( it = all_toggled_gates.begin(); it != all_toggled_gates.end(); it ++)
  {
    string gate_name = m_gates.at(*it)->getName();
    all_toggled_gates_file << gate_name << endl;
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
  if (maintain_toggle_profiles == 0)
  {
    toggle_profile_file << " -maintain_toggle_profiles option is set to 0. Please set it to 1. " << endl;
    return;
  }
  for (int i = 0; i < getGateNum(); i++)
  {
    if (m_gates[i]->getIsClkTree() == true) continue;
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
  list < pair < double ,  vector <int> > >& collection = tree->get_collection();
  list < pair < double, vector < int > > > :: iterator it;
  for (it = collection.begin(); it != collection.end() ; it++)
  {
    vector <int>& unit = it->second;
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
    it->first = worst_slack;
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
     once = true;
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
    list < pair < double , vector <int> > > & collection = tree->get_collection();
    list < pair < double, vector <int> > > :: iterator it;
    for (it = collection.begin(); it != collection.end(); it++)
    {
        vector <int>& unit = it->second;
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
  if (exeOp == 24)
  {
    find_dynamic_slack_pins_subset(T);
    return;
  }
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


void PowerOpt::resetAllVisitedForGates()
{
  for (int i = 0; i < m_gates.size(); i ++)
  {
    Gate* gate = m_gates[i];
    gate->setVisited(false);
  }
}


void PowerOpt:: resetAllDeadToggles()
{
  for (int i = 0; i < m_gates.size(); i++)
  {
    Gate* gate = m_gates[i];
    gate->setDeadToggle(false);
  }
}

void PowerOpt::check_for_flop_toggles_fast(int cycle_num, int cycle_time, designTiming * T)
{
    cout << "Checking for toggles in cycle " << cycle_num << " and cycle time " << cycle_time << endl;
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
    }
    resetAllVisitedForGates();
    resetAllDeadToggles();
    check_for_dead_ends(cycle_num, toggled_gates_str);
    print_dead_end_gates(cycle_num);
    cout << "inserting" << endl;
    map<string, pair<int, pair< int, float > > > :: iterator it = toggled_sets_counts.find(toggled_gates_str);
    if (it != toggled_sets_counts.end())
    {
      (it->second.second.first)++;
      per_cycle_toggled_sets.push_back(make_pair (cycle_time, it));
    }
    else  // NEW GATESET
    {
      std::pair<std::map<string, pair<int, pair<int, float> > >::iterator , bool> ret;
      ret = toggled_sets_counts.insert(std::pair<string, std::pair<int, std::pair<int, float> > > (toggled_gates_str, std::make_pair(unique_toggled_set_id, std::make_pair (1, 10000.0))));
      assert(ret.second); // true if uniquely inserted
      per_cycle_toggled_sets.push_back(make_pair (cycle_time, ret.first));
      toggled_gate_vector.push_back(toggled_gates_vec);
      toggled_sets.push_back(toggled_gates_set);
      unique_toggle_gate_sets << unique_toggled_set_id << ":" << toggled_gates_str << endl;
      unique_toggled_set_id++;
    }

    //cout << "[TOG] count_not_toggled is " <<  count_not_toggled << endl;
    //cout << "[TOG] count_toggled is " <<  count_toggled << endl;
    //toggle_info_file << endl;
}


void PowerOpt::print_dead_end_gates(int cycle_num)
{



}

void PowerOpt::check_for_dead_ends(int cycle_num, string toggled_gates_str, vector<int>& nondeadend_toggled_gates_indices)
{

  vector<string> toggled_gates;
  int dead_gates_count = 0;
  int non_clk_tree_non_ff_toggled_gates_count = 0;
  tokenize(toggled_gates_str, ',', toggled_gates);
  cout << "CHECKING FOR DEAD ENDS and size of toggled set is " << toggled_gates.size() << endl;
  for (int i = 0 ; i < toggled_gates.size(); i++)
  {
    string gate_name = toggled_gates[i];
    Gate* gate = gate_name_dictionary[gate_name];
    if (gate->getFFFlag()) continue;
    if (gate->getIsClkTree() == false) non_clk_tree_non_ff_toggled_gates_count ++;
    bool dead_gate = gate->check_for_dead_toggle(cycle_num);
    if (dead_gate)
    {
      gate->setDeadToggle(true);
      //cout << gate->getName() << " is a dead end gate" << endl;
      gate->trace_back_dead_gate(dead_gates_count, cycle_num);
    }
  }

  for (int i =0; i < toggled_gates.size(); i++)
  {
    string gate_name = toggled_gates[i];
    Gate* gate = gate_name_dictionary[gate_name];
    if (!gate->isDeadToggle())
    {
      int id = gate->getId();
      nondeadend_toggled_gates_indices.push_back(id);
    }
  }
  cout << " Number of dead end gates : " << dead_gates_count << "/" << non_clk_tree_non_ff_toggled_gates_count << endl;
}

void PowerOpt::check_for_dead_ends(int cycle_num, string toggled_gates_str)
{

  vector<string> toggled_gates;
  int dead_gates_count = 0;
  int non_clk_tree_non_ff_toggled_gates_count = 0;
  tokenize(toggled_gates_str, ',', toggled_gates);
  cout << "CHECKING FOR DEAD ENDS and size of toggled set is " << toggled_gates.size() << endl;
  for (int i = 0 ; i < toggled_gates.size(); i++)
  {
    string gate_name = toggled_gates[i];
    Gate* gate = gate_name_dictionary[gate_name];
    if (gate->getFFFlag()) continue;
    if (gate->getIsClkTree() == false) non_clk_tree_non_ff_toggled_gates_count ++;
    bool dead_gate = gate->check_for_dead_toggle(cycle_num);
    if (dead_gate)
    {
      gate->setDeadToggle(true);
      //cout << gate->getName() << " is a dead end gate" << endl;
      gate->trace_back_dead_gate(dead_gates_count, cycle_num);
    }
  }

  cout << " Number of dead end gates : " << dead_gates_count << "/" << non_clk_tree_non_ff_toggled_gates_count << endl;
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
        //cout << gate_name << " -->          --> " << fanout_cells_str << endl;
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

}   //namespace POWEROPT
