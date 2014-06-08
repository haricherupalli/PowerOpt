/* Author: Seokhyeong Kang
 * PowerOpt main.v
 * Date: April. 21, 2010
 */
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdio.h>
#include <iomanip>
#include <cfloat>
#include <unistd.h>

#include "macros.h"
#include "GlobalFun.h"
#include "Gate.h"
#include "Grid.h"
#include "Path.h"
#include "OAIO.h"
#include "PowerOpt.h"
#include "analyzeTiming.h"
#include "Timer.h"

using namespace POWEROPT;
using namespace std;

void print_usage()
{
    cout << "PowerOpt -env [environment file] -f [command file]" << endl;
    cout << "[ -h ] -- print this message" << endl;
}

/*
void wait ( int seconds )
{
    clock_t endwait;
    endwait = clock () + seconds * CLOCKS_PER_SEC ;
    while (clock() < endwait) {}
}
*/

int main(int argc, char *argv[])
{
    char    envFile[1000];
    char    cmdFile[1000];
    bool    envFileExist = false;
    bool    cmdFileExist = false;

    //char    hostname[128];
    //gethostname(hostname, sizeof hostname);
    //printf ("My hostname: %s\n", hostname);

    for(int i = 1; i < argc; ++ i)
    {
        if ( strcmp(argv[i], "-env") == 0 )
        {
            sscanf(argv[++i], "%s", &envFile);
            envFileExist = true;
        }
        else if ( strcmp(argv[i], "-f") == 0 )
        {
            sscanf(argv[++i], "%s", &cmdFile);
            cmdFileExist = true;
        }
        else if ( strcmp(argv[i], "-h") == 0 )
        {
            print_usage();
            exit(0);
        }
    }

    if (!envFileExist) {
        cout << "Error: environment file should be provided!!"  << endl;
        print_usage();
        exit(0);
    }
    if (!cmdFileExist) {
        cout << "Error: command file should be provided!!"  << endl;
        print_usage();
        exit(0);
    }
    string envFileStr = envFile;
    string cmdFileStr = cmdFile;

    OAReader reader;
    PowerOpt po;

    po.readEnvFile(envFileStr);
    po.readCmdFile(cmdFileStr);

    if (po.getOaGenFlag() != 0) {
        cout << "[PowerOpt] OA DB generation ... " << endl;
        po.makeOA();
        cout << "[PowerOpt] OA DB generation ... done." << endl;
    }

    cout << "[PowerOpt] Design read ..." << endl;
    if (!reader.readDesign(&po)) {
        cout << "Error in Design read (OA)" << endl;
        exit(0);
    }
    cout << "[PowerOpt] Design read ... done " << endl;

    cout << "[PowerOpt] PrimeTime server excecution ... " << endl;
    po.exePTServer(false);
    cout << "[PowerOpt] PrimeTime server excecution ... done " << endl;

    if (po.getMmmcOn()) {
        cout << "[PowerOpt] PrimeTime server for MMMC excecution ... " << endl;
        po.exeMMMC();
        cout << "[PowerOpt] PrimeTime server for MMMC excecution ... done " << endl;
    }

    designTiming T;
    cout<<"[PowerOpt] PrimeTime server contact ... "<<endl;
    int pt_trial_cnt = 0;
    while (true) {
        po.wait (5);
        T.initializeServerContact(po.getPtClientTcl());
        if (T.checkPT()) break;
        if (pt_trial_cnt > 500) {
            cout << "Error: cannot access to PrimeTime!" << endl;
            exit(0);
        }
    }
    cout<<"[PowerOpt] PrimeTime server contact ... done "<<endl;

    cout << "[PowerOpt] Library Cell Read ... "<<endl;
    po.findLeakage(&T);
    if (!po.getUseGT()) po.readLibCells(&T);
    po.updateLibCells();
    cout << "[PowerOpt] Library Cell Read done... "<<endl;

    int gateCount = po.getGateNum();

    ofstream fout;
    fout.open(po.getReportFile().c_str());

    po.initSTA(&T);

    double worstSlack = po.getInitWNS();
    double worstSlackMin = po.getInitWNSMin();
    double leakage = po.getPtpxOff() ? po.getLeakPower() : T.getLeakPower();
    double totalPower = po.getPtpxOff() ? 0 : T.getTotalPower();

    po.setInitLeak (leakage);
    po.setCurLeak (leakage);
    po.setInitPower (totalPower);

    fout << "# Gate_count : "   << gateCount << endl;
    fout << "# Initial_WNS : "  << worstSlack << endl;
    fout << "# Initial_leakage : "  << scientific << leakage << endl;
    fout << "# Initial_power : "  << scientific << totalPower << endl;
    cout << "[PowerOpt] Initial_WNS : "  << worstSlack << endl;
    cout << "[PowerOpt] Initial_leakage : "  << scientific << leakage << endl;
    if (!po.getPtpxOff()) cout << "[PowerOpt] Initial_power : "  << scientific << totalPower << endl;
    T.setPtcnt(0);

    bool initFileExist = false;
    string initFile = "init.tcl";
    if (initFileExist) {
        T.readTcl(initFile);
        T.putCommand("current_instance");
        po.updateCellLib(&T);
    }

    time_t t1, t2, t3, t4;
    t1  = time(NULL);

    // set cell-base name
    po.setBaseName();
    //cout<<"[PowerOpt] Leakage pre-computation "<<endl;

    if (po.getLeakPT()) po.findLeakagePT(&T);
    else po.findLeakage(&T);

    bool rptPost = false;

    po.setDontTouch();
    t2  = time(NULL);

    if ( po.getExeOp() == 0 ) {     // TEST
        cout<<"=======PrimeTime Evaluation======== "<<endl;

        //po.testRunTime();
        po.testSlack();
        t3  = time(NULL);
        fout << "Optimization Time: " << t3 - t2 << endl;
    }

    fout << "# PT transaction (pre): " << T.getPtcnt() << endl;
    if ( po.getExeOp() == 1 || po.getExeOp() == 3 ) {
        po.setCurOp(1);
        // Leakage Optimization
        T.setPtcnt(0);
        cout<<"[PowerOpt] Leakage optimization "<<endl;
        fout<<"[PowerOpt] Leakage optimization "<<endl;
        if (po.getSwapstep() < 2 ) po.reduceLeak(&T);
        else po.reduceLeakMultiSwap(&T);
        t3  = time(NULL);
        fout << "# PT transaction (opt): " << T.getPtcnt() << endl;

        fout << "# swap(trial): " << po.getSwaptry() << endl;
        fout << "# swap(accepted): " << po.getSwapcnt() << endl;
        double accept_rate = (double)po.getSwapcnt()/(double)po.getSwaptry()
                             *(double)100;
        fout << "# accept_rate: "  << fixed << accept_rate << endl;
        fout << "# Run_Time for leakage optimization: " << t3 - t1 << endl;
        fout << "PowerCompute Time: " << t2 - t1 << endl;
        fout << "Optimization Time: " << t3 - t2 << endl;
        rptPost = true;
    }
    if ( po.getExeOp() == 2 || po.getExeOp() == 3 ) {
        // Timing Optimization
        cout<<"[PowerOpt] Timing optimization ..."<<endl;
        fout<<"[PowerOpt] Timing optimization "<<endl;
        po.setCurOp(2);

        worstSlack = T.getWorstSlack(po.getClockName());
        leakage = po.getPtpxOff() ? po.getLeakPower() : T.getLeakPower() ;
        po.setInitWNS (worstSlack);
        fout << "# WNS before timing optimization :"  << fixed << worstSlack << endl;
        cout << "[PowerOpt] WNS before timing optimization :"  << fixed << worstSlack << endl;
        fout << "# Leakage before timing optimization :" << leakage << endl;
        cout << "[PowerOpt] Leakage before timing optimization :"  << leakage << endl;

        t2  = time(NULL);
        T.setPtcnt(0);

        //if (po.getSensFunc() == 2) po.findLeakagePT(&T);
        po.optTiming(&T);
        t3  = time(NULL);
        fout << "# #PT_transaction(opt): " << T.getPtcnt() << endl;
        worstSlack = T.getWorstSlack(po.getClockName());
        int upsize_num = po.getSwapcnt();
        fout << "# #swap: " << upsize_num << endl;
        fout << "# Final_WNS: "  << fixed << worstSlack << endl;
        cout << "[PowerOpt] Final_WNS: "  << worstSlack << endl;
        /*
        double fin_Leak = T.getLeakPower();
        fout << "# Final_leakage: "  << scientific << fin_Leak << endl;
        cout << "[PowerOpt] Final_leakage: "  << scientific << fin_Leak << endl;
        */
        fout << "# Run_Time : " << t3 - t2 << endl;
        cout<<"[PowerOpt] Timing optimization ... done"<<endl;
        rptPost = true;
    }
    if ( po.getExeOp() == 4 ) {      // fix MaxTr
        cout<<"[PowerOpt] Check Max Transition Violation" <<endl;
        int maxTrCnt = po.checkMaxTr(&T);
        cout<<"[PowerOpt] MaxTr Violation: " << maxTrCnt << endl;
        fout<<"# MaxTr Violation (initial): " << maxTrCnt << endl;
        cout<<"[PowerOpt] Fix Max Transition Violation" <<endl;
        maxTrCnt = po.fixMaxTr(&T);
        cout<<"[PowerOpt] MaxTr Violation: " << maxTrCnt << endl;
        fout<<"# MaxTr Violation (final): " << maxTrCnt << endl;
        //fout<<"# Final_WNS: "  << fixed << worstSlack << endl;
        //cout<<"[PowerOpt] Final_WNS: "  << worstSlack << endl;
        rptPost = true;
    }

    if ( po.getExeOp() == 5 || po.getExeOp() == 14) {      // Error Rate Calculation (5) WITH TIMING VARIATION (14)

        // if this is a variation-aware analysis, initialize the delay variation of each gate
        if ( po.getExeOp() == 14 ) {
          po.readStdDevMap();
          //po.initVariations();
          cout<<"[PowerOpt] Variation-Aware Error Rate Calculation "<<endl;
        } else {
          cout<<"[PowerOpt] Error Rate Calculation "<<endl;
        }

        t2  = time(NULL);
        po.parseVCDALL(&T);
        t4  = time(NULL);
        if ( po.getExeOp() == 14 ) {
            po.calER_SSTA(&T);
        } else {
            po.calER(&T);
        }
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        fout << " parseVCDALL_Time : " << t4 - t2 << endl;
        fout << " calER_Time : " << t3 - t4 << endl;
        po.exitPT();
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 6 ) {      // optimize for target error rate
        cout<<"[PowerOpt] Optimization for target error rate "<<endl;
        t2  = time(NULL);
        //po.readLibCellsFile();
        po.parseVCDALL(&T);
        cout << "[PowerOpt] OptimizePaths Procedure" << endl;
        po.optimizeTargER(&T);
        totalPower = po.getPtpxOff() ? 0 : T.getTotalPower();
        cout << "[PowerOpt] TotalPower: " << totalPower << endl;
        cout << "[PowerOpt] ReducePower Procedure" << endl;
        po.reducePower(&T);
        totalPower = po.getPtpxOff() ? 0 : T.getTotalPower();
        cout << "[PowerOpt] TotalPower: " << totalPower << endl;
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        T.makeEcoTcl("swaplist.tcl", "eco.tcl");
        po.exitPT();
        cout<<"[PowerOpt] ECO Placement & Route ... "<<endl;
        po.exeSOCE();
        cout<<"[PowerOpt] ECO Placement & Route ... done "<<endl;
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 7 ) {      // slack optimizer
        cout<<"[PowerOpt] SlackOptimization "<<endl;
        t2  = time(NULL);
        //po.readLibCellsFile();
        po.parseVCDALL(&T);
        cout << "[PowerOpt] OptimizePaths Procedure" << endl;
        po.optimizeTargER(&T);
        totalPower = po.getPtpxOff() ? 0 : T.getTotalPower();
        cout << "[PowerOpt] TotalPower: " << totalPower << endl;
        cout << "[PowerOpt] ReducePower Procedure" << endl;
        po.reducePowerSO(&T);
        totalPower = po.getPtpxOff() ? 0 : T.getTotalPower();
        cout << "[PowerOpt] TotalPower: " << totalPower << endl;
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        T.makeEcoTcl("swaplist.tcl", "eco.tcl");
        po.exitPT();
        cout<<"[PowerOpt] ECO Placement & Route ... "<<endl;
        po.exeSOCE();
        cout<<"[PowerOpt] ECO Placement & Route ... done "<<endl;
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 8 ) {      // report FF toggle rate
        cout<<"[PowerOpt] Report Toggle Rate "<<endl;
        t2  = time(NULL);
        //po.readLibCellsFile();
        po.parseVCDALL(&T);
        po.reportToggleFF(&T);
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        po.exitPT();
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 9 ) {      // LSMC - not used
        int itr = 0;
        double kick_val = 0.01;
        double kick_max = 0.1;
        double k_leakage, k_worstSlack;
        double r_leakage, r_worstSlack;
        double best_leak = po.getInitLeak();
        int down_cnt, up_cnt;
        int deny_cnt = 0;
        int deny_max = 10;
        po.setBestLib(&T);
        while (true) {
            fout << "===== iteration #" << itr << " ====" << endl;
            //KICK MOVE
            down_cnt = po.kick_move(&T, kick_val);
            k_worstSlack = T.getWorstSlack(po.getClockName());
            k_leakage = T.getLeakPower();
            fout << "_D_" << itr << "_0_ downsize : " << kick_val*100 << endl;
            fout << "_D_" << itr << "_1_ downcnt : " << down_cnt << endl;
            fout << "_D_" << itr << "_2_ Pwr_after_kick : " << k_leakage << endl;
            fout << "_D_" << itr << "_3_ WNS_after_kick : " << k_worstSlack << endl;
            //Timing Recovery
            po.optTiming(&T);
            up_cnt = po.getSwapcnt();
            r_worstSlack = T.getWorstSlack(po.getClockName());
            r_leakage = T.getLeakPower();
            fout << "_D_" << itr << "_4_ upcnt : " << up_cnt << endl;
            fout << "_D_" << itr << "_5_ Pwr_after_recovery : " << r_leakage << endl;
            fout << "_D_" << itr << "_6_ WNS_after_recovery : " << r_worstSlack << endl;

            if (r_leakage < best_leak) {
                po.setBestLib(&T);
                best_leak = r_leakage;
                deny_cnt = 0;
                kick_val = 0.01;
                fout << "_D_" << itr << "_7_ ACCEPT_DENY : A" << endl;
                fout << "_D_" << itr << "_8_ BEST_LEAKAGE :" << best_leak << endl;
            }
            else {
                deny_cnt ++;
                po.restoreLib(&T);
                fout << "_D_" << itr << "_7_ ACCEPT_DENY : D" << endl;
                fout << "_D_" << itr << "_8_ BEST_LEAKAGE :" << best_leak << endl;
                if (deny_cnt == deny_max) {
                    kick_val = kick_val * 2;
                    deny_cnt = 0;
                }
                if (kick_val > kick_max) break;
            }
            itr++;
        }
    }
    if ( po.getExeOp() == 10 ) {      // Netlist partitioning
        cout<<"========Netlist partitioning======= "<<endl;
        t2  = time(NULL);
        po.netPartition(&T);
        t3  = time(NULL);
        fout << "_D_9_ Run_Time : " << t3 - t1 << endl;
    }
    if ( po.getExeOp() == 11 ) {      // Netlist partitioning with PT
        cout<<"========Netlist partitioning======= "<<endl;
        t2  = time(NULL);
        po.netPartitionPT(&T);
        t3  = time(NULL);
        fout << "_D_9_ Run_Time : " << t3 - t1 << endl;
    }
    if ( po.getExeOp() == 12 ) {      // check design
        cout<<"========Check design========" <<endl;
        po.checkMaxTr(&T);
    }

    if ( po.getExeOp() == 13 ) {      // Print Dynamic Slack Distribution
        cout<<"[PowerOpt] Print Dynamic Slack Distribution "<<endl;
        t2  = time(NULL);
        po.parseVCDALL(&T);
        t4  = time(NULL);
        po.printSlackDist(&T);
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        fout << " parseVCDALL_Time : " << t4 - t2 << endl;
        fout << " printSlackDist_Time : " << t3 - t4 << endl;
        po.exitPT();
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 15 ) {      // Print Dynamic Slack Distribution
        cout<<"[PowerOpt] Print Dynamic Slack Distribution "<<endl;
        t2  = time(NULL);
        //po.parsePtReport();
        po.readFlopWorstSlacks();
        po.read_modules_of_interest();
        // TESTCODE
        po.parseVCDALL_mode_15(&T);
        //po.check_for_toggles();
        t4  = time(NULL);
        //po.printSlackDist(&T);
        t3  = time(NULL);
        fout << " Run_Time : " << t3 - t1 << endl;
        fout << " parseVCDALL_Time : " << t4 - t2 << endl;
        fout << " printSlackDist_Time : " << t3 - t4 << endl;
        po.exitPT();
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    if ( po.getExeOp() == 16 ) {      //  read unique non-toggled gate sets and compute the slack
        cout<<"[PowerOpt] Reading in the gate sets"<<endl;
        t2  = time(NULL);
        po.readFlopWorstSlacks();
        po.read_unt_dump_file(); // unt = unique non toggled (gates)
        t4  = time(NULL);
        po.find_dynamic_slack_2(&T);
        //po.printSlackDist(&T);
        t3  = time(NULL);
        po.exitPT();
        fout.close();
        cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
        return 0;
    }

    // SOCEncounter
    //if (po.getSwapOp() == 2 && po.getExeOp() != 0 )
    //    po.setSOCEFlag(true);

    T.writeChange("swaplist.tcl");
    if (po.getSOCEFlag()) {
        T.makeEcoTcl("swaplist.tcl", "eco.tcl");
        cout<<"[PowerOpt] ECO Placement & Route ... "<<endl;
        po.exeSOCE();
        cout<<"[PowerOpt] ECO Placement & Route ... done "<<endl;
    } else {
        T.makeEcoTcl("swaplist.tcl", "eco.tcl");
        cout<<"[PowerOpt] Write Output Netlist and DEF ... "<<endl;
        //T.writeVerilog(po.getVerilogOut());
        po.updateBiasCell();
        reader.updateModel(&po, true);
        po.writeOut(&T);
    }
    po.exitPT();
    if (rptPost) {
        cout << "[PowerOpt] PrimeTime server excecution to report results " << endl;
        po.exePTServer(true);
        cout<<"[PowerOpt] PrimeTime server contact ... "<<endl;
        while (true) {
            T.initializeServerContact(po.getPtClientTcl());
            po.wait (5);
            if (T.checkPT()) break;
        }
        cout<<"[PowerOpt] PrimeTime server contact ... done "<<endl;
        worstSlack = T.getWorstSlack(po.getClockName());
        fout << "# Final_WNS: "  << fixed << worstSlack << endl;
        cout << "[PowerOpt] Final_WNS: "  << fixed << worstSlack << endl;
        double fin_Leak = po.getPtpxOff() ? po.getLeakPower() : T.getLeakPower();
        double fin_Pwr = po.getPtpxOff() ? 0 : T.getTotalPower();
        fout << "# Final_leakage: "  << scientific << fin_Leak << endl;
        if (!po.getPtpxOff()) fout << "# Final_power: "  << scientific << fin_Pwr << endl;
        cout << "[PowerOpt] Final_leakage: "  << scientific << fin_Leak << endl;
        if (!po.getPtpxOff()) cout << "[PowerOpt] Final_power: "  << scientific << fin_Pwr << endl;
        double imp_Leak = (fin_Leak - po.getInitLeak())*100/po.getInitLeak();
        double imp_Power = (fin_Pwr - po.getInitPower())*100/po.getInitPower();
        fout << "# leakage_change: " << fixed << imp_Leak << endl;
        if (!po.getPtpxOff()) fout << "# power_change: " << fixed << imp_Power << endl;
    }
    fout.close();
    T.Exit();
    cout<<"[PowerOpt] Ending PowerOpt ... " << endl;
    return 0;
}

