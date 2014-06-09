#include <iostream>
#include <fstream>
#include <ctime>

#include "oaDesignDB.h"
#include "OAIO.h"
#include "PowerOpt.h"
#include "typedefs.h"
#include "Pad.h"

using namespace std;
using namespace oa;

//#define DEBUG

namespace POWEROPT {

void OAReader::fill_reg_cells_list(PowerOpt *po)
{
  ifstream reg_list_file;
  string reg_cells_file = po->getRegCellsFile();
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

  bool OAReader::readDesign(PowerOpt *po)
  {
	  ofstream &logFile = po->getLogFile();
	  bool keepLog = po->getKeepLog();

    fill_reg_cells_list(po);

    StrIntMap &gateNameIdMap = po->getGateNameIdMap();
    StrIntMap &padNameIdMap = po->getPadNameIdMap();
    StrIntMap &subnetNameIdMap = po->getSubnetNameIdMap();

		string strLibName = po->getLibName();
		string strCellName = po->getCellName();
		string strViewName = po->getViewName();

    try {
      oaDesignInit();
      oaLibDefList::openLibs();

      oaNativeNS	oaNs;
      oaDefNS		oaLefDefNs;
			oaViewType	*viewType = oaViewType::get(oacMaskLayout);

      oaScalarName      libName(oaNs,oaString(strLibName.c_str()));
      oaScalarName      cellName(oaNs,oaString(strCellName.c_str()));
      oaScalarName      viewName(oaNs,oaString(strViewName.c_str()));
      oaString	        libPath(strLibName.c_str());

      if (! oaLib::find(libName)) {
        if (oaLib::exists(libPath)) {
          oaLib::open(libName, libPath);
        } else {
          cout << "Can not find the library " << libPath << endl;
          //exit(1);
          return false;
        }
      }

      oaDesign * design = oaDesign::open(libName, cellName, viewName, viewType, 'r');
      if (design == NULL || design->getTopBlock() == NULL) {
        cout << " Error : " << endl
             << "fail to open design " << endl;
        //exit(-1);
        return false;
      }

      viewType = design->getViewType();
      oaString viewNameStr;
      viewType->getName(viewNameStr);
      cout<<"ViewType: "<<viewNameStr<<endl;

			Chip & chip = po->getChip();
      int rowW = 0, rowH = 0;
      if (design->getTech())
      {
      	int factor = design->getTech()->getDBUPerUU(design->getViewType());
        cout << "LEFDEFFactor: " << factor << endl;
				chip.setLefDefFactor(factor);

        oaIter<oaSiteDef> siteIter(design->getTech()->getSiteDefs());
        while(oaSiteDef * site = siteIter.getNext()) {
	        if( site->getSiteDefType() == oacCoreSiteDefType)
          {
            rowW = site->getWidth();
            rowH = site->getHeight();
            chip.setRowH(rowH);
            chip.setRowW(rowW);
            break;
          }
        }
      }
      else
      {
        cout<<"No technology info ..."<<endl;
        //exit(-1);
        return false;
      }

      oaBox bBox;
      oaBlock *block = design->getTopBlock();
      if (block == NULL)
      {
        cout<<"Error no block is obtained\n";
        //exit(-1);
        return false;
      }

			int l = INT_MAX, b = INT_MAX, r = 0, t = 0;
			if( (block->getRows()).getCount() > 0)
			{
				oaIter<oaRow> rowIter(block->getRows());
				while(oaRow * row = rowIter.getNext())
				{
					oaPoint orgPoint;
					row-> getOrigin (orgPoint);
					//not always right
//					assert(orgPoint.x() == 0 && orgPoint.y() >= 0);
					l = MIN(l, orgPoint.x());
					r = MAX(r, orgPoint.x() + row->getNumSites() * rowW);
					b = MIN(b, orgPoint.y());
					t = MAX(t, (orgPoint.y() + rowH));
//					cout << "l: " << l  << " b: " << b <<" r: " << r  << " t: " << t << endl;
//					cout <<"Row: W="<<rowW<<", H="<<rowH<<" orgin ["<<orgPoint.x()<<","<<orgPoint.y()<<"] "<<" site number: "<<row->getNumSites()<<endl;
//					getchar();
				}
			}
			else
			{
				block->getBBox(bBox);
				l = bBox.left();
				b = bBox.bottom();
				r = bBox.right();
				t = bBox.top();
				cout<<"No site information !"<<endl;
				getchar();
			}

			int siteLeft = l;
			int siteRight = r;
			int siteBottom = b;
			int siteTop = t;
			po->setSiteBox(l, b, r, t);
			int numW = (int)ceil((r-l)/(double)rowW);
			int numH = (int)ceil((t-b)/(double)rowH);
			chip.setRowW(rowW);
			chip.setRowH(rowH);
			chip.setSiteRowNum(numH);
			chip.setSiteColNum(numW);
			chip.initSiteOrient(numH);

			if((block->getRows()).getCount() > 0)
			{
				oaIter<oaRow> rowIter2(block->getRows());
				while(oaRow * row = rowIter2.getNext())
				{
					oaPoint orgPoint;
					row-> getOrigin (orgPoint);
					int y = (orgPoint.y() - b)/rowH;
					chip.setSiteOrient(y, row->getSiteOrient());
//					cout << "Orient of row " << y << " : " << row->getSiteOrient() << endl;
				}
			}

      block->getBBox(bBox);
      int chipLeft = bBox.left();
      int chipBottom = bBox.bottom();
      int chipRight = bBox.right();
      int chipTop = bBox.top();
			chip.setDim(bBox.left(), bBox.bottom(), bBox.right(), bBox.top());

      cout << "Chip Left: " << chipLeft  << "\tChip Bottom: " << chipBottom << endl;
      cout << "Chip Right: " << chipRight  << "\tChip Top: " << chipTop << endl;
      cout << "site Left: " << siteLeft  << "\tsite Right: " << siteRight << endl;
      cout << "site Bottom: " << siteBottom  << "\tsite Top: " << siteTop << endl;
      cout<<"rowW: "<<rowW<<" rowH: "<<rowH<<" numW: "<<numW<<" numH: "<<numH<<endl;

      int gateNum = (block->getInsts()).getCount() + (block->getTerms()).getCount();
      cout << "Num of Gates: " << gateNum << endl;


      //read in the gates
      int gateIndex = 0;
      int padIndex = 0;
      oaIter<oaInst> instIter(design->getTopBlock()->getInsts());
      while(oaInst * inst = instIter.getNext())
      {
        oaString gateName;
        oaString cellName;
        oaBox	bbox;

        inst->getName(oaLefDefNs, gateName);
//          cout << "Instance Name: " << gateName << endl;
        inst->getCellName(oaLefDefNs, cellName);
//          cout << "cell Name: " << cellName << endl;

//					cout<<"isTerm ()?"<<inst->isTerm ()<<endl;
//					cout<<"isInst () ?"<<inst->isInst () <<endl;
//					cout<<"  isBitInst ()  ?"<<inst->  isBitInst ()  <<endl;

        string strCellName = ((const oaChar*)(cellName));
        string strOrgCellName = strCellName;

        bool FFFlag = false;
        //oaString sub1 = "DF";
//        oaString sub2 = "LH";
//        oaString sub3 = "LN";
//        if (cellName.substr(sub1) != cellName.getLength())
//        {
//          cout << "Adding cell " << strCellName << " to ff list" << endl;
//        //							cout<<"cellName.getLength(): "<<cellName.getLength()<<endl;
//        //            	cout<<"cellName.substr('FF'): "<<cellName.substr(sub1)<<endl;
//        //              cout<<"flip-flop "<<gateName<<" found!"<<endl;
//          FFFlag = true;
//        }
        if(reg_cells_set.find(strCellName) != reg_cells_set.end()) {
          cout << "Adding cell " << strCellName << " to ff list" << endl;
          FFFlag = true;
        }


        /*if (!FFFlag)
        {
        	//be carefull!! do not consider clock buffers
        	//some clock buffers here for new jpeg
        	//now we want to consider clock buffers
          oaString sub = "clk";
          if (gateName.substr(sub) != gateName.getLength())
          {
      	  	continue;
          	cout<<"clock buffer (for JPEG) gateName: "<<gateName<<" cellName: "<<cellName<<endl;
          }
        }
       */
        /*LEON
        //be carefull!! this is error-prone
        //change the biased cell names to nominal names
        if (!FFFlag)
        {
//					cout<<"original cell name: "<<strCellName<<endl;
          size_t pos = strCellName.find("_");
          if (pos != string::npos)
          {
          	strCellName.erase(strCellName.begin()+pos, strCellName.end());
          	cellName = strCellName.c_str();
          }
//					cout<<"changed to cell name: "<<cellName<<endl;
        }
        */
        inst->getBBox(bbox);
        //           inst->getOrigin(point);
        //           cout << "Instance Type: " << inst->getType().getName()<<endl;
        //           cout << "Origin: ( " << point.x()<<" , "<<point.y()<<" ) ";
        //           cout << "\tBounding Box: " << "[{" << bbox.left() << "," << bbox.bottom()<< "} , {" << bbox.right() << "," << bbox.top() << "}]" << endl << endl;
        //           //            assert(point.x() == bbox.left() && point.y() == bbox.bottom());
        //           oaString orient = inst->getOrient().getName();
        //           cout<<"Orient: "<<orient<<endl;
        //           cout<<"-------------------------"<<endl;
        oaDesign * cv = inst->getMaster();
        int width, height;
				if(cv != NULL)
				{
					oaSnapBoundary * boundary = oaSnapBoundary::find(cv->getTopBlock());
					oaBox box;

					boundary -> getBBox(box);
					width = box.getWidth();
					height = box.getHeight();
#ifdef DEBUG
					cout << "Width: " << width << "\tHeight: " << height << endl;
#endif
					oaString macroName;
					cv->getCellName(oaLefDefNs, macroName);
				}

				// Get and print the origin of the instance.
				oaOrient orient = inst->getOrient();

				if( orient % 2 == 1)
				{
					int tmp = width;
					width = height;
					height = tmp;
				}
				oaPoint orgPoint;
				int x, y;
				inst->getOrigin(orgPoint);
//				if( orient == oacR0)
//				{
//					x = orgPoint.x() - chipLeft;
//					y = orgPoint.y() - chipBottom;
//				} else if( orient == oacR90) {
//					x = orgPoint.x() - width - chipLeft;
//					y = orgPoint.y() - chipBottom;
//				} else if( orient == oacR180) {
//					x = orgPoint.x() - width - chipLeft;
//					y = orgPoint.y() - height - chipBottom;
//				} else if( orient == oacR270) {
//					x = orgPoint.x() - chipLeft;
//					y = orgPoint.y() - height - chipBottom;
//				} else if( orient == oacMY) {
//					x = orgPoint.x() - width - chipLeft;
//					y = orgPoint.y() - chipBottom;
//				} else if( orient == oacMX) {
//					x = orgPoint.x() - chipLeft;
//					y = orgPoint.y() - height - chipBottom;
//				} else if( orient == oacMYR90) {
//					x = orgPoint.x() - width - chipLeft;
//					y = orgPoint.y() - height - chipBottom;
//				} else if( orient == oacMXR90) {
//					x = orgPoint.x() - chipLeft;
//					y = orgPoint.y() - chipBottom;
//				}
				if( orient == oacR0)
				{
					x = orgPoint.x();
					y = orgPoint.y();
				} else if( orient == oacR90) {
					x = orgPoint.x() - width;
					y = orgPoint.y();
				} else if( orient == oacR180) {
					x = orgPoint.x() - width;
					y = orgPoint.y() - height;
				} else if( orient == oacR270) {
					x = orgPoint.x();
					y = orgPoint.y() - height;
				} else if( orient == oacMY) {
					x = orgPoint.x() - width;
					y = orgPoint.y();
				} else if( orient == oacMX) {
					x = orgPoint.x();
					y = orgPoint.y() - height;
				} else if( orient == oacMYR90) {
					x = orgPoint.x() - width;
					y = orgPoint.y() - height;
				} else if( orient == oacMXR90) {
					x = orgPoint.x();
					y = orgPoint.y();
				}

				if (x < chipLeft || y < chipBottom || x+width > chipRight || y+height > chipTop)
				{
					cout<<"Error: instances out of chip!"<<endl;
					cout<<"instance name: "<<gateName<<" origin: ";
					po->printPoint(orgPoint.x(), orgPoint.y());
					cout<<endl;
					cout<<"orient: "<<orient<<endl;
					cout<<"chip lb point: ";
					po->printPoint(chipLeft, chipBottom);
					cout<<endl;

					//exit(0);
                    //return false;
				}
				int siteOrient = po->getSiteOrient((y-siteBottom)/rowH);
//					cout<<"instance name: "<<gateName<<" origin: ";
//					po->printPoint(orgPoint.x(), orgPoint.y());
//					cout<<endl;
//					cout<<"orient: "<<orient<<endl;
//					cout<<"chip lb point: ";
//					po->printPoint(chipLeft, chipBottom);
//					cout<<endl;
//					cout<<"After transformation instance origin: ";
//					po->printPoint(x, y);
//					cout<<endl;
//					cout<<"siteOrient: "<<siteOrient<<" inst orient: "<<orient<<endl;
				//no site orientation for ibm cases
//					assert(siteOrient%2 == orient%2);

        //new a gate and store it
        string tmpName = (const oaChar*)(gateName);
        gateNameIdMap[tmpName] = gateIndex;
        string tmpCellName = (const oaChar*)(cellName);
//          if (gateIndex == 4647)
//          {
//		        cout<<"Gate: id: "<<gateIndex<<" name: "<<tmpName<<" cell: "<<tmpCellName<<" orient: "<<orient<<" dim: ";
//		        po->printBox( x,  y, x+width, y+height);
//		        cout<<endl;
//		        getchar();
//          }
        Gate *g = new Gate(gateIndex++, FFFlag, tmpName, tmpCellName, CHANNEL_LENGTH, x, y, width, height);
        g->setOrgCellName(strOrgCellName);
        g->setOrgOrient(orient);
//					cout<<"width: "<<width<<" height: "<<height<<endl;
//					cout<<"bbox.left(): "<<bbox.left()<<" bbox.right(): "<<bbox.right()<<endl;
//					cout<<"bbox.bottom(): "<<bbox.bottom()<<" bbox.top(): "<< bbox.top()<<endl;
//          assert(width == bbox.right() - bbox.left());
//          assert(height == bbox.top() - bbox.bottom());
			  if (keepLog)
						logFile<<"Gate "<<tmpName<<" cell: "<<tmpCellName<<" in row: "<<y/rowH<<" ["<<x<<","<<y<<"], width: "<<width<<" height: "<<height<<" orient: "<<orient<<" siteOrient: "<<siteOrient<<endl;
        po->addGate(g);
      }

//      cout<<"Gate num: "<<po->getGateNum()<<endl;

      oaIter<oaTerm> termIter(design->getTopBlock()->getTerms());
      while(oaTerm * term = termIter.getNext())
      {
        oaString termName;
        term->getName(oaLefDefNs, termName);
//           cout << "pad Name: " << termName << endl;
        if (term->getNumBits() != 1)
        {
          cout<<term->getNumBits()<<" bits term found!"<<endl;
          //exit(-1);
          //return false;
        }
        if (term->getTermType() != 0 && term->getTermType() != 1)
        {
          cout<<term->getTermType().getName()<<" type term found!"<<endl;
//              exit(-1);
        }

//           cout<<"term type: "<<term->getTermType()<<endl;
//           cout<<"term type: "<<term->getTermType().getName()<<endl;

        oaIter<oaPin> pinIter(term-> getPins());
        oaPin * pin = pinIter.getNext();
        oaBox box;
        if (pin == NULL)
        {
					cout << "Warning: I/O pad " << termName << " has no physical geometry pin\n" << endl;
			//exit(-1);
            //return false;
        }
        else
        {
          oaIter<oaPinFig> pinfigIter(pin->getFigs());
          oaPinFig * pinfig = pinfigIter.getNext();
          if(pinfig == NULL)
          {
            cout << "Warning: I/O pad " << termName << " has no physical geometry pinfig\n" << endl;
            //exit(-1);
            //return false;
          }
          pinfig->getBBox(box);
//        	cout << "\tBounding Box of pad: " << "[{" << box.left() << "," << box.bottom()<< "} , {" << box.right() << "," << box.top() << "}]" << endl;
      	}

        char tmpName[2000];
        strcpy(tmpName, (termName));
        padNameIdMap[tmpName] = padIndex;
				PadType padType = InputOutput;
        if (term->getTermType() == 0)
        	padType = PrimiaryInput;
        else if (term->getTermType() == 1)
        	padType = PrimiaryOutput;
        Pad *pad = new Pad(padIndex++, (const oaChar*)(termName), padType, box.left(), box.bottom(), box.right(), box.top());
        po->addPad(pad);
      }
//      cout<<"Pad num: "<<po->getPadNum()<<endl;

      int netIndex = 0;
			int termIndex = 0;
      oaIter<oaNet> scalarNets(design->getTopBlock()->getNets(oacNetIterAll));
      while (oaNet *net = scalarNets.getNext())
      {
        oaString netName;
        net->getName(oaLefDefNs, netName);


        if (net->getSigType() == oacPowerSigType || net->getSigType() == oacGroundSigType)
          {
            continue;
        }

        //add the clock net
        /* LEON3 - clock signal consider
        if (net->getSigType() == oacClockSigType)
        {
	      	if (po->getOptClock())
	      	{
	          cout<<"clock net "<<netName<<" will be processed"<<endl;
	        }
	        else
	        {
	      	  continue;
	        }
	      }
          else if (net->getSigType() == oacPowerSigType || net->getSigType() == oacGroundSigType)
          {
            continue;
          }
	      else if (net->getSigType() != oacSignalSigType)
        {
//              cout << "Warning: non-sigal net: " <<netName<<": "<< net->getSigType().getName() <<" found."<< endl;
      	  continue;
        }
        */
        if( (net->getInstTerms()).getCount() == 0)
        {
//              cout << "0 term net: " << netName <<" found : ignore."<< endl;
    	    continue;
        }

//					cout << "============== net " << netName <<"=============="<< endl;
//					cout << "index: " << netIndex << endl;
//					cout<<"number of term instances: "<<net->getInstTerms().getCount()<<endl;
//					cout<<"number of terms: "<<net->getTerms().getCount()<<endl;

				Net *n = new Net(netIndex++, (const oaChar*)(netName));

				//here are the gates connected to the net
				oaIter<oaInstTerm> instTerms(net->getInstTerms());
				while (oaInstTerm *instTerm = instTerms.getNext())
				{
					oaInst  *instGate = instTerm->getInst();
					oaString instGateName;
					instGate->getName(oaLefDefNs, instGateName);

					oaString cellName;
					instGate->getCellName(oaLefDefNs, cellName);

					char tmpName[2000];
					strcpy(tmpName, (instGateName));

					if (gateNameIdMap.find(tmpName) == gateNameIdMap.end())
					{
						cout<<"Error: Cannot find gate "<<tmpName<<" in map"<<endl;
						getchar();
						continue;
					}
					int gateId = gateNameIdMap[tmpName];
					if(gateId >= po->getGateNum() || gateId < 0)
						printf("error gate id %d\n", gateId);
					Gate *g = po->getGate(gateId);
//						cout<<"Connected gate: "<<g->getName()<<endl;

					oaString termName;
					instTerm->getTermName(oaLefDefNs, termName);
					Terminal *term = new Terminal(termIndex++, (const oaChar*)(termName));
					term->setGate(g);
					term->addNet(n);

          oaTermTypeEnum terminalType = instTerm->getTerm()->getTermType();
        //  assert(terminalType == oacInputTermType || terminalType == oacOutputTermType);
          PinType pinType;
          if (terminalType == oacOutputTermType)
          {
          	pinType = OUTPUT;
//          	cout<<"Output pin: "<<termName<<endl;
          }
          else
          {
          	pinType = INPUT;
//          	cout<<"Input pin: "<<termName<<endl;
          }

					if (pinType == OUTPUT)
					{
			       term->setPinType(OUTPUT);
			       g->addFanoutTerminal(term);
			       n->addInputTerminal(term);
					}
					else
					{
			       term->setPinType(INPUT);
			       g->addFaninTerminal(term);
			       n->addOutputTerminal(term);
					}

           if (!instTerm->isBound())
           {
              cout << "Warning: InstTerm "<<termName<<" is unbounded" << endl;
              cout<<"insTerm isBound: "<<instTerm->isBound()<<endl;
              getchar();
           }

           term->addNet(n);
           po->addTerminal(term);
         }

         //here are the pads connected to the net
         oaIter<oaTerm> Terms(net->getTerms());
         while (oaTerm * term = Terms.getNext())
         {
           oaString termName;
           term->getName(oaLefDefNs, termName);
           char tmpName[120];
           strcpy(tmpName, (termName));
           if(padNameIdMap.find(tmpName) == padNameIdMap.end())
           {
             cout << "I/O pad " << tmpName <<" not found in map "<< endl;
             //exit(-1);
             //return false;
           }
           int padId = padNameIdMap[tmpName];
           Pad *pad = po->getPad(padId);
           pad->addNet(n);
           n->addPad(pad);
         }
         po->addNet(n);
         //add the net to all the connected gates
				int terminalNum = n->getTerminalNum();
        for (int k = 0; k < terminalNum; k ++)
        {
          Terminal *t = n->getTerminal(k);
          Gate *g = t->getGate();
          g->addNet(n);
        }
      }
    }
    catch (oaException &excp)
      {
        cout << "ERROR : " << (const char *)excp.getMsg() << endl;
        //exit(-1);
        //return false;
      }
    ///////////////////////for ibm cases, there are no source pins!!!!!!!
    //there construct the timing graph
    int netNum = po->getNetNum();
    cout<<"Net number: "<<netNum<<endl;
    int subnetIndex = 0;
    for (int i = 0; i < netNum; i ++)
    {
      Net *n = po->getNet(i);
      int padNum = n->getPadNum();
      int terminalNum = n->getTerminalNum();
//         cout<<"Net "<<i<<" Name: "<<n->getName()<<endl;
//         cout<<"Pad number: "<<padNum<<endl;
//         cout<<"Terminal number: "<<terminalNum<<endl;
      //consider the input and output pads

      //connect pads to cells
      if (padNum > 0)
      {
        for (int j = 0; j < padNum; j ++)
        {
          Pad *pad = n->getPad(j);
          string pName = pad->getName();

          for (int k = 0; k < terminalNum; k ++)
          {
            Terminal *t = n->getTerminal(k);
            string tName = t->getName();
            Gate *g = t->getGate();
            string gName = g->getName();
            if (pad->getType() == PrimiaryInput || pad->getType() == InputOutput)
            {
	          	assert(n->getInputTerminal() == NULL);
            	//be carefull!!!!!!!!!
            	assert(t->getPinType() == INPUT);
              string sName = pName + "-" + gName + "/" + tName;
            	if (t->getPinType() == OUTPUT)
            	{
            		cout<<"Error: input pad "<<pad->getName()<<" connects to output terminal "<<sName<<endl;
            		//exit(-1);
                    //return false;
            	}
		          subnetNameIdMap[sName] = subnetIndex;
              Subnet *subnet = new Subnet(subnetIndex++, sName, true, false);
              subnet->setInputPad(pad);
              subnet->setOutputTerminal(t);
              po->addSubnet(subnet);
              g->addInputSubnet(subnet);
              t->addSubnet(subnet);
              g->addFaninPad(pad);
              pad->addSubnet(subnet);
//                        cout<<"Pad "<<pad->getName()<<" => "<<t->getGate()->getName()<<"."<<t->getName()<<endl;
              if (!g->getFFFlag())
              {
                if (g->getGateType() == GATEUNKNOWN)
                {
                    g->setGateType(GATEPADPI);
                }
                else
                {
									//assert (g->getGateType() == GATEPADPI || g->getGateType() == GATEPADPO);
									if (g->getGateType() == GATEPADPO)
										g->setGateType(GATEPADPIO);
                }
              }
            }
            else
            {
            	assert(pad->getType() == PrimiaryOutput);
              string sName = gName + "/" + tName + "-" + pName;
            	//be carefull!!!!!!!!!
            	if (t->getPinType() == INPUT)
            	{
//                      		cout<<"Warning: output pad "<<pad->getName()<<" connects to input terminal "<<sName<<endl;
//                      		getchar();
            		continue;
            	}
		          subnetNameIdMap[sName] = subnetIndex;
              Subnet *subnet = new Subnet(subnetIndex++, sName, false, true);
              subnet->setOutputPad(pad);
              subnet->setInputTerminal(t);
              po->addSubnet(subnet);
              g->addOutputSubnet(subnet);
              t->addSubnet(subnet);
              pad->addSubnet(subnet);
//                        cout<<t->getGate()->getName()<<"."<<t->getName()<<" => Pad "<<pad->getName()<<" Gate Type: "<<g->getGateType()<<endl;
              assert(pad->getType() == PrimiaryOutput);
              g->addFanoutPad(pad);

              if (!g->getFFFlag())
              {
                if (g->getGateType() == GATEUNKNOWN)
                {
                    g->setGateType(GATEPADPO);
                }
                else
                {
									assert (g->getGateType() == GATEPADPI || g->getGateType() == GATEPADPO);
									if (g->getGateType() == GATEPADPI)
										g->setGateType(GATEPADPIO);
                }
              }
            }
          }
        }
      }
      else
      {
				if (n->getInputTerminal() == NULL)
				{
					cout<<"No source terminal!!! net Name: "<<n->getName()<<endl;
					n->makeFalseInputTerminal();
					//exit(0);
				}
      }
      //consider the signal nets between gates
      //cout<<n->getName()<<endl;

      //connects between cells
      assert(n->getTerminalNum() >= 1);
      Terminal *source = n->getInputTerminal();
      if (source)
      {
        assert(source->getPinType() == OUTPUT);
        string sTName = source->getName();
        Gate *sg = source->getGate();
        string sGName = sg->getName();
        for (int j = 1; j < n->getTerminalNum(); j ++)
        {
          Terminal *sink = n->getTerminal(j);
          assert(sink->getPinType() == INPUT);
          string tTName = sink->getName();
          Gate *tg = sink->getGate();
          string tGName = tg->getName();
          string sName = sGName + "/" + sTName + "-" + tGName + "/" + tTName;
          subnetNameIdMap[sName] = subnetIndex;
          Subnet *subnet = new Subnet(subnetIndex++, sName, false, false);
          subnet->setInputTerminal(source);
          subnet->setOutputTerminal(sink);
          po->addSubnet(subnet);
          sg->addOutputSubnet(subnet);
          tg->addInputSubnet(subnet);
          source->addSubnet(subnet);
          sink->addSubnet(subnet);
          //                     cout<<source->getGate()->getName()<<"."<<source->getName()<<" => "<<sink->getGate()->getName()<<"."<<sink->getName()<<endl;
          source->getGate()->addFanoutGate(sink->getGate());
          sink->getGate()->addFaninGate(source->getGate());
        }
      //                 ////getchar();
      }
    }

    //set the gates connected to FF's to GATEPO
    for (int i = 0; i < po->getGateNum(); i ++)
      {
        Gate *g = po->getGate(i);
        if (g->getFFFlag())
          {
            int faninNum = g->getFaninGateNum();
            for (int j = 0; j < faninNum; j ++)
            {
              Gate *fanin = g->getFaninGate(j);
              if (fanin->getFFFlag())
                  continue;
              fanin->setGateType(GATEPO);
            }
          }
      }

    //here we seek to disable the gates on paths between PI/POs and FF's
    IntList liveGates;
    for (int i = 0; i < po->getGateNum(); i ++)
    {
			Gate *g = po->getGate(i);
			if (g->getGateType() == GATEPADPI)
			{
				assert(!g->getFFFlag());
				if (g->getFaninGateNum() == 0)
				{
					if(g->getFaninTerminalNum() != 1)
					{
//						cout<<"more than one fanin terminals connects to PI"<<endl;
//						g->print();
					}
					g->setDisable(true);
					assert(g->getId() == i);
					liveGates.push_back(i);
				}
				else
				{
					assert(g->getFaninTerminalNum() > 1);
				}
			}
		}
    while(liveGates.size())
      {
//        cout<<"liveGates.size(): "<<liveGates.size()<<endl;
        int gId = liveGates.front();
        liveGates.pop_front();
        Gate *g = po->getGate(gId);
//        cout<<"gId: "<<gId<<" Gate Name: "<<g->getName()<<endl;
        int fanoutNum = g->getFanoutGateNum();
        for (int i = 0; i < fanoutNum; i++)
          {
            Gate *fanout = g->getFanoutGate(i);
            if (fanout->getFFFlag() || fanout->isDisabled())
            	continue;
            Terminal *t = fanout->getFaninConnectedTerm(gId);
            assert(t != NULL && fanout->hasTerminal(t));
            t->setDisable(true);
            if (fanout->allFaninTermDisabled())
            {
            	fanout->setDisable(true);
            	liveGates.push_back(fanout->getId());
//              cout<<"Fanout gate : "<<fanout->getName()<<" is disabled"<<endl;
            }
          }
      }
    assert(liveGates.size() == 0);
    liveGates.clear();
    for (int i = 0; i < po->getGateNum(); i ++)
    {
			Gate *g = po->getGate(i);
			if (g->getGateType() == GATEPADPO)
			{
				assert(!g->getFFFlag());
				if (g->getFanoutGateNum() == 0)
				{
					assert(g->getFanoutTerminalNum() == 1);
					g->setDisable(true);
					assert(g->getId() == i);
					liveGates.push_back(i);
				}
			}
		}
    while(liveGates.size())
      {
//        cout<<"liveGates.size(): "<<liveGates.size()<<endl;
        int gId = liveGates.front();
        liveGates.pop_front();
        Gate *g = po->getGate(gId);
//        cout<<"gId: "<<gId<<" Gate Name: "<<g->getName()<<endl;
        int faninNum = g->getFaninGateNum();
        for (int i = 0; i < faninNum; i++)
          {
            Gate *fanin = g->getFaninGate(i);
            if (fanin->getFFFlag() || fanin->isDisabled())
            	continue;
            if (fanin->allFanoutGateDisabled())
            {
            	fanin->setDisable(true);
            	liveGates.push_back(fanin->getId());
//              cout<<"Fanin gate : "<<fanin->getName()<<" is disabled"<<endl;
            }
          }
      }

    for (int i = 0; i < po->getGateNum(); i ++)
      {
				Gate *g = po->getGate(i);
			  if (keepLog)
			  {
					logFile<<"==== Gate "<<g->getId()<<", Name: "<<g->getName()<<"---"<<g->getCellName();
					if (g->isDisabled())
						logFile<<" Is Disabled ===="<<endl;
					else
						logFile<<" ===="<<endl;
					logFile<<"FaningateNum: "<<g->getFaninGateNum()<<endl;
					logFile<<"FaninTerminalNum: "<<g->getFaninTerminalNum()<<endl;
					logFile<<"FaninPadNum: "<<g->getFaninPadNum()<<endl;
					logFile<<"FanoutgateNum: "<<g->getFanoutGateNum()<<endl;
					logFile<<"FanoutTerminalNum: "<<g->getFanoutTerminalNum()<<endl;
					logFile<<"FanoutPadNum: "<<g->getFanoutPadNum()<<endl;

					logFile<<"----Fanin Terminals----"<<endl;
					for (int j = 0; j < g->getFaninTerminalNum(); j ++)
					 {
					   Terminal *t = g->getFaninTerminal(j);
					   logFile<<t->getName()<<endl;
					 }
					logFile<<"----Fanin Gates----"<<endl;
					for (int j = 0; j < g->getFaninGateNum(); j ++)
					 {
					   Gate *finG = g->getFaninGate(j);
					   logFile<<finG->getName()<<endl;
					 }
					logFile<<"----Fanin Pads----"<<endl;
					for (int j = 0; j < g->getFaninPadNum(); j ++)
					 {
					   Pad *p = g->getFaninPad(j);
					   logFile<<p->getName()<<endl;
					 }
					logFile<<"----Fanout Terminals----"<<endl;
					for (int j = 0; j < g->getFanoutTerminalNum(); j ++)
					 {
					   Terminal *t = g->getFanoutTerminal(j);
					   logFile<<t->getName()<<endl;
					 }
					logFile<<"----Fanout Gates----"<<endl;
					for (int j = 0; j < g->getFanoutGateNum(); j ++)
					 {
					   Gate *foutG = g->getFanoutGate(j);
					   logFile<<foutG->getName()<<endl;
					 }
					logFile<<"----Fanout Pads----"<<endl;
					for (int j = 0; j < g->getFanoutPadNum(); j ++)
					 {
					   Pad *p = g->getFanoutPad(j);
					   logFile<<p->getName()<<endl;
					 }
					logFile<<"========================"<<endl;
				}
//				if (g->getName() == "U6742")
//					//getchar();
//        assert(g->getFaninGateNum()+g->getFaninPadNum() <= g->getFaninTerminalNum());
//        assert(g->getFanoutGateNum()+g->getFanoutPadNum() >= g->getFanoutTerminalNum());
      }
  return true;
  }


  void OAReader::updateModelTest(PowerOpt *po)
  {
		try
		{
			oaNativeNS		  oaNs;
			oaDefNS		    	oaLefDefNs;
			oaViewType *    viewType = oaViewType::get(oacMaskLayout);

			string strLibName = po->getLibName();
			string strCellName = po->getCellName();
			string strViewName = po->getViewName();

      oaScalarName      libName(oaNs,oaString(strLibName.c_str()));
      oaScalarName      cellName(oaNs,oaString(strCellName.c_str()));
      oaScalarName      viewName(oaNs,oaString(strViewName.c_str()));
      oaString	        libPath(strLibName.c_str());

      if (! oaLib::find(libName)) {
        if (oaLib::exists(libPath)) {
          oaLib::open(libName, libPath);
        } else {
          cout << "Can not find the library " << libPath << endl;
          exit(1);
        }
      }

			oaDesign * design = oaDesign::open(libName, cellName, viewName, viewType, 'a');
	    if (design == NULL)
	    {
				cout << " Error : " << endl << "fail to open design " << endl;
				////getchar();
				exit(-1);
			}

			oaScalarName viewModelName(oaNs, oaString(po->getViewName().c_str()));
			char modelName[2000];

	    //write the results
	    for (int i = 0; i < po->getGateNum(); i ++)
	      {
	        Gate *g = po->getGate(i);
	        if (g->getFFFlag())
	          continue;
	        string gateName = g->getName();
	        string biasCellName = g->getBiasCellName();
//	        cout<<"Gate: "<<g->getName()<<" biasedCellName: "<<biasCellName<<endl;

					oaSimpleName device(oaLefDefNs, gateName.c_str());
					oaInst* inst = oaInst::find(design->getTopBlock(), device);
					if(inst == NULL)
					{
						//cout << "Warning: inst " << gateName << " not found\n";
						////getchar();
						continue;
					}
		    	oaScalarName cellModelName(oaNs, oaString(biasCellName.c_str()));
					if(! oaDesign::exists(libName, cellModelName, viewModelName))
					{
						cout<<"Warning: modelName: "<<biasCellName<<" does not exist"<<endl;
						string cellName = g->getCellName();
						cellModelName.init(oaNs, oaString(strCellName.c_str()));
						cout<<"changed to "<<cellName<<endl;
					}

					//inst->setMaster(libName, cellModelName, viewModelName);
					//*
					oaDesign * modelDesign = oaDesign::open(libName, cellModelName, viewModelName, viewType, 'r');
				  if (modelDesign == NULL)
				  {
						cout << "Warning: " << "fail to open the new cell model " << modelName << endl;
						////getchar();
						continue ;
					}
					inst->setMaster(modelDesign);
					modelDesign->close();
					//*/
      	}
			design -> save();
			design -> close();
		}
		catch (oaException &excp)
		{
			cout << "ERROR : " << (const char *)excp.getMsg() << endl;
			////getchar();
			exit(-1);
		}
	}

  void OAReader::updateModel(PowerOpt *po, bool swap)
  {
		try
		{
			oaNativeNS		  oaNs;
			oaDefNS		    	oaLefDefNs;
			oaViewType *    viewType = oaViewType::get(oacMaskLayout);

			string strLibName = po->getLibName();
			string strCellName = po->getCellName();
			string strViewName = po->getViewName();

      oaScalarName      libName(oaNs,oaString(strLibName.c_str()));
      oaScalarName      cellName(oaNs,oaString(strCellName.c_str()));
      oaScalarName      viewName(oaNs,oaString(strViewName.c_str()));
      oaString	        libPath(strLibName.c_str());

      if (! oaLib::find(libName)) {
        if (oaLib::exists(libPath)) {
          oaLib::open(libName, libPath);
        } else {
          cout << "Can not find the library " << libPath << endl;
          exit(1);
        }
      }

			oaDesign * design = oaDesign::open(libName, cellName, viewName, viewType, 'a');
	    if (design == NULL)
	    {
				cout << " Error : " << endl << "fail to open design " << endl;
				exit(-1) ;
			}

			oaScalarName viewModelName(oaNs, oaString(po->getViewName().c_str()));
			char modelName[2000];

	    //write the results
	    for (int i = 0; i < po->getGateNum(); i ++)
	      {
	        Gate *g = po->getGate(i);
	        //if (g->getFFFlag())
	        //  continue;
	        if (swap && !g->isSwapped())
	        	continue;

	        string gateName = g->getName();
	        string biasCellName = g->getBiasCellName();
//	        cout<<"Gate: "<<g->getName()<<" biasedCellName: "<<biasCellName<<endl;

					oaSimpleName device(oaLefDefNs, gateName.c_str());
					oaInst* inst = oaInst::find(design->getTopBlock(), device);
					if(inst == NULL)
					{
						//cout << "Warning: inst " << gateName << " not found\n";
						////getchar();
						continue;
					}
		    	oaScalarName cellModelName(oaNs, oaString(biasCellName.c_str()));
					if(! oaDesign::exists(libName, cellModelName, viewModelName))
					{
						cout<<"Warning: modelName: "<<biasCellName<<" does not exist"<<endl;
						string cellName = g->getCellName();
						cellModelName.init(oaNs, oaString(strCellName.c_str()));
						cout<<"changed to "<<cellName<<endl;
					}

					//inst->setMaster(libName, cellModelName, viewModelName);
					//*
					oaDesign * modelDesign = oaDesign::open(libName, cellModelName, viewModelName, viewType, 'r');
				  if (modelDesign == NULL)
				  {
						cout << " Warning: " << "fail to open the new cell model " << modelName << endl;
						////getchar();
						continue ;
					}
					inst->setMaster(modelDesign);
					modelDesign->close();
					//*/
      	}
			design -> save();
			design -> close();
		}
		catch (oaException &excp)
		{
			cout << "ERROR : " << (const char *)excp.getMsg() << endl;
			exit(-1);
		}
	}

//any bug???
	void OAReader::updateCoord(PowerOpt *po, bool all)
	{
		ofstream &logFile = po->getLogFile();
		bool keepLog = po->getKeepLog();

		try
		{
			oaNativeNS		  oaNs;
			oaDefNS		    	oaLefDefNs;
			oaViewType	    *viewType = oaViewType::get(oacMaskLayout);

			string strLibName = po->getLibName();
			string strCellName = po->getCellName();
			string strViewName = po->getViewName();

      oaScalarName      libName(oaNs,oaString(strLibName.c_str()));
      oaScalarName      cellName(oaNs,oaString(strCellName.c_str()));
      oaScalarName      viewName(oaNs,oaString(strViewName.c_str()));

			oaDesign * outdesign = oaDesign::open(libName, cellName, viewName, viewType, 'a');
			if (outdesign == NULL)
			{
				cout << " Error : " << endl	<< "fail to open output design " << endl;
				exit(-1);
			}

			oaBox box;
			oaBlock * block = outdesign-> getTopBlock();
			block-> getBBox( box);
	#ifdef DEBUG
			cout << "Chip Width: " << box.getWidth()  << "\tChip Height: " << box.getHeight() << endl;
	#endif

			int gateId = 0;
      StrIntMap &gateNameIdMap = po->getGateNameIdMap();
			oaIter<oaInst> instIter(block->getInsts());
			while(oaInst * inst = instIter.getNext())
			{
				oaString cellName;
				inst->getName(oaLefDefNs, cellName);
				if (gateNameIdMap.find((const char *)cellName) == gateNameIdMap.end())
				{
//					cout<<"Warning: Cannot find gate "<<(const char *)cellName<<" in map"<<endl;
					continue;
				}
				gateId = gateNameIdMap[(const char *)cellName];
	#ifdef DEBUG
				cout << "inst Name: " << cellName << "Id: "<<gateId <<endl;
	#endif
				Gate *g = po->getGate(gateId);
				if (cellName != g->getName().c_str())
					cout << "Warning: mismatch cell names: " << cellName << "\t" << g->getName() << endl;
				if (!all && !g->isSwapped())
					continue;
				if (keepLog)
					logFile<<"=========== Update coords of the swapped gates ========="<<endl;
				int width = g->getWidth();
				int height = g->getHeight();
				int lx = g->getLX();
				int by = g->getBY();

				//cout<<"BBox of gate: ["<<lx<<","<<by<<"]-["<<lx+width<<","<<by+height<<"]"<<endl;

				int row = (int)((by-po->getSiteBottom()) / po->getRowHeight());
				//cout << "Orient of row " << row << " : " << po->getSiteOrient(row);

				oaOrient orient;
				if (g->isRollBack() || all)
				{
					orient = ((oaOrientEnum)(g->getOrgOrient()));
					g->setRollBack(false);
					if (keepLog)
						logFile<<"roll back gate "<<g->getName()<<" ["<<lx<<","<<by<<"] width: "<<width<<" height: "<<height<<" original orient "<<inst->getOrient()<<endl;
				}
				else
				{
					orient = ((oaOrientEnum)(po->getSiteOrient(row)));
					if (keepLog)
						logFile<<"Gate "<<g->getName()<<" ["<<lx<<","<<by<<"] width: "<<width<<" height: "<<height<<" original orient "<<inst->getOrient()<<endl;
				}
				// set the origin of the instance.
		    oaPoint        orgPoint;
				if( orient == oacR0)
				{
			    orgPoint.x() = (oaInt4)(lx);
					orgPoint.y() = (oaInt4)(by);
				} else if( orient == oacR90) {
			    orgPoint.x() = (oaInt4)(lx + width);
					orgPoint.y() = (oaInt4)(by);
				} else if( orient == oacR180) {
			    orgPoint.x() = (oaInt4)(lx + width);
					orgPoint.y() = (oaInt4)(by + height);
				} else if( orient == oacR270) {
			    orgPoint.x() = (oaInt4)(lx);
					orgPoint.y() = (oaInt4)(by + height);
				} else if( orient == oacMY) {
			    orgPoint.x() = (oaInt4)(lx + width);
					orgPoint.y() = (oaInt4)(by);
				} else if( orient == oacMYR90) {
			    orgPoint.x() = (oaInt4)(lx + width);
					orgPoint.y() = (oaInt4)(by + height);
				} else if( orient == oacMX) {
			    orgPoint.x() = (oaInt4)(lx);
					orgPoint.y() = (oaInt4)(by + height);
				} else if( orient == oacMXR90) {
			    orgPoint.x() = (oaInt4)(lx);
					orgPoint.y() = (oaInt4)(by);
				}
				else
				{
					cout<<"Unknown inst orient"<<endl;
					exit(-1);
				}

				inst->setOrigin(orgPoint);
				inst->setPlacementStatus(oaPlacementStatus(oacPlacedPlacementStatus));
				inst->setOrient(orient);
				inst->getOrigin(orgPoint);
				if (keepLog)
					logFile<<"Computed origin ["<<orgPoint.x()<<","<<orgPoint.y()<<"] new orient "<<inst->getOrient()<<endl;
			}

			outdesign -> save();
			outdesign -> close();
		}
		catch (oaException &excp)
		{
			cout << "ERROR in output: " << (const char *)excp.getMsg() << endl;
			exit(0);
		}

		cout << "output finished" << endl;
	}

}
