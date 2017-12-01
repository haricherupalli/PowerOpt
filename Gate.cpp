#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <limits.h>
#include <bitset>
#include <string>
#include <stdlib.h>

#include "Globals.h"
#include "Gate.h"
#include "Terminal.h"
#include "Path.h"
#include "Pad.h"
#include "PowerOpt.h"


using namespace std;


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


namespace POWEROPT {

ofstream Gate::gate_debug_file;

int Gate::max_toggle_profile_size = 0;

  void  Gate::openFiles(string outDir)
  {
    string gate_debug_file_name = outDir+"/PowerOpt/gate_debug_file";
    gate_debug_file.open(gate_debug_file_name.c_str(), std::ofstream::out);
  }

	bool Gate::hasTerminal(Terminal *t)
	{
		if (t == NULL)
			return false;
//		cout<<"Gate: "<<endl;
//		print();
//		cout<<"terminal: "<<endl;
//		t->print();
		string tName = t->getName();
    for (int i = 0; i < faninTerms.size(); i ++)
      {
        if (faninTerms[i]->getName() == tName)
          {
            return true;
          }
      }

    for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        if (fanoutTerms[i]->getName() == tName)
          {
            return true;
          }
      }
      return false;
	 }

	void Gate::rollBack()
	{
		assert(swapped);
		rolledBack = true;

		centerX = oldCenterX;
		centerY = oldCenterY;
		lx = oldLX;
		by = oldBY;
		biasCellName = oldBiasCellName;
		rolledBack = true;
		pGrid->removeGate(this->id);
		pGrid = oldPGrid;
		pGrid->addGate(this);
		//update net boxes
		calNetBox();
		calHPWL();
		for (int m = 0; m < fanin.size(); m ++)
		{
			Gate *fin = fanin[m];
			fin->calNetBox();
			fin->calHPWL();
		}
		for (int m = 0; m < fanout.size(); m ++)
		{
			Gate *fout = fanout[m];
			fout->calNetBox();
			fout->calHPWL();
		}
    optChannelLength = oldOptChannelLength;
		calOptDelay();
		calOptLeakage();

		for (int m = 0; m < paths.size(); m ++)
		{
			Path *tempP = paths[m];
			tempP->decSwappedCell();
		}
	}

	void Gate::clearMem()
	{
    weight = 0;
    critical = false;
    hpwl = 0;
    paths.clear();
    swapped = false;
    fixed = false;
	}

  void Gate::addFaninPad(Pad *p)
  {
  	for (int i = 0; i < faninPads.size(); i ++)
  	{
  		if (faninPads[i]->getId() == p->getId())
  		{
  			//cout<<"fanin pad "<<p->getId()<<" Name: "<<p->getName()<<" already added for gate "<<name<<endl;
		  	return;
		  }
  	}
  	faninPads.push_back(p);
  }

  void Gate::addFanoutPad(Pad *p)
  {
  	for (int i = 0; i < fanoutPads.size(); i ++)
  	{
  		if (fanoutPads[i]->getId() == p->getId())
  		{
  			//cout<<"fanout pad "<<p->getId()<<" Name: "<<p->getName()<<" already added for gate "<<name<<endl;
		  	return;
		  }
  	}
  	fanoutPads.push_back(p);
  }

  void Gate::addFaninGate(Gate *g)
  {
  	for (int i = 0; i < fanin.size(); i ++)
  	{
  		if (fanin[i]->getId() == g->getId())
  		{
  			//cout<<"fanin gate "<<g->getId()<<" Name: "<<g->getName()<<" already added for gate "<<name<<endl;
		  	return;
		  }
  	}
  	fanin.push_back(g);
  }

  void Gate::addFanoutGate(Gate *g)
  {
  	for (int i = 0; i < fanout.size(); i ++)
  	{
  		if (fanout[i]->getId() == g->getId())
  		{
  			//cout<<"fanout gate "<<g->getId()<<" Name: "<<g->getName()<<" already added for gate "<<name<<endl;
		  	return;
		  }
  	}
  	fanout.push_back(g);
  }

	void Gate::calNetBox()
	{
		//bounding box of the gate
		int faninNum = fanin.size();
		int fanoutNum = fanout.size();

		int nlx = lx, nrx = lx+width, nby = by, nty = by+height;
		for (int j = 0; j < faninNum; j ++)
		{
			Gate *gin = fanin[j];
//	  		cout<<"Gate: "<<gin->getName()<<" ["<<gin->getLX()<<","<<gin->getBY()<<"]->["<<gin->getLX()+gin->getWidth()<<","<<gin->getBY()+gin->getHeight()<<"]"<<endl;
			if (nlx > gin->getLX())
				nlx = gin->getLX();
			if (nby > gin->getBY())
				nby = gin->getBY();
			if (nrx < gin->getRX())
				nrx = gin->getRX();
			if (nty < gin->getTY())
				nty = gin->getTY();
		}

		for (int j = 0; j < fanoutNum; j ++)
		{
			Gate *gout = fanout[j];
//	  		cout<<"Gate: "<<gout->getName()<<" ["<<gout->getLX()<<","<<gout->getBY()<<"]->["<<gout->getLX()+gout->getWidth()<<","<<gout->getBY()+gout->getHeight()<<"]"<<endl;
			if (nlx > gout->getLX())
				nlx = gout->getLX();
			if (nby > gout->getBY())
				nby = gout->getBY();
			if (nrx < gout->getRX())
				nrx = gout->getRX();
			if (nty < gout->getTY())
				nty = gout->getTY();
		}

		netBox.set(nlx, nby, nrx, nty);
	}

	int Gate::calHPWL()
	{
		//bounding box of the gate
		int faninNum = fanin.size();
		int fanoutNum = fanout.size();

		hpwl = 0;

		for (int j = 0; j < faninNum; j ++)
		{
			Gate *gin = fanin[j];
			int cX = gin->getCenterX();
			int cY = gin->getCenterY();
			hpwl += abs(cX-centerX)+abs(cY-centerY);
//	  		cout<<"Gate: "<<gin->getName()<<" ["<<gin->getLX()<<","<<gin->getBY()<<"]->["<<gin->getLX()+gin->getWidth()<<","<<gin->getBY()+gin->getHeight()<<"]"<<endl;
		}

		for (int j = 0; j < fanoutNum; j ++)
		{
			Gate *gout = fanout[j];
			int cX = gout->getCenterX();
			int cY = gout->getCenterY();
			hpwl += abs(cX-centerX)+abs(cY-centerY);
//	  		cout<<"Gate: "<<gout->getName()<<" ["<<gout->getLX()<<","<<gout->getBY()<<"]->["<<gout->getLX()+gout->getWidth()<<","<<gout->getBY()+gout->getHeight()<<"]"<<endl;
		}

		return hpwl;
	}

	int Gate::calHPWL(int x, int y)
	{
		//bounding box of the gate
		int faninNum = fanin.size();
		int fanoutNum = fanout.size();

		int wl = 0;

		for (int j = 0; j < faninNum; j ++)
		{
			Gate *gin = fanin[j];
			int cX = gin->getCenterX();
			int cY = gin->getCenterY();
			wl += abs(cX-x)+abs(cY-y);
//	  		cout<<"Gate: "<<gin->getName()<<" ["<<gin->getLX()<<","<<gin->getBY()<<"]->["<<gin->getLX()+gin->getWidth()<<","<<gin->getBY()+gin->getHeight()<<"]"<<endl;
		}

		for (int j = 0; j < fanoutNum; j ++)
		{
			Gate *gout = fanout[j];
			int cX = gout->getCenterX();
			int cY = gout->getCenterY();
			wl += abs(cX-x)+abs(cY-y);
//	  		cout<<"Gate: "<<gout->getName()<<" ["<<gout->getLX()<<","<<gout->getBY()<<"]->["<<gout->getLX()+gout->getWidth()<<","<<gout->getBY()+gout->getHeight()<<"]"<<endl;
		}

		return wl;
	}

	bool Gate::addPath(Path *p)
	{
          Path *p1;
          for (int i = 0; i < paths.size(); i++)
          {
            p1 = paths[i];
            if (p1->getId() == p->getId()){
              return false;
            }
          }
          paths.push_back(p);
          return true;
	}

  // JMS-SHK begin
	bool Gate::addMinusPath(Path *p)
	{
          Path *p1;
          for (int i = 0; i < minus_paths.size(); i++)
          {
            p1 = minus_paths[i];
            if (p1->getId() == p->getId()){
              return false;
            }
          }
          minus_paths.push_back(p);
          return true;
	}
  // JMS-SHK end

  bool Gate::removePath(Path *p)
  {
    Path *p1;
    for (int i = 0; i < paths.size(); i++)
    {
      p1 = paths[i];
      if (p1->getId() == p->getId()){
        // erase the path from the list
        paths.erase(paths.begin() + i);
        return true;
      }
    }
    // path was not found in the list for this gate
    return false;
  }

  void Gate::addDelayVal(string tName, double delay)
  {
    if (FFFlag)
      {
        int i;
        for (i = 0; i < fanoutTerms.size(); i ++)
          if (fanoutTerms[i]->getName() == tName)
            break;
        assert(i < fanoutTerms.size());
      }
    else
      {
        int i;
        for (i = 0; i < faninTerms.size(); i ++)
          if (faninTerms[i]->getName() == tName)
            break;
        assert(i < faninTerms.size());
      }

    delayMap[tName] = delay;
  }

  void Gate::calOptDelay()
  {
    if (!FFFlag && !disable)
      {
        for (int i = 0; i < faninTerms.size(); i ++)
          {
            string tName = faninTerms[i]->getName();
            double orgDelay = delayMap[tName];
            optDelayMap[tName] = orgDelay + Ad*(optChannelLength-channelLength);
          }
      }
  }

  double Gate::getMaxDelay()
  {
  	if(delayMap.size() == 0)
  	{
//  		cout<<"Gate "<<name<<" has no terminal!"<<endl;
  		return 0;
  	}

    StrDMapItr itr = delayMap.begin();
    double delay = itr->second;
    while(++itr != delayMap.end())
    {
    	if (delay < itr->second)
    		delay = itr->second;
    }

    return delay;
  }

  double Gate::getMaxOptDelay()
  {
  	if(optDelayMap.size() == 0)
  	{
//  		cout<<"Gate "<<name<<" has no terminal!"<<endl;
  		return 0;
  	}

    StrDMapItr itr = optDelayMap.begin();
    double delay = itr->second;
    while(++itr != optDelayMap.end())
    {
    	if (delay < itr->second)
    		delay = itr->second;
    }

    return delay;
  }

  double Gate::getDelayVal(string tName)
  {
    StrDMapItr itr = delayMap.find(tName);
    if(itr == delayMap.end())
    {
	    cout<<"Gate: "<<name<<" terminal: "<<tName<<endl;
			cout<<"g->getGateType() == GATEPI: "<<(type == GATEPI)<<endl;
	    print();
    }
    assert(itr != delayMap.end());

    return delayMap[tName];
  }

  double Gate::getOptDelayVal(string tName)
  {
  	if (disable)
  		return 0;
    StrDMapItr itr = optDelayMap.find(tName);
    assert(itr != optDelayMap.end());
    return optDelayMap[tName];
  }

  Pad * Gate::getInputPad()
  {
    assert(faninPads.size() == 1);
    return faninPads[0];
  }
  Pad * Gate::getOutputPad()
  {
    assert(fanoutPads.size() == 1);
    return fanoutPads[0];
  }
  Gate * Gate::getInputFF()
  {
    assert(fanin.size() > 0);
    bool found = false;
    for (int i = 0; i < fanin.size(); i ++)
      {
        if (fanin[i]->getFFFlag())
          {
            found = true;
            return fanin[i];
          }
      }
    assert(found);
    return NULL;
  }
  Gate * Gate::getOutputFF()
  {
    assert(fanout.size() > 0);
    bool found = false;
    for (int i = 0; i < fanout.size(); i ++)
      {
        if (fanout[i]->getFFFlag())
          {
            found = true;
            return fanout[i];
          }
      }
    assert(found);
    return NULL;
  }
  bool Gate::outputIsPad()
  {
    if (fanoutPads.size() > 0)
      return true;
    else
      return false;
  }
  bool Gate::outputIsFF()
  {
    if (fanout.size() == 0)
      return false;
    for (int i = 0; i < fanout.size(); i ++)
      {
        if (fanout[i]->getFFFlag())
          {
            return true;
          }
      }
    return false;
  }
  bool Gate::inputIsPad()
  {
    if (faninPads.size() > 0)
      return true;
    else
      return false;
  }
  bool Gate::inputIsFF()
  {
    if (fanin.size() == 0)
      return false;
    for (int i = 0; i < fanin.size(); i ++)
      {
        if (fanin[i]->getFFFlag())
          {
            return true;
          }
      }
    return false;
  }

  void Gate::setFaninLoopGatePtr(Gate *g)
  {
    bool found = false;
    for (int i = 0; i < fanin.size(); i ++)
      {
        if (g->getId() == fanin[i]->getId())
          {
            setFaninLoopGate(i);
            found = true;
          }
      }
    assert(found);
  }

  Terminal* Gate::getFaninTerminalByName(string &name)
  {
    for (int i = 0; i < faninTerms.size(); i ++)
      {
        if (faninTerms[i]->getName() == name)
          {
            return faninTerms[i];
          }
      }
    return NULL;
  }

  Terminal* Gate::getFanoutTerminalByName(string &name)
  {
    for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        if (fanoutTerms[i]->getName() == name)
          {
            return fanoutTerms[i];
          }
      }
    return NULL;
  }

  double Gate::getLoadCap()
  {
    double loadCap = 0;
    for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        Terminal *t = fanoutTerms[i];
        double cap = t->getLoadCap();
        if (cap > loadCap)
          {
            loadCap = cap;
          }
      }
//    if (loadCap <= 0)
//    	cout<<"Warning gate: "<<name<<" loadcap: "<<loadCap<<endl;
    return loadCap;
  }

  double Gate::getInputSlew()
  {
    double inputSlew = 0;
    for (int i = 0; i < faninTerms.size(); i ++)
      {
        Terminal *t = faninTerms[i];
        double slew = t->getInputSlew();
        if (slew > inputSlew)
          {
            inputSlew = slew;
          }
      }
//    if (inputSlew <= 0)
//    	cout<<"Warning gate: "<<name<<" slew: "<<inputSlew<<endl;

    return inputSlew;
  }

  double Gate::getDelay()
  {
    assert(!FFFlag);
    assert(fanoutTerms.size() == 1);
    return fanoutTerms[0]->getDelay();
  }

  double Gate::getFFDelay(int index)
  {
    assert(0 <= index && index < fanoutTerms.size());
    return fanoutTerms[index]->getDelay();
  }

  double Gate::getMaxFFDelay()
  {
    assert(0  < fanoutTerms.size());
    double maxDelay = fanoutTerms[0]->getDelay();
    for (int i = 1; i < fanoutTerms.size(); i ++)
      {
        double delay = fanoutTerms[i]->getDelay();
        if (delay > maxDelay)
          maxDelay = delay;
      }
    return maxDelay;
  }

	Terminal *Gate::getFaninConnectedTerm(int gId)
	{
		for (int i = 0; i < inputSubnets.size(); i ++)
		{
			Subnet *s = inputSubnets[i];
			if (!s->inputIsPad())
			{
				if (s->getInputTerminal()->getGate()->getId() == gId)
				{
					Terminal *t = s->getOutputTerminal();
					assert(t->getGate()->getId() == this->getId());
					return t;
				}
			}
		}
		return NULL;
	}

	bool Gate::allFaninTermDisabled()
	{
    for (int i = 0; i < faninTerms.size(); i ++)
      {
      	if (!faninTerms[i]->isDisabled())
      		return false;
      }
      return true;
	}

	bool Gate::allFanoutGateDisabled()
	{
    for (int i = 0; i < fanout.size(); i ++)
      {
      	if (!fanout[i]->isDisabled())
      		return false;
      }
      return true;
	}

  void Gate::print()
  {
    cout<<"Gate "<<id<<" Name "<<name<<" Channel Length: "<<channelLength<<endl;
    cout<<"Fanin terms: ";
    for (int i = 0; i < faninTerms.size(); i ++)
      {
        cout<<"Id: "<<faninTerms[i]->getId()<<" Name: "<<faninTerms[i]->getName()<<" ";
      }
    cout<<endl;
    cout<<"Fanout terms: ";
    for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        cout<<"Id: "<<fanoutTerms[i]->getId()<<" Name: "<<fanoutTerms[i]->getName()<<" ";
      }
    cout<<endl;
  }

  double Gate::getMinDelay()
  {
  	if(delayMap.size() == 0)
  	{
//  		cout<<"Gate "<<name<<" has no terminal!"<<endl;
  		return 0;
  	}

    StrDMapItr itr = delayMap.begin();
    double delay = itr->second;
    while(++itr != delayMap.end())
    {
    	if (delay > itr->second)
    		delay = itr->second;
    }

    return delay;
  }

	bool Gate::calLeakWeight()
	{
		if (getMaxDelay() == 0)
			leakWeight = 1;
		else
			leakWeight = leakage/getMaxDelay();//abs(5.0*Al/Ad + (200*Al+Bl)/Ad);
	}

	double Gate::calMaxPossibleDelay()
	{
		double delay = getMaxDelay();
		return delay + fabs(Ad)*10;
	}

	void Gate::addNet(Net *n)
	{
		for (int i = 0; i < nets.size(); i ++)
		{
			if (n->getId() == nets[i]->getId())
				return;
		}
		nets.push_back(n);
	}

  void Gate::updateToggleProfile(int cycle_num)
  {
     int sz = CHAR_BIT*sizeof(ulong);
     int rem = cycle_num%sz;
     int quo = cycle_num/sz;

     if (quo + 1 > toggle_profile.size()) toggle_profile.resize(quo+1, (long) 0xFFFFFFFFFFFFFFFF);
     toggle_profile[quo].set(rem,0);
     if (max_toggle_profile_size < toggle_profile.size()) max_toggle_profile_size = toggle_profile.size();
  }

   void Gate::printToggleProfile(ofstream& file)
   {

       file << name << ",";
       for (int i = 0; i < toggle_profile.size(); i++)
       {
         file << toggle_profile[i].to_string() << ",";
       }
       file << endl;
   }

   void Gate::print_terms(ofstream& file)
   {
      for (int i = 0; i < faninPads.size(); i ++)
      {
        Pad* pad = faninPads[i];
        file << pad->getName() << ":" << pad->getSimValue() << endl;
      }
      for (int i = 0; i < faninTerms.size(); i ++)
      {
        Terminal* term = faninTerms[i];
        file << term->getFullName() << ":" << term->getSimValue() << endl;
      }
      for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        Terminal* term = fanoutTerms[i];
        file << term->getFullName() << ":" << term->getSimValue() << endl;
      }
      for (int i = 0; i < fanoutPads.size(); i ++)
      {
        Pad* pad = fanoutPads[i];
        file << pad->getName() << ":" << pad->getSimValue() << endl;
      }
   }

   int Gate::getToggleCountFromProfile()
   {
      int sum = 0;
      for (int i = 0; i < toggle_profile.size(); i++)
      {
        sum += toggle_profile[i].count();
      }
      return sum;
   }

   void Gate::resizeToggleProfile(int val)
   {
      toggle_profile.resize(val, (long) 0xFFFFFFFFFFFFFFFF);
   }

   int   Gate::getToggleCorrelation(Gate* gate)
   {
      int sz1 = toggle_profile.size();
      int sz2 = gate->toggle_profile.size();
      //assert (sz1 == sz2);
      int sz = sz1 > sz2 ? sz2 : sz1;
      resizeToggleProfile(sz);
      gate->resizeToggleProfile(sz);
      int sum = 0;
      for (int i = 0; i < sz ; i++)
      {
        long temp = (toggle_profile[i].to_ulong()) & (gate->toggle_profile[i].to_ulong());
        bitset<64> x(temp);
        sum += x.count(); // counts number of 1's
      }
      return sum;
   }

   bool Gate::notInteresting()
   {
       vector<string> hierarchy;
       tokenize (name, '/', hierarchy);
       if (hierarchy[0] == "sfr_0"  )        return true;
       if (hierarchy[0] == "dbg_0" )         return true;
       if (hierarchy[0] == "watchdog_0" )    return true;
       if (name.find("clk") != string::npos) return true;
       return false;
   }

    string Gate::getMuxSelectPinVal()
    {
        assert(func == MUX);
        string val = "";
        for(int i = 0; i < faninTerms.size(); i++)
        {
            Terminal* term = faninTerms[i];
            if (term->getName() == "S") val = term->getToggledTo();
        }
        assert(val.length());
        return val;
    }

    void Gate::untoggleMuxInput(string input)
    {
       assert(func == MUX);
       for (int i = 0; i < faninTerms.size(); i++)
        {
            Terminal* term = faninTerms[i];
            string term_name = term->getName();
            if (term_name.find(input) != string::npos) term->setToggled(false); // ideally I should disable the terminal, since it might have toggled but leads to no paths.
        }
    }

      // ensure gate is not FF or LH or INV or BUFF
      // handle mux differently
    bool Gate::checkANDToggle  ()
    {
       // ASSUMES OUTPUT TOGGLED
      ToggleType toggle_type = fanoutTerms[0]->getToggType();
      bool fine;
      switch (toggle_type)
      {
        case RISE:
                   assert(fanoutTerms[0]->getToggledTo() == "1");
                    fine = true;
                   for (int i = 0; i < faninTerms.size(); i++)
                   {
                      Terminal* term = faninTerms[i];
                      if ((term->isToggled() && term->getToggledTo() == "0")) fine = false; // x is also possible
                   }
                   break;
        case FALL:
                    assert(fanoutTerms[0]->getToggledTo() == "0");
                    fine = false;
                    for (int i = 0; i < faninTerms.size(); i++)
                    {
                        Terminal* term = faninTerms[i];
                        if (term->getToggledTo() == "0" || term->getToggledTo() == "x") fine = true;
                    }
                    break;
        case UNKN:
                    //assert(fanoutTerms[0]->getToggledTo() == "x");
                    break;
        default  : assert(0);
      }
      assert(fine);
    }
    bool Gate::checkAOIToggle  ()
    {
        Terminal * term_A1, *term_A2, * term_B;
        for (int i = 0; i < faninTerms.size(); i ++)
        {
            Terminal* term = faninTerms[i];
            if (term->getName() == "A1") term_A1 = term;
            else if (term->getName() == "A2") term_A2 = term;
            else if (term->getName() == "B") term_B = term;
        }
        ToggleType toggle_type = fanoutTerms[0]->getToggType();
        bool fine;
        switch (toggle_type)
        {
          case RISE:
            assert(fanoutTerms[0]->getToggledTo() == "1");
            fine = true;
            if (term_B->getToggledTo() == "1") fine = false;
            else if (term_A1->getToggledTo() == "1" && term_A2->getToggledTo() == "1") fine = false;
            //else if (term_A2->getToggledTo() == "0") fine = false;
            break;
          case FALL:
            assert(fanoutTerms[0]->getToggledTo() == "0");
            fine = true;
            //if (term_B->getToggledTo() == "1") fine = false;
            if (term_B->getToggledTo() == "0" && (term_A1->getToggledTo() == "0" || term_A2->getToggledTo() == "0")) fine = false;
            break;
          case UNKN:
            //assert(fanoutTerms[0]->getToggledTo() == "x");
            break;
          default  : assert(0);
        }
        assert(fine);
    }
    bool Gate::checkBUFFToggle ()
    {
        assert(faninTerms.size() == 1);
        assert(faninTerms[0]->isToggled() == fanoutTerms[0]->isToggled());
        assert(faninTerms[0]->getToggType() == fanoutTerms[0]->getToggType());
        assert(faninTerms[0]->getToggledTo() == fanoutTerms[0]->getToggledTo());
    }
    bool Gate::checkDFFToggle  ()
    {
       // NOTHING HERE. CANNOT EVEN CHECK THE THE CLOCK PIN TOGGLED (What if it was clock gated?)
    }
    bool Gate::checkINVToggle  ()
    {
        assert(faninTerms.size() == 1);
        ToggleType toggle_type = fanoutTerms[0]->getToggType();
        switch (toggle_type)
        {
            case RISE:
                      assert(fanoutTerms[0]->getToggledTo() == "1");
                      assert(faninTerms[0]->getToggledTo() == "0");
                      break;
            case FALL:
                      assert(fanoutTerms[0]->getToggledTo() == "0");
                      assert(faninTerms[0]->getToggledTo() == "1");
                      break;
            case UNKN:
                      //assert(fanoutTerms[0]->getToggledTo() == "x");
                      break;
            default  : assert(0);

        }
    }
    bool Gate::checkLHToggle   ()
    {
       // NOTHING HERE. CANNOT EVEN CHECK THE THE CLOCK PIN TOGGLED (What if it was clock gated?)
    }
    bool Gate::checkMUXToggle  ()
    {
        string select_val = "s";
        Terminal * term_0 ,* term_1,* term_s;
        for (int i = 0; i < faninTerms.size(); i ++)
        {
            Terminal* term = faninTerms[i];
            if (term->getName() == "S")  term_s = term;
            else if (term->getName() == "I1") term_1 = term;
            else if (term->getName() == "I0") term_0 = term;
        }
        select_val = term_s->getToggledTo();
        assert(select_val != "s");
        if (select_val == "1")
            assert(fanoutTerms[0]->getToggledTo() == term_1->getToggledTo() || term_1->getToggledTo() == "x");
        else if (select_val == "0")
            assert(fanoutTerms[0]->getToggledTo() == term_0->getToggledTo() || term_0->getToggledTo() == "x");
        else if (select_val == "x") {}
        else assert(0);
    }
    bool Gate::checkNANDToggle ()
    {
      ToggleType toggle_type = fanoutTerms[0]->getToggType();
      bool fine;
      switch (toggle_type)
      {
        case FALL:
                   assert(fanoutTerms[0]->getToggledTo() == "0");
                   fine = true;
                   for (int i = 0; i < faninTerms.size(); i++)
                   {
                      Terminal* term = faninTerms[i];
                      if ((term->isToggled() && term->getToggledTo() == "0")) fine = false; // x is also possible
                   }
                   break;
        case RISE:
                    assert(fanoutTerms[0]->getToggledTo() == "1");
                    fine = false;
                    for (int i = 0; i < faninTerms.size(); i++)
                    {
                        Terminal* term = faninTerms[i];
                        if (term->getToggledTo() == "0" || term->getToggledTo() == "x") fine = true;
                    }
                    break;
        case UNKN:
                    //assert(fanoutTerms[0]->getToggledTo() == "x");
                    break;
        default  : assert(0);
      }
      assert(fine);

    }
    bool Gate::checkNORToggle  ()
    {
      ToggleType toggle_type = fanoutTerms[0]->getToggType();
      bool fine;
      switch (toggle_type)
      {
        case FALL:
                   assert(fanoutTerms[0]->getToggledTo() == "0");
                   fine = false;
                   for (int i = 0; i < faninTerms.size(); i++)
                   {
                      Terminal* term = faninTerms[i];
                      if (term->getToggledTo() == "1" || term->getToggledTo() == "x") fine = true;
                   }
                   break;
        case RISE:
                    assert(fanoutTerms[0]->getToggledTo() == "1");
                    fine = true;
                    for (int i = 0; i < faninTerms.size(); i++)
                    {
                        Terminal* term = faninTerms[i];
                        if ((term->isToggled() && term->getToggledTo() == "1")) fine = false; // x is also possible
                    }
                    break;
        case UNKN:
                    //assert(fanoutTerms[0]->getToggledTo() == "x");
                    break;
        default  : assert(0);
      }
      assert(fine);

    }
    bool Gate::checkOAIToggle  ()
    {
        Terminal * term_A1, *term_A2, * term_B;
        for (int i = 0; i < faninTerms.size(); i ++)
        {
            Terminal* term = faninTerms[i];
            if (term->getName() == "A1") term_A1 = term;
            else if (term->getName() == "A2") term_A2 = term;
            else if (term->getName() == "B") term_B = term;
        }
        ToggleType toggle_type = fanoutTerms[0]->getToggType();
        bool fine;
        switch (toggle_type)
        {
          case RISE:
            assert(fanoutTerms[0]->getToggledTo() == "1");
            fine = false;
            if (term_B->getToggledTo() == "0") fine = true;
            else if (term_A1->getToggledTo() == "0") fine = true;
            else if (term_A2->getToggledTo() == "0") fine = true;
            break;
          case FALL:
            assert(fanoutTerms[0]->getToggledTo() == "0");
            fine = true;
            if (term_B->getToggledTo() == "0") fine = false;
            if (term_A1->getToggledTo() == "0" && term_A2->getToggledTo() == "0") fine = false;
            break;
          case UNKN:
            //assert(fanoutTerms[0]->getToggledTo() == "x");
            break;
          default  : assert(0);
        }
        assert(fine);

    }
    bool Gate::checkORToggle   ()
    {
      ToggleType toggle_type = fanoutTerms[0]->getToggType();
      bool fine;
      switch (toggle_type)
      {
        case RISE:
                   assert(fanoutTerms[0]->getToggledTo() == "1");
                   fine = false;
                   for (int i = 0; i < faninTerms.size(); i++)
                   {
                      Terminal* term = faninTerms[i];
                      if (term->getToggledTo() == "1" || term->getToggledTo() == "x") fine = true;
                   }
                   break;
        case FALL:
                    assert(fanoutTerms[0]->getToggledTo() == "0");
                    fine = true;
                    for (int i = 0; i < faninTerms.size(); i++)
                    {
                        Terminal* term = faninTerms[i];
                        if ((term->isToggled() && term->getToggledTo() == "1")) fine = false; // x is also possible
                    }
                    break;
        case UNKN:
                    //assert(fanoutTerms[0]->getToggledTo() == "x");
                    break;
        default  : assert(0);
      }
      assert(fine);

    }
    bool Gate::checkXNORToggle ()
    {
        // BASIC CHECKS ONLY DONE FOR NOW
    }
    bool Gate::checkXORToggle  ()
    {
        // BASIC CHECKS ONLY DONE FOR NOW
    }
    bool Gate::check_toggle_fine()
    {
        // BASIC CHECK
        if (isClkTree) return true;
        if (name == "clock_module_0/clock_gate_aclk/U3") return true;
        if (func == DFF || func == LH) return true;
        if (!fanoutTerms[0]->isToggled()) return true;
        if (fanoutTerms[0]->getPrevVal() ==  fanoutTerms[0]->getToggledTo()) return true;
        bool fine = false;
        for (int i = 0; i < faninTerms.size(); i++)
        {
            Terminal* term = faninTerms[i];
            if (term->isToggled()) { fine = true ; break; }
        }
        if (!fine && fanoutTerms[0]->getPrevVal() == "x") fine = true;
        return fine;
        //assert(fine);

        // DETAILED FUNCTIONALITY CHECK
        switch (func)
        {
          case AND  :  return checkANDToggle  ( );
          case AOI  :  return checkAOIToggle  ( );
          case BUFF :  return checkBUFFToggle ( );
          case DFF  :  return checkDFFToggle  ( );
          case INV  :  return checkINVToggle  ( );
          case LH   :  return checkLHToggle   ( );
          case MUX  :  return checkMUXToggle  ( );
          case NAND :  return checkNANDToggle ( );
          case NOR  :  return checkNORToggle  ( );
          case OAI  :  return checkOAIToggle  ( );
          case OR   :  return checkORToggle   ( );
          case XNOR :  return checkXNORToggle ( );
          case XOR  :  return checkXORToggle  ( );
          default   :  assert(0);
        }
    }

    void Gate::handleANDMIS(vector<int>& false_term_ids, designTiming* T)
    {
      // check if gate toggled to controlling value
      assert(fanoutTerms.size() == 1);
      //if (isClkTree) return;
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "0")  return ;
      // get toggled term ids by iterating over all fanin terms
      // then check
      list <pair<Terminal*, double> > term_slack_list;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->isToggled() && ((term->getToggledTo() == "0") || (term->getToggledTo() == "x")))
         {
            term_slack_list.push_back(make_pair(term, 10000.0));
         }
      }
      //assert(term_slack_list.size() > 0);
      if (term_slack_list.size() < 2) return ;

      // WE HAVE controlling MIS !
      // Get MinSlack among inputs
      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         //string slack = T->getMaxFallSlack(term->getFullName());
         string slack = T->getMaxFallArrival(term->getFullName());;
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
         //slack
/*         Terminal * term = [i];
         double min_slack = 10000.0;
         if (term->isToggled() && term->getToggledTo() == "0")
         {
            double slack;
            slack = T->getMaxFallSlack(term->getFullName());
            if (min_slack > slack) min_slack = slack;
         }*/
      }

      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        if (slack == 10000.0)
        {
             continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }

      // check for MIS
      // return the vector of term id's to apply

    }

    void Gate::handleAOIMIS(vector<int>& false_term_ids, designTiming* T)
    {
      assert(fanoutTerms.size() == 1);
      //if (isClkTree) return;
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "1")  return ;
      Terminal * term_A1, *term_A2, * term_B;
      for (int i = 0; i < faninTerms.size(); i ++)
      {
        Terminal* term = faninTerms[i];
        if (term->getName() == "A1") term_A1 = term;
        else if (term->getName() == "A2") term_A2 = term;
        else if (term->getName() == "B") term_B = term;
      }
      list <pair<Terminal*, double> > term_slack_list;
      if (term_B->isToggled() && (term_B->getToggledTo() != "0"))
        term_slack_list.push_back(make_pair(term_B, 10000.0));

      if (term_A1->isToggled() && term_A1->getToggledTo() != "0" && term_A2->getToggledTo() != "0") {
        term_slack_list.push_back(make_pair(term_A1, 10000.0));
      }
      if (term_A2->isToggled() && term_A2->getToggledTo() != "0" && term_A1->getToggledTo() != "0") {
        term_slack_list.push_back(make_pair(term_A2, 10000.0));
      }
      if (term_slack_list.size() < 2) return ;

      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         //string slack = T->getMaxRiseSlack(term->getFullName());
         string slack = T->getMaxRiseArrival(term->getFullName());
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
      }
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        Terminal * term = it->first;
        if (slack == 10000.0)
        {
            continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }
    }

    void Gate::handleBUFFMIS (vector<int>& false_term_ids, designTiming* T )
    {
        return;
    }

    void Gate::handleDFFMIS  (vector<int>& false_term_ids, designTiming* T )
    {
        return;
    }

    void Gate::handleLHMIS   (vector<int>& false_term_ids, designTiming* T )
    {
        return;
    }

    void Gate::handleINVMIS  (vector<int>& false_term_ids, designTiming* T )
    {
        return;
    }

    void Gate::handleMUXMIS  (vector<int>& false_term_ids, designTiming* T )
    {
        return;
    }

    void Gate::handleNANDMIS (vector<int>& false_term_ids, designTiming* T )
    {
      //if (isClkTree) return;
      assert(fanoutTerms.size() == 1);
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "1")  return ;
      // get toggled term ids by iterating over all fanin terms
      // then check
      list <pair<Terminal*, double> > term_slack_list;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->isToggled() && ((term->getToggledTo() == "0") || (term->getToggledTo() == "x")))
         {
            term_slack_list.push_back(make_pair(term, 10000.0));
         }
      }
      if (term_slack_list.size() < 2) return ;

      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         //string slack = T->getMaxFallSlack(term->getFullName());
         string slack = T->getMaxFallArrival(term->getFullName());
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
         //slack
      }

      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        Terminal * term = it->first;
        if (slack == 10000.0)
        {
             continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }


    }

    void Gate::handleNORMIS  (vector<int>& false_term_ids, designTiming* T )
    {
      //if (isClkTree) return;
      assert(fanoutTerms.size() == 1);
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "0")  return ;
      list <pair<Terminal*, double> > term_slack_list;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->isToggled() && ((term->getToggledTo() == "1") || (term->getToggledTo() == "x")))
         {
            term_slack_list.push_back(make_pair(term, 10000.0));
         }
      }
      if (term_slack_list.size() < 2) return ;

      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         //string slack = T->getMaxRiseSlack(term->getFullName());
         string slack = T->getMaxRiseArrival(term->getFullName());
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
         //slack
      }

      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        Terminal * term = it->first;
        if (slack == 10000.0)
        {
             continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }
    }

    void Gate::handleOAIMIS  (vector<int>& false_term_ids, designTiming* T )
    {
      assert(fanoutTerms.size() == 1);
      //if (isClkTree) return;
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "0")  return ;
      Terminal * term_A1, *term_A2, * term_B;
      for (int i = 0; i < faninTerms.size(); i ++)
      {
        Terminal* term = faninTerms[i];
        if (term->getName() == "A1") term_A1 = term;
        else if (term->getName() == "A2") term_A2 = term;
        else if (term->getName() == "B") term_B = term;
      }
      list <pair<Terminal*, double> > term_slack_list;
      if (term_B->isToggled() && (term_B->getToggledTo() != "1"))
        term_slack_list.push_back(make_pair(term_B, 10000.0));

      if (term_A1->isToggled() && term_A1->getToggledTo() != "1" && term_A2->getToggledTo() != "1") {
        term_slack_list.push_back(make_pair(term_A1, 10000.0));
      }
      if (term_A2->isToggled() && term_A2->getToggledTo() != "1" && term_A1->getToggledTo() != "1") {
        term_slack_list.push_back(make_pair(term_A2, 10000.0));
      }
      if (term_slack_list.size() < 2) return ;

      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         string slack = T->getMaxFallArrival(term->getFullName());
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
      }
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        Terminal * term = it->first;
        if (slack == 10000.0)
        {
             continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }

    }

    void Gate::handleORMIS (vector<int>& false_term_ids, designTiming* T )
    {
      //if (isClkTree) return;
      assert(fanoutTerms.size() == 1);
      Terminal* term = fanoutTerms[0];
      if (!term->isToggled() || term->getToggledTo() != "1")  return ;
      list <pair<Terminal*, double> > term_slack_list;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->isToggled() && ((term->getToggledTo() == "1") || (term->getToggledTo() == "x")))
         {
            term_slack_list.push_back(make_pair(term, 10000.0));
         }
      }
      if (term_slack_list.size() < 2) return ;

      list<pair<Terminal*, double> >::iterator it;
      double max_slack = 0.0;
      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
         Terminal * term = it->first;
         string slack = T->getMaxRiseArrival(term->getFullName());
         cout << " Slack is " << slack << " for term " << term->getFullName() << endl;
         if (slack == "ARRIVAL" || slack == "INFINITY" || slack[0] == '0' || slack.empty()) {
            continue;
         }
         double slack_dbl = atof(slack.c_str());
         it->second  = slack_dbl;
         if (max_slack < slack_dbl) max_slack = slack_dbl;
         //slack
      }

      for (it = term_slack_list.begin(); it != term_slack_list.end(); it++)
      {
        double slack = it->second;
        Terminal * term = it->first;
        if (slack == 10000.0)
        {
             continue;
        }
        cout << "SLACK IS " << slack << "FOR " << term->getFullName() << endl;
        int id = it->first->getId();
        if (slack < 0.9*max_slack) false_term_ids.push_back(id);
      }

    }

    void Gate::handleXNORMIS (vector<int>& false_term_ids, designTiming* T ) {}
    void Gate::handleXORMIS  (vector<int>& false_term_ids, designTiming* T ) {}

    void Gate::checkControllingMIS(vector<int>& false_term_ids, designTiming* T)
    {
        // GET LIBCELL CLASS -> NAND, AND, NOR, OR, OAI, IAO, MUX, DFF, INV, XOR/XNOR, BUFF, LHQ (LATCH)
        if (isClkTree) return;
        switch (func)
        {
          case AND  :  return handleANDMIS  ( false_term_ids, T);
          case AOI  :  return handleAOIMIS  ( false_term_ids, T);
          case BUFF :  return handleBUFFMIS ( false_term_ids, T);
          case DFF  :  return handleDFFMIS  ( false_term_ids, T);
          case INV  :  return handleINVMIS  ( false_term_ids, T);
          case LH   :  return handleLHMIS   ( false_term_ids, T);
          case MUX  :  return handleMUXMIS  ( false_term_ids, T);
          case NAND :  return handleNANDMIS ( false_term_ids, T);
          case NOR  :  return handleNORMIS  ( false_term_ids, T);
          case OAI  :  return handleOAIMIS  ( false_term_ids, T);
          case OR   :  return handleORMIS   ( false_term_ids, T);
          case XNOR :  return handleXNORMIS ( false_term_ids, T);
          case XOR  :  return handleXORMIS  ( false_term_ids, T);
          default   :  assert(0);
        }
    }


    int Gate::numToggledInputs()
    {
      assert (func != MUX);
      int num_toggled_inputs = 0;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* term = faninTerms[i];
        if (term->isToggled()) num_toggled_inputs++;
      }
      return num_toggled_inputs;
//      if (num_toggled_inputs > 1) return true;
//      return false;
    }

    bool Gate:: isClusterBoundaryDriver()
    {
      //assert(!isClkTree);
      for (int i = 0; i < fanout.size(); i++)
      {
        Gate* gate = fanout[i];
        if (gate->getIsClkTree()) continue;
        if(cluster_id != gate->getClusterId())
        {
          cout << "Gate " << name << " in Cluster " << cluster_id << " is driving gate " << gate->getName() << " is cluster " << gate->getClusterId() << endl;
            return true;
        }
      }
      return false;
    }


    void Gate::handleORExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " | " ;
      }
      expr.resize(expr.size() - 2); // removing the last '|' and the accompanying white space
      expr = expr+")";
      fo_term->setExpr(expr);
    }

    void Gate::handleANDExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " & " ;
      }
      expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
      expr = expr+")";
      fo_term->setExpr(expr);
    }

    void Gate::handleBUFFExpr()
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ( ";
      Terminal* term = faninTerms[0];
      string exp = term->getExpr();
      expr = expr + exp+ " ) ";
      fo_term->setExpr(expr);
    }

    void Gate::handleINVExpr()
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ( ~";
      Terminal* term = faninTerms[0];
      string exp = term->getExpr();
      expr = expr + exp +" ) ";
      fo_term->setExpr(expr);
    }

    void Gate::handleMUXExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr;
      string S_expr, I1_expr, I0_expr;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "S")
            S_expr = term->getExpr();
         else if (term->getName() == "I1")
            I1_expr = term->getExpr();
         else if (term->getName() == "I0")
            I0_expr = term->getExpr();
      }
      expr = " ( " + S_expr + "&" + I1_expr + " + " + " ~" + S_expr + "&" + I0_expr + " ) " ;
      fo_term->setExpr(expr);
    }

    void Gate::handleAOIExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr;
      string B_expr, A1_expr, A2_expr;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "B")
            B_expr = term->getExpr();
         else if (term->getName() == "A1")
            A1_expr = term->getExpr();
         else if (term->getName() == "A2")
            A2_expr = term->getExpr();
      }
      expr = " ( ~" + B_expr + "| (" + A1_expr + " & " + A2_expr + " ) )" ;
      fo_term->setExpr(expr);
    }

    void Gate::handleOAIExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr;
      string B_expr, A1_expr, A2_expr;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "B")
            B_expr = term->getExpr();
         else if (term->getName() == "A1")
            A1_expr = term->getExpr();
         else if (term->getName() == "A2")
            A2_expr = term->getExpr();
      }
      expr = " ( ~" + B_expr + "& (" + A1_expr + " | " + A2_expr + " ) )" ;
      fo_term->setExpr(expr);
    }

    void Gate::handleNANDExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ~( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " & " ;
      }
      expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
      expr = expr + ")";
      fo_term->setExpr(expr);
    }

    void Gate::handleNORExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ~( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " | " ;
      }
      expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
      expr = expr+")";
      fo_term->setExpr(expr);
    }

    void Gate::handleXORExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " ^ " ;
      }
      expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
      expr = expr+")";
      fo_term->setExpr(expr);
    }

    void Gate::handleXNORExpr()
    {
      assert(fanoutTerms.size() == 1);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string expr = " ~( ";
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         string exp = term->getExpr();
         expr = expr + exp + " ^ " ;
      }
      expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
      expr = expr+")";
      fo_term->setExpr(expr);
    }

    void Gate::computeNetExpr()// its actually compute output term expression
    {
        // GET LIBCELL CLASS -> NAND, AND, NOR, OR, OAI, IAO, MUX, DFF, INV, XOR/XNOR, BUFF, LHQ (LATCH)
        Terminal* fo_term = fanoutTerms[0];
        if (isClkTree)
        {
            fo_term->setExpr("clk");
            return;
        }
        if (FFFlag)
        {
            fo_term->setExpr(name);
            return;
        }
/*        string expr = "(";
        for (int i = 0; i < faninTerms.size(); i++)
        {
           Terminal* term = faninTerms[i];
           string exp = term->getExpr();
           expr = expr+ exp + " & ";
        }
        expr.resize(expr.size() - 2); // removing the last '&' and the accompanying white space
        expr = expr+")";
        fo_term -> setExpr(expr);*/
        switch (func)
        {
          case AND  :  handleANDExpr  ( ); break;
          case AOI  :  handleAOIExpr  ( ); break;
          case BUFF :  handleBUFFExpr ( ); break;
          //case DFF  :  handleDFFExpr  ( ); break;
          case INV  :  handleINVExpr  ( ); break;
          //case LH   :  handleLHExpr   ( ); break;
          case MUX  :  handleMUXExpr  ( ); break;
          case NAND :  handleNANDExpr ( ); break;
          case NOR  :  handleNORExpr  ( ); break;
          case OAI  :  handleOAIExpr  ( ); break;
          case OR   :  handleORExpr   ( ); break;
          case XNOR :  handleXNORExpr ( ); break;
          case XOR  :  handleXORExpr  ( ); break;
          default   :  assert(0);
        }
        // Get the input pins and get their nets and their drivers.

    }

    string Gate::getSimValue()
    {
        if (fanoutTerms.size() == 0) return "No Fanout Terms for Gate "+name;
        return fanoutTerms[0]->getSimValue();
    }

    void Gate::setSimValueReg(string val, priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(FFFlag == 1);
      for (int i = 0; i < fanoutTerms.size(); i ++)
      {
        Terminal * fo_term = fanoutTerms[i];
        if (fo_term->getName() == "Q") 
        {
          fo_term->setSimValue(val, sim_wf);
        }
        else if (fo_term->getName() == "QN")
        {
          string val_inv;
          ComputeBasicINV_new(val_inv, val);
          fo_term->setSimValue(val_inv, sim_wf);
        }
        else assert(0);
      }
    }

    bool Gate::transferDtoQ(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(FFFlag);
        bool toggled = false;
        if (name == "u_top/u_sys/u_core/ipsr_q_reg_0_")
        {
          cout << "gate is u_top/u_sys/u_core/ipsr_q_reg_0_" << endl;
        }
        if (func == DFF || func == DFFQ || func == DFFRPQ || func == SDFFRPQ || func == SDFF || func == SDFFH  || func == SDFFHQ  || func == SDFFHQN || func == SDFFNH  || func == SDFFNSRH || func == SDFFQ   || func == SDFFQN   || func == SDFFR   || func == SDFFRHQ || func == SDFFRQ  || func == SDFFS   || func == SDFFSHQ || func == SDFFSQ  || func == SDFFSR  || func == SDFFSRHQ || func == SDFFTR  || func == SEDFF   || func == SEDFFHQ || func == SEDFFTR || func == SMDFFHQ )
        {
          string Dval,SetVal, ClrVal, Qval, QNval, SNVal, RNVal, SEVal, SIVal, ClkVal, RVal;
          Terminal * Q_term = NULL, *QN_term = NULL;
          for(int i = 0; i < faninTerms.size(); i++)
          {
            Terminal* term = faninTerms[i];

            if (term->getName() == "D")  Dval = term->getSimValue();
            else if (term->getName() == "SDN")  SetVal = term->getSimValue();
            else if (term->getName() == "CDN")  ClrVal = term->getSimValue();
            else if (term->getName() == "SN")  SNVal = term->getSimValue();
            else if (term->getName() == "RN")  RNVal = term->getSimValue();
            else if (term->getName() == "R")   RVal = term->getSimValue();
            else if (term->getName() == "SE")  SEVal = term->getSimValue();
            else if (term->getName() == "SI")  SIVal = term->getSimValue();
            else if (term->getName() == "CK")  ClkVal = term->getSimValue();
            else assert(0);
          }
          for(int i = 0; i < fanoutTerms.size(); i++)
          {
            Terminal* term = fanoutTerms[i];

            if (term->getName() == "Q") { Qval = term->getSimValue(); Q_term = term; }
            else if (term->getName() == "QN") { QNval = term->getSimValue(); QN_term = term; }
            else assert(0);
          }

          // MUX
          string OutMux;
          if (!SEVal.empty())
          {
            if (!SIVal.empty() && !Dval.empty())
            {
              ComputeBasicMUX(Dval, SIVal, SEVal, OutMux);
            }
          }
          else
          {
            OutMux = Dval;
          }
          if (ClrVal == "0")
          {
            toggled |= Q_term->setSimValue("0", sim_wf);
          }
          else if (SetVal == "0")
          {
            toggled |= Q_term->setSimValue("1", sim_wf);
          }
          else if (ClrVal == "X")
          {
            toggled |= Q_term->setSimValue("X", sim_wf);
          }
          else if (SetVal == "X")
          {
            toggled |= Q_term->setSimValue("X", sim_wf);
          }
          else if (SNVal == "0")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("1", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("0", sim_wf);
          }
          else if (SNVal == "X")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("X", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("X", sim_wf);
          }
          else if (RNVal == "0")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("0", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("1", sim_wf);
          }
          else if (RNVal == "X")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("X", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("X", sim_wf);
          }
          else if (RVal == "1")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("0", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("1", sim_wf);
          }
          else if (RVal == "X")
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue("X", sim_wf);
            if (QN_term != NULL)
              toggled |= QN_term->setSimValue("X", sim_wf);
          }
          else
          {
            if (Q_term != NULL)
              toggled |= Q_term->setSimValue(OutMux, sim_wf);
            if (QN_term != NULL)
            {
              string OutMux_inv;
              ComputeBasicINV(OutMux, OutMux_inv);
              toggled |= QN_term->setSimValue(OutMux_inv, sim_wf);
            }
          }
        }
        else if (func == LH )
        {
          string Dval,EnVal,Qval;
          for(int i = 0; i < faninTerms.size(); i++)
          {
            Terminal* term = faninTerms[i];

            if (term->getName() == "D")  Dval = term->getSimValue();
            else if (term->getName() == "E")  EnVal = term->getSimValue();
          }
          Terminal* fo_term = fanoutTerms[0];
          assert(fo_term->getName() == "Q");
          Qval = fo_term->getSimValue();
          if (EnVal == "0")
          {
            if (Dval != "0") toggled = true;
            fo_term->setSimValue("0", sim_wf);
          }
          else
          {
            if (Dval != Qval) toggled = true;
            fo_term->setSimValue(Dval, sim_wf);
          }
        }
        else if (func == TLATNTSCA)
        {
/*          string Eval,SEval,ECKval, CKval;
          for(int i = 0; i < faninTerms.size(); i++)
          {
            Terminal* term = faninTerms[i];

            if (term->getName() == "E")  Eval = term->getSimValue();
            else if (term->getName() == "SE")  SEval = term->getSimValue();
            else if (term->getName() == "CK")  CKval = term->getSimValue();
          }
          Terminal* fo_term = fanoutTerms[0];
          assert(fo_term->getName() == "ECK");
          ECKval = fo_term->getSimValue();
          string n1;
          ComputeBasicOR(Eval, SEval, n1);
          string n0;
          if (CKval == "0") // Transfer the value
          {
            n0 = n1;
          }
          else // CKval = 1 or  X -> don't transfer the value
          { }
          ComputeBasicAND(n0, CKval, ECKval);
          fo_term->setSimValue(n0, sim_wf);*/
        }
//        else if (func == PREICG )
//        { 
//          ComputeValPREICG_new (sim_wf);
//        }
        else if (func == RF1R1WS)
        {
          ComputeValRF1R1WS_new (sim_wf);
        }
//        else if (func == RFWL)
//        {
//        }
        else if (func == LATCH) 
        {
          ComputeValLATCH_new(sim_wf);
        }
        else assert(0);
        return toggled;
    }

    bool Gate::computeVal(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      PowerOpt* po = PowerOpt::getInstance();
      if (po->getDesign() == "CORTEXM0PLUS" || po->getDesign() == "PLASTICARM" )
      {
        return computeValOne(sim_wf);
      }
      switch (func)
      {
        case AND  : toggled =  computeANDVal ( sim_wf ) ; break ;
        case AOI  : toggled =  computeAOIVal ( sim_wf ) ; break ;
        case BUFF : toggled =  computeBUFFVal( sim_wf ) ; break ;
        case DFF  : toggled =  computeDFFVal ( sim_wf ) ; break ;
        case INV  : toggled =  computeINVVal ( sim_wf ) ; break ;
        case LH   : toggled =  computeLHVal  ( sim_wf ) ; break ;
        case MUX  : toggled =  computeMUXVal ( sim_wf ) ; break ;
        case NAND : toggled =  computeNANDVal( sim_wf ) ; break ;
        case NOR  : toggled =  computeNORVal ( sim_wf ) ; break ;
        case OAI  : toggled =  computeOAIVal ( sim_wf ) ; break ;
        case OR   : toggled =  computeORVal  ( sim_wf ) ; break ;
        case XNOR : toggled =  computeXNORVal( sim_wf ) ; break ;
        case XOR  : toggled =  computeXORVal ( sim_wf ) ; break ;
        default   :  assert(0);
      }
      return toggled;

    }

    bool Gate::computeANDVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "0")
        {
            toggled = fo_term->setSimValue("0", sim_wf);
        }
        if (fi_0_val == "1")
        {
            //fo_term->setSimValue(fi_1_val, sim_wf); // simpler version
            if (fi_1_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
        return toggled;
    }
    bool Gate::computeAOIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string B_val, A1_val, A2_val;
      bool toggled;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "B")
            B_val = term->getSimValue();
         else if (term->getName() == "A1")
            A1_val = term->getSimValue();
         else if (term->getName() == "A2")
            A2_val = term->getSimValue();
      }

      if (B_val == "0")
      {
        if (A1_val == "0")
        {
            toggled = fo_term->setSimValue("1", sim_wf);
        }
        if (A1_val == "1")
        {
            if (A2_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (A2_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (A2_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (A1_val == "X")
        {
            if (A2_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
      }
      else if (B_val == "1")
      {
        toggled = fo_term->setSimValue("0", sim_wf);
      }
      else if (B_val == "X")
      {
        if(A1_val == "1" && A2_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
        else toggled = fo_term->setSimValue("X", sim_wf);
      }
      return toggled;
    }
    bool Gate::computeBUFFVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 1);
        bool toggled;
        Terminal* fo_term = fanoutTerms[0];
        Terminal* fi_term = faninTerms[0];
        toggled = fo_term->setSimValue(fi_term->getSimValue(), sim_wf);
    }
    bool Gate::computeDFFVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf){ return true; } // since if a ff is required to be evaluated from the sim_wf it must have toggled.
    bool Gate::computeINVVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 1);
        Terminal* fo_term = fanoutTerms[0];
        string fi_val = faninTerms[0]->getSimValue();
        if (fi_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
        else if (fi_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
        else if (fi_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
    }
    bool Gate::computeLHVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf){ return true; }
    bool Gate::computeMUXVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      Terminal* fo_term = fanoutTerms[0];
      string S_val, I1_val, I0_val;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "S")
            S_val = term->getSimValue();
         else if (term->getName() == "I1")
            I1_val = term->getSimValue();
         else if (term->getName() == "I0")
            I0_val = term->getSimValue();
      }
      bool toggled;
      if (S_val == "0") toggled = fo_term->setSimValue(I0_val, sim_wf);
      else if (S_val == "1") toggled = fo_term->setSimValue(I1_val, sim_wf);
      else if (S_val == "X") {
         if (I1_val != I0_val)
          toggled = fo_term->setSimValue("X", sim_wf);
        else
          toggled = fo_term->setSimValue(I1_val, sim_wf);
      }
      return toggled;
    }
    bool Gate::computeNANDVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "0")
        {
            toggled = fo_term->setSimValue("1", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
        return toggled;
    }
    bool Gate::computeNORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "1")
        {
            toggled = fo_term->setSimValue("0", sim_wf);
        }
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
        return toggled;
    }
    bool Gate::computeOAIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      Terminal* fo_term = fanoutTerms[0];
      list <pair<Terminal*, double> > term_slack_list;
      string B_val, A1_val, A2_val;
      for (int i = 0; i < faninTerms.size(); i++)
      {
         Terminal* term = faninTerms[i];
         if (term->getName() == "B")
            B_val = term->getSimValue();
         else if (term->getName() == "A1")
            A1_val = term->getSimValue();
         else if (term->getName() == "A2")
            A2_val = term->getSimValue();
      }

      bool toggled;
      if (B_val == "1")
      {
        if (A1_val == "1")
        {
            toggled = fo_term->setSimValue("0", sim_wf);
        }
        if (A1_val == "0")
        {
            if (A2_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (A2_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (A2_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (A1_val == "X")
        {
            if (A2_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
      }
      else if (B_val == "0")
      {
        toggled = fo_term->setSimValue("1", sim_wf);
      }
      else if (B_val == "X")
      {
        if(A1_val == "0" && A2_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
        else toggled = fo_term->setSimValue("X", sim_wf);
      }
      return toggled;
    }
    bool Gate::computeORVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "1")
        {
            toggled = fo_term->setSimValue("1", sim_wf);
        }
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "1") toggled = fo_term->setSimValue("1", sim_wf);
            else toggled = fo_term->setSimValue("X", sim_wf);
        }
        return toggled;
    }
    bool Gate::computeXNORVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            toggled = fo_term->setSimValue("X", sim_wf);
        }
    }
    bool Gate::computeXORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0];
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        bool toggled;
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") toggled = fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") toggled = fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") toggled = fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            toggled = fo_term->setSimValue("X", sim_wf);
        }
        return toggled;
    }


    // BASIC GATES
    void Gate::ComputeBasicAND(string A, string B, string& Y)
    {
      if (A == "1")
      {
        if (B == "1")  Y = "1"; // 11
        else if (B == "0")  Y = "0"; // 10
        else if (B == "X") Y = "X"; // 1X
      }
      else if (A == "0")
      {
        Y = "0"; // 0X
      }
      else if (A == "X")
      {
        if (B == "1")  Y = "X"; // X1
        else if (B == "0")  Y = "0"; // X0
        else if (B == "X") Y = "X"; // XX
      }
      else assert(0);
    }

    void Gate::ComputeBasicOR(string A, string B, string& Y)
    {
      if (A == "1")
      {
        Y = "1";
      }
      else if (A == "0")
      {
        if (B == "1")  Y = "1"; // 01
        else if (B == "0")  Y = "0"; //00
        else if (B == "X") Y = "X"; // 0X
      }
      else if (A == "X")
      {
        if (B == "1")  Y = "1"; // X1
        else if (B == "0")  Y = "X"; // X0
        else if (B == "X") Y = "X"; // XX
      }
      else assert(0);
    }

    void Gate::ComputeBasicXOR(string A, string B, string& Y)
    {
      if (A == "1")
      {
        if (B == "1")  Y = "0"; // 11
        else if (B == "0")  Y = "1"; //10
        else if (B == "X") Y = "X"; // 1X
      }
      else if (A == "0")
      {
        if (B == "1")  Y = "1"; // 01
        else if (B == "0")  Y = "0"; //00
        else if (B == "X") Y = "X"; // 0X
      }
      else if (A == "X")
      {
        Y = "X";
      }
      else assert(0);
    }

    void Gate::ComputeBasicINV(string A, string& Y)
    {
      if (A == "1") Y = "0";
      else if (A == "0") Y = "1";
      else if (A == "X") Y = "X";
      else assert(0);
    }

    void Gate::ComputeBasicMUX(string A, string B, string S, string& Y)
    {
        if (S == "0") Y = A;
        else if (S == "1") Y = B;
        else if (S == "X")
        {
          if (A != B) Y = "X";
          else Y = A;
        }
        else assert(0);
    }


    void Gate::ComputeBasicAND_new(string& Y, string A, string B)
    {
      if (A == "1")
      {
        if (B == "1")  Y = "1"; // 11
        else if (B == "0")  Y = "0"; // 10
        else if (B == "X") Y = "X"; // 1X
      }
      else if (A == "0")
      {
        Y = "0"; // 0X
      }
      else if (A == "X")
      {
        if (B == "1")  Y = "X"; // X1
        else if (B == "0")  Y = "0"; // X0
        else if (B == "X") Y = "X"; // XX
      }
      else assert(0);

    }

    void Gate::ComputeBasicAND_new(string& Y, string A, string B, string C)
    {
      string AandB;
      ComputeBasicAND_new(AandB, A, B);
      ComputeBasicAND_new(Y, AandB, C);
    }

    void Gate::ComputeBasicAND_new(string& Y, string A, string B, string C, string D)
    {
      string AandBandC;
      ComputeBasicAND_new(AandBandC, A, B, C);
      ComputeBasicAND_new(Y, AandBandC, D);
    }

    void Gate::ComputeBasicOR_new (string& Y, string A, string B)
    {
      if (A == "1")
      {
        Y = "1";
      }
      else if (A == "0")
      {
        if (B == "1")  Y = "1"; // 01
        else if (B == "0")  Y = "0"; //00
        else if (B == "X") Y = "X"; // 0X
      }
      else if (A == "X")
      {
        if (B == "1")  Y = "1"; // X1
        else if (B == "0")  Y = "X"; // X0
        else if (B == "X") Y = "X"; // XX
      }
      else assert(0);

    }

    void Gate::ComputeBasicOR_new (string& Y, string A, string B, string C)
    {
      string AorB;
      ComputeBasicOR_new(AorB, A, B);
      ComputeBasicOR_new(Y, AorB, C);

    }
    void Gate::ComputeBasicOR_new (string& Y, string A, string B, string C, string D)
    {
      string AorBorC;
      ComputeBasicOR_new(AorBorC, A, B, C);
      ComputeBasicOR_new(Y, AorBorC, D);
    }

    void Gate::ComputeBasicXOR_new (string& Y, string A, string B)
    {
      if (A == "1")
      {
        if (B == "1")  Y = "0"; // 11
        else if (B == "0")  Y = "1"; //10
        else if (B == "X") Y = "X"; // 1X
      }
      else if (A == "0")
      {
        if (B == "1")  Y = "1"; // 01
        else if (B == "0")  Y = "0"; //00
        else if (B == "X") Y = "X"; // 0X
      }
      else if (A == "X")
      {
        Y = "X";
      }
      else assert(0);

    }

    void Gate::ComputeBasicXOR_new (string& Y, string A, string B, string C)
    {
      string AxorB;
      ComputeBasicXOR_new(AxorB, A, B);
      ComputeBasicXOR_new(Y, AxorB, C);
    }

    void Gate::ComputeBasicXOR_new (string& Y, string A, string B, string C, string D)
    {
      string AxorBxorC;
      ComputeBasicXOR_new(AxorBxorC, A, B, C);
      ComputeBasicXOR_new(Y, AxorBxorC, D);
    }

    void Gate::ComputeBasicXNOR_new (string& Y, string A, string B)
    {
      string AxorB;
      ComputeBasicXOR_new(AxorB, A, B);
      ComputeBasicINV_new(Y, AxorB);
    }

    void Gate::ComputeBasicXNOR_new (string& Y, string A, string B, string C)
    {
      string AxorBxorC;
      ComputeBasicXOR_new(AxorBxorC, A, B, C);
      ComputeBasicINV_new(Y, AxorBxorC);
    }

    void Gate::ComputeBasicXNOR_new (string& Y, string A, string B, string C, string D)
    {
      string AxorBxorCxorD;
      ComputeBasicXOR_new(AxorBxorCxorD, A, B, C, D);
      ComputeBasicINV_new(Y, AxorBxorCxorD);
    }


    void Gate::ComputeBasicNAND_new (string& Y, string A, string B)
    {
      string AandB;
      ComputeBasicAND_new (AandB, A, B);
      ComputeBasicINV_new (Y, AandB);
    }

    void Gate::ComputeBasicNAND_new (string& Y, string A, string B, string C)
    {
      string AandBandC;
      ComputeBasicAND_new (AandBandC, A, B, C);
      ComputeBasicINV_new (Y, AandBandC);
    }

    void Gate::ComputeBasicNAND_new (string& Y, string A, string B, string C, string D)
    {
      string AandBandCandD;
      ComputeBasicAND_new (AandBandCandD, A, B, C, D);
      ComputeBasicINV_new (Y, AandBandCandD);
    }


    void Gate::ComputeBasicNOR_new (string& Y, string A, string B)
    {
      string AorB;
      ComputeBasicOR_new (AorB, A, B);
      ComputeBasicINV_new (Y, AorB);
    }

    void Gate::ComputeBasicNOR_new (string& Y, string A, string B, string C)
    {
      string AorBorC;
      ComputeBasicOR_new (AorBorC, A, B, C);
      ComputeBasicINV_new (Y, AorBorC);
    }

    void Gate::ComputeBasicNOR_new (string& Y, string A, string B, string C, string D)
    {
      string AorBorCorD;
      ComputeBasicOR_new (AorBorCorD, A, B, C, D);
      ComputeBasicINV_new (Y, AorBorCorD);
    }

    void Gate::ComputeBasicINV_new (string& Y, string A)
    {
      if (A == "1") Y = "0";
      else if (A == "0") Y = "1";
      else if (A == "X") Y = "X";
      else assert(0);

    }

    void Gate::ComputeBasicMUX_new (string& Y, string A, string B, string S)
    {
        if (S == "0") Y = A;
        else if (S == "1") Y = B;
        else if (S == "X")
        {
          if (A != B) Y = "X";
          else Y = A;
        }
        else assert(0);

    }

//#include <./Gate_Technology_ARM.txt> 

bool Gate::ComputeValADDF_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 2);
  assert (faninTerms.size() == 3);
  string S, CO;
  string A, CI, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "CI") CI = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string b_and_ci, a_and_b, a_and_ci;
  ComputeBasicXOR_new (S, A, B, CI);
  ComputeBasicAND_new (a_and_b, A, B);
  ComputeBasicAND_new (a_and_ci, A, CI);
  ComputeBasicAND_new (b_and_ci, B, CI);
  ComputeBasicOR_new (CO, a_and_b, a_and_ci, b_and_ci);

  assert(!S.empty()); 
  assert(!CO.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
    else if ( fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValADDFH_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 2);
  assert (faninTerms.size() == 3);
  string S, CO;
  string A, CI, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "CI") CI = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string b_and_ci, a_and_b, a_and_ci;
  ComputeBasicXOR_new (S, A, B, CI);
  ComputeBasicAND_new (a_and_b, A, B);
  ComputeBasicAND_new (a_and_ci, A, CI);
  ComputeBasicAND_new (b_and_ci, B, CI);
  ComputeBasicOR_new (CO, a_and_b, a_and_ci, b_and_ci);

  assert(!S.empty()); 
  assert(!CO.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
    else if ( fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValADDH_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 2);
  assert (faninTerms.size() == 2);
  string S, CO;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicXOR_new (S, A, B);
  ComputeBasicAND_new (CO, A, B);

  assert(!S.empty()); 
  assert(!CO.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
    else if ( fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAND2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicAND_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAND3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicAND_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAND4_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string A, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicAND_new (Y, A, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAO21_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicOR_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAO22_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI211_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, C0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicNOR_new (Y, B0, C0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI21_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicNOR_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI21B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A1, B0N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "B0N") B0N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicINV_new (outB, B0N);
  ComputeBasicNOR_new (Y,outA,outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI221_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 5);
  string Y;
  string B0, C0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicNOR_new (Y, C0, outB, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI222_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 6);
  string Y;
  string B0, C0, B1, A1, A0, C1;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "C1") C1 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outC, outB;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicAND_new (outC, C0, C1);
  ComputeBasicNOR_new (Y, outA, outB, outC);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI22_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicNOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI2B1_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicAND_new (outA, A0, outA1);
  ComputeBasicNOR_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI2BB1_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1N, A0N;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0N") A0N = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicNOR_new (outA, A0N, A1N);
  ComputeBasicNOR_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI2BB2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1N, A0N;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0N") A0N = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicNOR_new (outA, A0N, A1N);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicNOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI31_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, A1, A0, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicAND_new (outA, A0, A1, A2);
  ComputeBasicNOR_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI32_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 5);
  string Y;
  string B0, B1, A1, A0, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1, A2);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicNOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAOI33_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 6);
  string Y;
  string B0, B1, A1, A0, B2, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "B2") B2 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicAND_new (outA, A0, A1, A2);
  ComputeBasicAND_new (outB, B0, B1, B2);
  ComputeBasicNOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValINV_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 1);
  string Y;
  string A;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicINV_new (Y, A);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNAND_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND2B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string AN, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNAND_new (Y, Ax, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNAND_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND3B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string AN, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNAND_new (Y, Ax, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND4_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string A, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNAND_new (Y, A, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND4B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string AN, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNAND_new (Y, Ax, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNAND4BB_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string BN, AN, D, C;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "BN") BN = fi_term->getSimValue();
    else if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else assert(0);
  }

  string Ax, Bx;
  ComputeBasicINV_new (Bx, BN);
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNAND_new (Y, Ax, Bx, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNOR_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR2B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string AN, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNOR_new (Y, Ax, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNOR_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR3B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string AN, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNOR_new (Y, Ax, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR4_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string A, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicNOR_new (Y, A, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR4B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string AN, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  string Ax;
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNOR_new (Y, Ax, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValNOR4BB_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string BN, AN, D, C;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "BN") BN = fi_term->getSimValue();
    else if ( fi_term_name == "AN") AN = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else assert(0);
  }

  string Ax, Bx;
  ComputeBasicINV_new (Bx, BN);
  ComputeBasicINV_new (Ax, AN);
  ComputeBasicNOR_new (Y, Ax, Bx, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOA21_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicAND_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOA22_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI211_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, C0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicNAND_new (Y, B0, C0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI21_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicNAND_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI21B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A1, B0N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "B0N") B0N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicINV_new (outB, B0N);
  ComputeBasicOR_new (outA, A0,  A1);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI221_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 5);
  string Y;
  string B0, C0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicNAND_new (Y, C0, outB, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI222_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 6);
  string Y;
  string B0, C0, B1, A1, A0, C1;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "C1") C1 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outC, outB;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicOR_new (outC, C0, C1);
  ComputeBasicNAND_new (Y, outA, outB, outC);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI22_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicOR_new (outA, A0, A1);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI2B11_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, C0, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "C0") C0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicOR_new (outA, A0, outA1);
  ComputeBasicNAND_new (Y, outA, B0, C0);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI2B1_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicOR_new (outA, A0, outA1);
  ComputeBasicNAND_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI2B2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA, outB;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicOR_new (outA, A0, outA1);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI2BB1_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string B0, A1N, A0N;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0N") A0N = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicNAND_new (outA, A0N, A1N);
  ComputeBasicNAND_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI2BB2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1N, A0N;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0N") A0N = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicNAND_new (outA, A0N, A1N);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI31_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, A1, A0, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA;
  ComputeBasicOR_new (outA, A0, A1, A2);
  ComputeBasicNAND_new (Y, B0, outA);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI32_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 5);
  string Y;
  string B0, B1, A1, A0, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicOR_new (outA, A0, A1, A2);
  ComputeBasicOR_new (outB, B0, B1);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOAI33_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 6);
  string Y;
  string B0, B1, A1, A0, B2, A2;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1") A1 = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else if ( fi_term_name == "B2") B2 = fi_term->getSimValue();
    else if ( fi_term_name == "A2") A2 = fi_term->getSimValue();
    else assert(0);
  }

  string outA, outB;
  ComputeBasicOR_new (outA, A0, A1, A2);
  ComputeBasicOR_new (outB, B0, B1, B2);
  ComputeBasicNAND_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOR2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicOR_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOR3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicOR_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValOR4_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string A, D, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "D") D = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicOR_new (Y, A, B, C, D);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValXNOR2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicXNOR_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValXNOR3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicXNOR_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValXOR2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 2);
  string Y;
  string A, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicXOR_new (Y, A, B);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValXOR3_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 3);
  string Y;
  string A, C, B;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "A") A = fi_term->getSimValue();
    else if ( fi_term_name == "C") C = fi_term->getSimValue();
    else if ( fi_term_name == "B") B = fi_term->getSimValue();
    else assert(0);
  }

  ;
  ComputeBasicXOR_new (Y, A, B, C);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAO2B2_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1") B1 = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA, outB;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicAND_new (outA, A0, outA1);
  ComputeBasicAND_new (outB, B0, B1);
  ComputeBasicOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValAO2B2B_new ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert (fanoutTerms.size() == 1);
  assert (faninTerms.size() == 4);
  string Y;
  string B0, B1N, A1N, A0;
  for ( int i = 0 ; i < faninTerms.size(); i++)
  {
    Terminal * fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if ( fi_term_name == "B0") B0 = fi_term->getSimValue();
    else if ( fi_term_name == "B1N") B1N = fi_term->getSimValue();
    else if ( fi_term_name == "A1N") A1N = fi_term->getSimValue();
    else if ( fi_term_name == "A0") A0 = fi_term->getSimValue();
    else assert(0);
  }

  string outA1, outA, outB1, outB;
  ComputeBasicINV_new (outA1, A1N);
  ComputeBasicAND_new (outA, A0, outA1);
  ComputeBasicINV_new (outB1, B1N);
  ComputeBasicAND_new (outB, B0, outB1);
  ComputeBasicOR_new (Y, outA, outB);

  assert(!Y.empty()); 
  bool toggled = false;
  for (int i  = 0; i < fanoutTerms.size(); i++)
  {
    Terminal * fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if ( fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }
  return toggled; 
}


bool Gate::ComputeValBUF_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 1);
  string Y;
  string A; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else assert(0);
  }

  Y = A;

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValBUFZ_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 2);
  string Y = "X";
  string A, OE;
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "OE") OE = fi_term->getSimValue();
    else assert(0);
  }

  if (OE == "1" || OE == "X") Y = A;

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValDLY4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 1);
  string Y;
  string A; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else assert(0);
  }

  Y = A;

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValPREICG_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 3);
  string ECK;
  string CK, E, SE;
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "CK") CK = fi_term->getSimValue();
    else if (fi_term_name == "E")  E  = fi_term->getSimValue();
    else if (fi_term_name == "SE") SE = fi_term->getSimValue();
    else assert(0);
  }

  // ASSUMING NON GLITCHY CIRCUITS. SO IGNORING EDGE 
  string EorSE;
  static string EorSE_prev = "0";
  ComputeBasicOR_new(EorSE, E, SE);
  if (CK == "1" || CK == "X")
  {
    ComputeBasicAND_new(ECK, CK, EorSE_prev);
  }
  else if (CK == "0")
  {
    ECK = "0";
  }

  EorSE_prev = EorSE;

  assert(!ECK.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "ECK") toggled |= fo_term->setSimValue(ECK, sim_wf);
    else assert(0);
  }

  return toggled;
}

bool Gate::ComputeValRFWL_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 2);
  string ECK;
  string CK, E;
  static string E_prev = "0";
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "CK") CK = fi_term->getSimValue();
    else if (fi_term_name == "E")  E  = fi_term->getSimValue();
    else assert(0);
  }

  // ASSUMING NON GLITCHY CIRCUITS. SO IGNORING EDGE 
  if (CK == "1" || CK == "X")
  {
    ComputeBasicAND_new(ECK, CK, E_prev);
  }
  else if (CK == "0")
  {
    ECK = "0";
  }

  E_prev = E;

  assert(!ECK.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "ECK") toggled |= fo_term->setSimValue(ECK, sim_wf);
    else assert(0);
  }

  return toggled;
}

bool Gate::ComputeValRF1R1WS_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 3);
  string RBL;
  string RWL, WWL, WBL;
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "RWL") RWL = fi_term->getSimValue();
    else if (fi_term_name == "WWL")  WWL  = fi_term->getSimValue();
    else if (fi_term_name == "WBL")  WBL  = fi_term->getSimValue();
    else assert(0);
  }

  bool toggled = false; ;

  if (RWL == "1" || RWL == "X")
  {
    assert ( WWL != "1");
    RBL = SRAM_stored_value;
    assert(!RBL.empty());
    for (int i = 0; i < fanoutTerms.size(); i++)
    {
      Terminal* fo_term = fanoutTerms[i];
      string fo_term_name = fo_term->getName();
      if (fo_term_name == "RBL") toggled |= fo_term->setSimValue(RBL, sim_wf);
      else assert(0);
    }
  }
  if ( WWL == "1" || WWL == "X" || WBL == "X")
  {
    assert ( RWL != "1" );
    SRAM_stored_value = WBL;
    toggled = false;
  }

  return toggled;
}


bool Gate::ComputeValLATCH_new   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 2);
  string Q;
  string D, G;
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "G") G = fi_term->getSimValue();
    else if (fi_term_name == "D")  D  = fi_term->getSimValue();
    else assert(0);
  }

  bool toggled = false; ;
  if (G == "1")
  {
    Q = D ;

    assert(!Q.empty());
    for (int i = 0; i < fanoutTerms.size(); i++)
    {
      Terminal* fo_term = fanoutTerms[i];
      string fo_term_name = fo_term->getName();
      if (fo_term_name == "Q") toggled |= fo_term->setSimValue(Q, sim_wf);
      else assert(0);
    }
  }

  return toggled;
}


bool Gate::ComputeValTIELO_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  string Y;

  Y = "0";

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValTIEHI_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  string Y;

  Y = "1";

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMX2_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 3);
  string Y;
  string A, B, S0; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else assert(0);
  }

  ComputeBasicMUX_new(Y, A, B, S0);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMX3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 5);
  string Y;
  string A, B, C,  S0, S1; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "C") C = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp1, tmp2;
  ComputeBasicMUX_new(tmp1, A, B, S0);
  ComputeBasicMUX_new(tmp2, C, C, S0);
  ComputeBasicMUX_new(Y, tmp1, tmp2, S1);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMX4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 6);
  string Y;
  string A, B, C, D, S0, S1; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "C") C = fi_term->getSimValue();
    else if (fi_term_name == "D") D = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp1, tmp2;
  ComputeBasicMUX_new(tmp1, A, B, S0);
  ComputeBasicMUX_new(tmp2, C, D, S0);
  ComputeBasicMUX_new(Y, tmp1, tmp2, S1);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMXI2_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 3);
  string Y;
  string A, B, S0; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp;
  ComputeBasicMUX_new(tmp, A, B, S0);
  ComputeBasicINV_new(Y, tmp);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMXI2D_new     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 3);
  string Y;
  string A, B, S0; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp;
  ComputeBasicMUX_new(tmp, A, B, S0);
  ComputeBasicINV_new(Y, tmp);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMXI3_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 5);
  string Y;
  string A, B, C,  S0, S1; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "C") C = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp1, tmp2, tmp3;
  ComputeBasicMUX_new(tmp1, A, B, S0);
  ComputeBasicMUX_new(tmp2, C, C, S0);
  ComputeBasicMUX_new(tmp3, tmp1, tmp2, S1);
  ComputeBasicINV_new(Y, tmp3);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

bool Gate::ComputeValMXI4_new      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
{
  assert(fanoutTerms.size() == 1);
  assert(faninTerms.size() == 6);
  string Y;
  string A, B, C, D, S0, S1; // there is a macro D being used
  for (int i = 0; i < faninTerms.size(); i++)
  {
    Terminal* fi_term = faninTerms[i];
    string fi_term_name = fi_term->getName();
    if (fi_term_name == "A") A = fi_term->getSimValue();
    else if (fi_term_name == "B") B = fi_term->getSimValue();
    else if (fi_term_name == "C") C = fi_term->getSimValue();
    else if (fi_term_name == "D") D = fi_term->getSimValue();
    else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
    else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
    else assert(0);
  }

  string tmp1, tmp2, tmp3;
  ComputeBasicMUX_new(tmp1, A, B, S0);
  ComputeBasicMUX_new(tmp2, C, D, S0);
  ComputeBasicMUX_new(tmp3, tmp1, tmp2, S1);
  ComputeBasicINV_new(Y, tmp3);

  assert(!Y.empty());
  bool toggled = false; ;
  for (int i = 0; i < fanoutTerms.size(); i++)
  {
    Terminal* fo_term = fanoutTerms[i];
    string fo_term_name = fo_term->getName();
    if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
    else assert(0);
  }

  return toggled;

}

    void Gate::print_gate_debug_info()
    {
        static int i = 0;
        gate_debug_file << i << " Gate is " << name << " type : " << cellName << endl;
        gate_debug_file << "Inputs are : " << endl;
        for (int i = 0; i < faninTerms.size(); i++)
        {
          Terminal* fi_term = faninTerms[i];
          string fi_term_name = fi_term->getName();
          string fi_term_sim_val = fi_term->getSimValue();
          gate_debug_file << fi_term_name << " : " << fi_term_sim_val << "  " ;
        }
        gate_debug_file << endl;
        gate_debug_file << "Outputs are : " << endl;
        for (int i = 0; i < fanoutTerms.size(); i++)
        {
          Terminal* fo_term = fanoutTerms[i];
          string fo_term_name = fo_term->getName();
          string fo_term_sim_val = fo_term->getSimValue();
          gate_debug_file << fo_term_name << " : " << fo_term_sim_val << "  " ;
        }
        gate_debug_file << endl;

        gate_debug_file << " -------------------------------------" << endl;
        i++;
    }

    bool Gate::ComputeValADDF     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 2);
      assert(faninTerms.size() == 3);
      string S, CO;
      string A, B, CI;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "CI") CI = fi_term->getSimValue();
        else assert(0);
      }

      // XOR3 on S
      string tmp;
      ComputeBasicXOR(A, B, tmp);
      ComputeBasicXOR(tmp, CI, S);

      // AND A, B
      string AandB;
      ComputeBasicAND(A,B, AandB);

      // AND A, CI
      string AandCI;
      ComputeBasicAND(A,CI, AandCI);

      // AND B, CI
      string BandCI;
      ComputeBasicAND(B,CI, BandCI);


      // OR for CO
      string temp;
      ComputeBasicOR(AandB, AandCI, temp);
      ComputeBasicOR(temp, BandCI, CO);

      assert(!S.empty() && !CO.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
        else if (fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
        else assert(0);
      }

      return toggled;
    }
    bool Gate::ComputeValADDFH    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 2);
      assert(faninTerms.size() == 3);
      string S, CO;
      string A, B, CI;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "CI") CI = fi_term->getSimValue();
        else assert(0);
      }

      // XOR3 on S
      string tmp;
      ComputeBasicXOR(A, B, tmp);
      ComputeBasicXOR(tmp, CI, S);

      // AND A, B
      string AandB;
      ComputeBasicAND(A,B, AandB);

      // AND A, CI
      string AandCI;
      ComputeBasicAND(A,CI, AandCI);

      // AND B, CI
      string BandCI;
      ComputeBasicAND(B,CI, BandCI);


      // OR for CO
      string temp;
      ComputeBasicOR(AandB, AandCI, temp);
      ComputeBasicOR(temp, BandCI, CO);

      assert(!S.empty() && !CO.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
        else if (fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValADDH     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 2);
      assert(faninTerms.size() == 2);
      string S, CO;
      string A, B;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      // XOR2 A, B => S
      ComputeBasicXOR(A, B, S);

      // AND A, B => CO
      ComputeBasicAND(A, B, CO);

      assert(!S.empty() && !CO.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "S") toggled |= fo_term->setSimValue(S, sim_wf);
        else if (fo_term_name == "CO") toggled |= fo_term->setSimValue(CO, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAND2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      // AND A, B => Y
      ComputeBasicAND(A, B, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAND3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      // AND A, B => Y
      string tmp;
      ComputeBasicAND(A, B, tmp);
      ComputeBasicAND(tmp, C, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAND4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      // AND A, B => Y
      string tmp1, tmp2;
      ComputeBasicAND(A, B, tmp1);
      ComputeBasicAND(C,D, tmp2);
      ComputeBasicAND(tmp1, tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAO21     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      // AND A, B => Y
      string tmp;
      ComputeBasicAND(A0, A1, tmp);
      ComputeBasicOR(tmp, B0, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAO22     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(B0, B1, tmp2);
      ComputeBasicOR(tmp1, tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAO2B2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1N, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicAND(A0, tmp1, tmp2);
      ComputeBasicAND(B0, B1, tmp3);
      ComputeBasicOR(tmp2, tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAO2B2B     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1N, B0, B1N; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1N") B1N = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicAND(A0, tmp1, tmp2);
      ComputeBasicINV(B1N, tmp3);
      ComputeBasicAND(B0, tmp3, tmp4);
      ComputeBasicOR(tmp2, tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::ComputeValAOI21    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicOR(tmp1, B0, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValAOI211   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, C0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicOR(tmp1, B0, tmp2);
      ComputeBasicOR(tmp2, C0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValAOI21B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0N; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0N") B0N = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicINV(B0N, tmp2);
      ComputeBasicOR(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValAOI22    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(B0, B1, tmp2);
      ComputeBasicOR(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI221   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A0, A1, B0, B1, C0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(B0, B1, tmp2);
      ComputeBasicOR(tmp1, tmp2, tmp3);
      ComputeBasicOR(tmp3, C0, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI222   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 6);
      string Y;
      string A0, A1, B0, B1, C0, C1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else if (fi_term_name == "C1") C1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(B0, B1, tmp2);
      ComputeBasicAND(C0, C1, tmp3);
      ComputeBasicOR(tmp1, tmp2, tmp4);
      ComputeBasicOR(tmp3, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI2B1   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1N, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicAND(A0, tmp1, tmp2);
      ComputeBasicOR(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI2BB1  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0N, A1N, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0N") A0N = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0N, A1N, tmp1);
      ComputeBasicINV(tmp1, tmp2); // Apply DeMorgan's
      ComputeBasicOR(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI2BB2  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0N, A1N, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0N") A0N = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0N, A1N, tmp1);
      ComputeBasicINV(tmp1, tmp2);
      ComputeBasicAND(B0, B1, tmp3);
      ComputeBasicOR(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValAOI31    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, A2, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(tmp1, A2, tmp2);
      ComputeBasicOR(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValAOI32    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A0, A1, A2, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(tmp1, A2, tmp2);
      ComputeBasicAND(B0, B1, tmp3);
      ComputeBasicOR(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValAOI33    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 6);
      string Y;
      string A0, A1, A2, B0, B1, B2; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "B2") B2 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicAND(A0, A1, tmp1);
      ComputeBasicAND(tmp1, A2, tmp2);
      ComputeBasicAND(B0, B1, tmp3);
      ComputeBasicAND(tmp3, B2, tmp4);
      ComputeBasicOR(tmp2, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValBUF      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 1);
      string Y;
      string A; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else assert(0);
      }

      Y = A;

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValINV      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 1);
      string Y;
      string A; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else assert(0);
      }

      ComputeBasicINV(A, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValMX2      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, S0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else assert(0);
      }

      ComputeBasicMUX(A, B, S0, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMX3      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A, B, C,  S0, S1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicMUX(A, B, S0, tmp1);
      ComputeBasicMUX(C, C, S0, tmp2);
      ComputeBasicMUX(tmp1, tmp2, S1, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMX4      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 6);
      string Y;
      string A, B, C, D, S0, S1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicMUX(A, B, S0, tmp1);
      ComputeBasicMUX(C, D, S0, tmp2);
      ComputeBasicMUX(tmp1, tmp2, S1, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMXI2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, S0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp;
      ComputeBasicMUX(A, B, S0, tmp);
      ComputeBasicINV(tmp, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMXI2D     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, S0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp;
      ComputeBasicMUX(A, B, S0, tmp);
      ComputeBasicINV(tmp, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMXI3      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A, B, C,  S0, S1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicMUX(A, B, S0, tmp1);
      ComputeBasicMUX(C, C, S0, tmp2);
      ComputeBasicMUX(tmp1, tmp2, S1, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValMXI4      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 6);
      string Y;
      string A, B, C, D, S0, S1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else if (fi_term_name == "S0") S0 = fi_term->getSimValue();
        else if (fi_term_name == "S1") S1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicMUX(A, B, S0, tmp1);
      ComputeBasicMUX(C, D, S0, tmp2);
      ComputeBasicMUX(tmp1, tmp2, S1, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValNAND2    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      string tmp;
      ComputeBasicAND(A, B, tmp);
      ComputeBasicINV(tmp, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNAND2B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string AN, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicAND(tmp1, B, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNAND3    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicAND(A, B, tmp1);
      ComputeBasicAND(tmp1, C, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }
    bool Gate::ComputeValNAND3B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string AN, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicAND(tmp1, B, tmp2);
      ComputeBasicAND(tmp2, C, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNAND4    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicAND(A, B, tmp1);
      ComputeBasicAND(C, D, tmp2);
      ComputeBasicAND(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNAND4B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string AN, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicAND(tmp1, B, tmp2);
      ComputeBasicAND(C, D, tmp3);
      ComputeBasicAND(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValNAND4BB   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string AN, BN, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "BN") BN = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicINV(BN, tmp2);
      ComputeBasicAND(tmp1, tmp2, tmp3);
      ComputeBasicAND(C, D, tmp4);
      ComputeBasicAND(tmp3, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValNOR2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B; // there is a macro D being used
      if (name == "U5638")
      {
        gate_debug_file << "Name : " << name << endl;
      }
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1;
      ComputeBasicOR(A, B, tmp1);
      ComputeBasicINV(tmp1, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNOR2B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string AN, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicOR(tmp1, B, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNOR3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicOR(A, B, tmp1);
      ComputeBasicOR(C, tmp1, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNOR3B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string AN, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicOR(tmp1, B, tmp2);
      ComputeBasicOR(tmp2, C, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNOR4     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3;
      ComputeBasicOR(A, B, tmp1);
      ComputeBasicOR(C, D, tmp2);
      ComputeBasicOR(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValNOR4B    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string AN, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicOR(tmp1, B, tmp2);
      ComputeBasicOR(C, D, tmp3);
      ComputeBasicOR(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValNOR4BB    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string AN, BN, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "AN") AN = fi_term->getSimValue();
        else if (fi_term_name == "BN") BN = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicINV(AN, tmp1);
      ComputeBasicINV(BN, tmp2);
      ComputeBasicOR(tmp1, tmp2, tmp3);
      ComputeBasicOR(C, D, tmp4);
      ComputeBasicOR(tmp3, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValOA21     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicAND(tmp1, B0, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOA22     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(B0, B1, tmp2);
      ComputeBasicAND(tmp1, tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI21    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicAND(tmp1, B0, tmp2);
      ComputeBasicINV(tmp2, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI211   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, C0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicAND(tmp1, B0, tmp2);
      ComputeBasicAND(tmp2, C0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI21B   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1, B0N; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0N") B0N = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicINV(B0N, tmp1);
      ComputeBasicOR(A0, A1, tmp2);
      ComputeBasicAND(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI22    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(B0, B1, tmp2);
      ComputeBasicAND(tmp1, tmp2, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }

    bool Gate::ComputeValOAI221    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A0, A1, B0, B1, C0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(B0, B1, tmp2);
      ComputeBasicAND(tmp1, tmp2, tmp3);
      ComputeBasicAND(tmp3, C0, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI222   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 6);
      string Y;
      string A0, A1, B0, B1, C0, C1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else if (fi_term_name == "C1") C1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(B0, B1, tmp2);
      ComputeBasicOR(B0, B1, tmp3);
      ComputeBasicAND(tmp1, tmp2, tmp4);
      ComputeBasicAND(tmp3, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI2B1   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0, A1N, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicOR(A0, tmp1, tmp2);
      ComputeBasicAND(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI2B11  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1N, B0, C0;
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "C0") C0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicOR(A0, tmp1, tmp2);
      ComputeBasicAND(tmp2, B0, tmp3);
      ComputeBasicAND(tmp3, C0, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI2B2   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1N, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicINV(A1N, tmp1);
      ComputeBasicOR(A0, tmp1, tmp2);
      ComputeBasicOR(B0, B1, tmp3);
      ComputeBasicAND(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI2BB1  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A0N, A1N, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0N") A0N = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicAND(A0N, A1N, tmp1);
      ComputeBasicINV(tmp1, tmp2);
      ComputeBasicAND(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI2BB2  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0N, A1N, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0N") A0N = fi_term->getSimValue();
        else if (fi_term_name == "A1N") A1N = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicAND(A0N, A1N, tmp1);
      ComputeBasicINV(tmp1, tmp2);
      ComputeBasicOR(B0, B1, tmp3);
      ComputeBasicAND(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOAI31    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A0, A1, A2, B0; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(tmp1, A2, tmp2);
      ComputeBasicAND(tmp2, B0, tmp3);
      ComputeBasicINV(tmp3, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;


    }
    bool Gate::ComputeValOAI32    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A0, A1, A2, B0, B1; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(tmp1, A2, tmp2);
      ComputeBasicOR(B0, B1, tmp3);
      ComputeBasicAND(tmp2, tmp3, tmp4);
      ComputeBasicINV(tmp4, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;


    }

    bool Gate::ComputeValOAI33    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 5);
      string Y;
      string A0, A1, A2, B0, B1, B2; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A0") A0 = fi_term->getSimValue();
        else if (fi_term_name == "A1") A1 = fi_term->getSimValue();
        else if (fi_term_name == "A2") A2 = fi_term->getSimValue();
        else if (fi_term_name == "B0") B0 = fi_term->getSimValue();
        else if (fi_term_name == "B1") B1 = fi_term->getSimValue();
        else if (fi_term_name == "B2") B2 = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2, tmp3, tmp4, tmp5;
      ComputeBasicOR(A0, A1, tmp1);
      ComputeBasicOR(tmp1, A2, tmp2);
      ComputeBasicOR(B0, B1, tmp3);
      ComputeBasicOR(tmp3, B2, tmp4);
      ComputeBasicAND(tmp2, tmp4, tmp5);
      ComputeBasicINV(tmp5, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;


    }
    bool Gate::ComputeValOR2      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      ComputeBasicOR(A, B, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOR3      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp;
      ComputeBasicOR(A, B, tmp);
      ComputeBasicOR(tmp, C, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValOR4      ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 4);
      string Y;
      string A, B, C, D; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else if (fi_term_name == "D") D = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicOR(A, B, tmp1);
      ComputeBasicOR(C, D, tmp2);
      ComputeBasicOR(tmp1, tmp2,  Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValSDFF     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);
//      assert(fanoutTerms.size() == 1);
//      assert(faninTerms.size() == 4);

    }
    bool Gate::ComputeValSDFFH    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFHQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFQ    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFR    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFRHQ  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFRQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFS    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFSHQ  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValSDFFSQ   ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValTLATNTSCA( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(0);

    }
    bool Gate::ComputeValXNOR2    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicXOR(A, B, tmp1);
      ComputeBasicINV(tmp1,  Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }
    bool Gate::ComputeValXNOR3    ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp1, tmp2;
      ComputeBasicXOR(A, B, tmp1);
      ComputeBasicXOR(tmp1, C, tmp2);
      ComputeBasicINV(tmp2,  Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;

    }
    bool Gate::ComputeValXOR2     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 2);
      string Y;
      string A, B; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else assert(0);
      }

      ComputeBasicXOR(A, B, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }
    bool Gate::ComputeValXOR3     ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
      assert(faninTerms.size() == 3);
      string Y;
      string A, B, C; // there is a macro D being used
      for (int i = 0; i < faninTerms.size(); i++)
      {
        Terminal* fi_term = faninTerms[i];
        string fi_term_name = fi_term->getName();
        if (fi_term_name == "A") A = fi_term->getSimValue();
        else if (fi_term_name == "B") B = fi_term->getSimValue();
        else if (fi_term_name == "C") C = fi_term->getSimValue();
        else assert(0);
      }

      string tmp;
      ComputeBasicXOR(A, B, tmp);
      ComputeBasicXOR(tmp, C, Y);

      assert(!Y.empty());
      bool toggled = false; ;
      for (int i = 0; i < fanoutTerms.size(); i++)
      {
        Terminal* fo_term = fanoutTerms[i];
        string fo_term_name = fo_term->getName();
        if (fo_term_name == "Y") toggled |= fo_term->setSimValue(Y, sim_wf);
        else assert(0);
      }

      return toggled;
    }

    bool Gate::computeValOne_new(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      switch (func)
      {
         case ADDF      : { toggled = ComputeValADDF_new      ( sim_wf); break; }
         case ADDFH     : { toggled = ComputeValADDFH_new     ( sim_wf); break; }
         case ADDH      : { toggled = ComputeValADDH_new      ( sim_wf); break; }
         case AND2      : { toggled = ComputeValAND2_new      ( sim_wf); break; }
         case AND3      : { toggled = ComputeValAND3_new      ( sim_wf); break; }
         case AND4      : { toggled = ComputeValAND4_new      ( sim_wf); break; }
         case AO21      : { toggled = ComputeValAO21_new      ( sim_wf); break; }
         case AO22      : { toggled = ComputeValAO22_new      ( sim_wf); break; }
         case AO2B2     : { toggled = ComputeValAO2B2_new     ( sim_wf); break; }
         case AO2B2B    : { toggled = ComputeValAO2B2B_new    ( sim_wf); break; }
         case AOI21     : { toggled = ComputeValAOI21_new     ( sim_wf); break; }
         case AOI211    : { toggled = ComputeValAOI211_new    ( sim_wf); break; }
         case AOI21B    : { toggled = ComputeValAOI21B_new    ( sim_wf); break; }
         case AOI22     : { toggled = ComputeValAOI22_new     ( sim_wf); break; }
         case AOI221    : { toggled = ComputeValAOI221_new    ( sim_wf); break; }
         case AOI222    : { toggled = ComputeValAOI222_new    ( sim_wf); break; }
         case AOI2B1    : { toggled = ComputeValAOI2B1_new    ( sim_wf); break; }
         case AOI2BB1   : { toggled = ComputeValAOI2BB1_new   ( sim_wf); break; }
         case AOI2BB2   : { toggled = ComputeValAOI2BB2_new   ( sim_wf); break; }
         case AOI31     : { toggled = ComputeValAOI31_new     ( sim_wf); break; }
         case AOI32     : { toggled = ComputeValAOI32_new     ( sim_wf); break; }
         case AOI33     : { toggled = ComputeValAOI33_new     ( sim_wf); break; }
         case BUF       : { toggled = ComputeValBUF_new       ( sim_wf); break; }
         case BUFZ      : { toggled = ComputeValBUFZ_new      ( sim_wf); break; }
         case DLY4      : { toggled = ComputeValDLY4_new      ( sim_wf); break; }
         case INV       : { toggled = ComputeValINV_new       ( sim_wf); break; }
         case LATCH     : { toggled = ComputeValLATCH_new     ( sim_wf); break; }
         case MX2       : { toggled = ComputeValMX2_new       ( sim_wf); break; }
         case MX3       : { toggled = ComputeValMX3_new       ( sim_wf); break; }
         case MX4       : { toggled = ComputeValMX4_new       ( sim_wf); break; }
         case MXI2      : { toggled = ComputeValMXI2_new      ( sim_wf); break; }
         case MXI2D     : { toggled = ComputeValMXI2D_new     ( sim_wf); break; }
         case MXI3      : { toggled = ComputeValMXI3_new      ( sim_wf); break; }
         case MXI4      : { toggled = ComputeValMXI4_new      ( sim_wf); break; }
         case NAND2     : { toggled = ComputeValNAND2_new     ( sim_wf); break; }
         case NAND2B    : { toggled = ComputeValNAND2B_new    ( sim_wf); break; }
         case NAND3     : { toggled = ComputeValNAND3_new     ( sim_wf); break; }
         case NAND3B    : { toggled = ComputeValNAND3B_new    ( sim_wf); break; }
         case NAND4     : { toggled = ComputeValNAND4_new     ( sim_wf); break; }
         case NAND4B    : { toggled = ComputeValNAND4B_new    ( sim_wf); break; }
         case NAND4BB   : { toggled = ComputeValNAND4BB_new   ( sim_wf); break; }
         case NOR2      : { toggled = ComputeValNOR2_new      ( sim_wf); break; }
         case NOR2B     : { toggled = ComputeValNOR2B_new     ( sim_wf); break; }
         case NOR3      : { toggled = ComputeValNOR3_new      ( sim_wf); break; }
         case NOR3B     : { toggled = ComputeValNOR3B_new     ( sim_wf); break; }
         case NOR4      : { toggled = ComputeValNOR4_new      ( sim_wf); break; }
         case NOR4B     : { toggled = ComputeValNOR4B_new     ( sim_wf); break; }
         case NOR4BB    : { toggled = ComputeValNOR4BB_new    ( sim_wf); break; }
         case PREICG    : { toggled = ComputeValPREICG_new    ( sim_wf); break; }
         case RFWL      : { toggled = ComputeValRFWL_new      ( sim_wf); break; }
         case RF1R1WS   : { toggled = ComputeValRF1R1WS_new   ( sim_wf); break; }
         case OA21      : { toggled = ComputeValOA21_new      ( sim_wf); break; }
         case OA22      : { toggled = ComputeValOA22_new      ( sim_wf); break; }
         case OAI21     : { toggled = ComputeValOAI21_new     ( sim_wf); break; }
         case OAI211    : { toggled = ComputeValOAI211_new    ( sim_wf); break; }
         case OAI21B    : { toggled = ComputeValOAI21B_new    ( sim_wf); break; }
         case OAI22     : { toggled = ComputeValOAI22_new     ( sim_wf); break; }
         case OAI221    : { toggled = ComputeValOAI221_new    ( sim_wf); break; }
         case OAI222    : { toggled = ComputeValOAI222_new    ( sim_wf); break; }
         case OAI2B1    : { toggled = ComputeValOAI2B1_new    ( sim_wf); break; }
         case OAI2B11   : { toggled = ComputeValOAI2B11_new   ( sim_wf); break; }
         case OAI2B2    : { toggled = ComputeValOAI2B2_new    ( sim_wf); break; }
         case OAI2BB1   : { toggled = ComputeValOAI2BB1_new   ( sim_wf); break; }
         case OAI2BB2   : { toggled = ComputeValOAI2BB2_new   ( sim_wf); break; }
         case OAI31     : { toggled = ComputeValOAI31_new     ( sim_wf); break; }
         case OAI32     : { toggled = ComputeValOAI32_new     ( sim_wf); break; }
         case OAI33     : { toggled = ComputeValOAI33_new     ( sim_wf); break; }
         case OR2       : { toggled = ComputeValOR2_new       ( sim_wf); break; }
         case OR3       : { toggled = ComputeValOR3_new       ( sim_wf); break; }
         case OR4       : { toggled = ComputeValOR4_new       ( sim_wf); break; }
//         case SDFF      : { toggled = ComputeValSDFF_new      ( sim_wf); break; }
//         case SDFFH     : { toggled = ComputeValSDFFH_new     ( sim_wf); break; }
//         case SDFFHQ    : { toggled = ComputeValSDFFHQ_new    ( sim_wf); break; }
//         case SDFFQ     : { toggled = ComputeValSDFFQ_new     ( sim_wf); break; }
//         case SDFFR     : { toggled = ComputeValSDFFR_new     ( sim_wf); break; }
//         case SDFFRHQ   : { toggled = ComputeValSDFFRHQ_new   ( sim_wf); break; }
//         case SDFFRQ    : { toggled = ComputeValSDFFRQ_new    ( sim_wf); break; }
//         case SDFFS     : { toggled = ComputeValSDFFS_new     ( sim_wf); break; }
//         case SDFFSHQ   : { toggled = ComputeValSDFFSHQ_new   ( sim_wf); break; }
//         case SDFFSQ    : { toggled = ComputeValSDFFSQ_new    ( sim_wf); break; }
//         case TLATNTSCA : { toggled = ComputeValTLATNTSCA_new ( sim_wf); break; }
         case TIELO     : { toggled = ComputeValTIELO_new     ( sim_wf); break; }
         case TIEHI     : { toggled = ComputeValTIEHI_new     ( sim_wf); break; }
         case XNOR2     : { toggled = ComputeValXNOR2_new     ( sim_wf); break; }
         case XNOR3     : { toggled = ComputeValXNOR3_new     ( sim_wf); break; }
         case XOR2      : { toggled = ComputeValXOR2_new      ( sim_wf); break; }
         case XOR3      : { toggled = ComputeValXOR3_new      ( sim_wf); break; }
         default : assert(0);
      }

         //print_gate_debug_info();
         return toggled;
    }

    bool Gate::computeValOne(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      return computeValOne_new(sim_wf);
/*      if (name == "u_top/u_sys/u_core/U92")
      {
        cout << "Gate is " << name << endl;
      }*/

      switch (func)
         {
           case ADDF      : { toggled = ComputeValADDF      (sim_wf); break; }
           case ADDFH     : { toggled = ComputeValADDFH     (sim_wf); break; }
           case ADDH      : { toggled = ComputeValADDH      (sim_wf); break; }
           case AND2      : { toggled = ComputeValAND2      (sim_wf); break; }
           case AND3      : { toggled = ComputeValAND3      (sim_wf); break; }
           case AND4      : { toggled = ComputeValAND4      (sim_wf); break; }
           case AO21      : { toggled = ComputeValAO21      (sim_wf); break; }
           case AO22      : { toggled = ComputeValAO22      (sim_wf); break; }
           case AO2B2     : { toggled = ComputeValAO2B2     (sim_wf); break; }
           case AO2B2B    : { toggled = ComputeValAO2B2B    (sim_wf); break; }
           case AOI21     : { toggled = ComputeValAOI21     (sim_wf); break; }
           case AOI211    : { toggled = ComputeValAOI211    (sim_wf); break; }
           case AOI21B    : { toggled = ComputeValAOI21B    (sim_wf); break; }
           case AOI22     : { toggled = ComputeValAOI22     (sim_wf); break; }
           case AOI221    : { toggled = ComputeValAOI221    (sim_wf); break; }
           case AOI222    : { toggled = ComputeValAOI222    (sim_wf); break; }
           case AOI2B1    : { toggled = ComputeValAOI2B1    (sim_wf); break; }
           case AOI2BB1   : { toggled = ComputeValAOI2BB1   (sim_wf); break; }
           case AOI2BB2   : { toggled = ComputeValAOI2BB2   (sim_wf); break; }
           case AOI31     : { toggled = ComputeValAOI31     (sim_wf); break; }
           case AOI32     : { toggled = ComputeValAOI32     (sim_wf); break; }
           case AOI33     : { toggled = ComputeValAOI33     (sim_wf); break; }
           case BUF       : { toggled = ComputeValBUF       (sim_wf); break; }
           case BUFZ      : { toggled = ComputeValBUFZ      (sim_wf); break; }
           case DLY4      : { toggled = ComputeValDLY4      (sim_wf); break; }
           case DFFQ      : { toggled = ComputeValDFFQ      (sim_wf); break; }
           case DFFRPQ    : { toggled = ComputeValDFFRPQ    (sim_wf); break; }
           case INV       : { toggled = ComputeValINV       (sim_wf); break; }
           case LATCH     : { toggled = ComputeValLATCH     (sim_wf); break; }
           case MX2       : { toggled = ComputeValMX2       (sim_wf); break; }
           case MX3       : { toggled = ComputeValMX3       (sim_wf); break; }
           case MX4       : { toggled = ComputeValMX4       (sim_wf); break; }
           case MXI2      : { toggled = ComputeValMXI2      (sim_wf); break; }
           case MXI2D     : { toggled = ComputeValMXI2D     (sim_wf); break; }
           case MXI3      : { toggled = ComputeValMXI3      (sim_wf); break; }
           case MXI4      : { toggled = ComputeValMXI4      (sim_wf); break; }
           case NAND2     : { toggled = ComputeValNAND2     (sim_wf); break; }
           case NAND2B    : { toggled = ComputeValNAND2B    (sim_wf); break; }
           case NAND3     : { toggled = ComputeValNAND3     (sim_wf); break; }
           case NAND3B    : { toggled = ComputeValNAND3B    (sim_wf); break; }
           case NAND4     : { toggled = ComputeValNAND4     (sim_wf); break; }
           case NAND4B    : { toggled = ComputeValNAND4B    (sim_wf); break; }
           case NAND4BB   : { toggled = ComputeValNAND4BB   (sim_wf); break; }
           case NOR2      : { toggled = ComputeValNOR2      (sim_wf); break; }
           case NOR2B     : { toggled = ComputeValNOR2B     (sim_wf); break; }
           case NOR3      : { toggled = ComputeValNOR3      (sim_wf); break; }
           case NOR3B     : { toggled = ComputeValNOR3B     (sim_wf); break; }
           case NOR4      : { toggled = ComputeValNOR4      (sim_wf); break; }
           case NOR4B     : { toggled = ComputeValNOR4B     (sim_wf); break; }
           case NOR4BB    : { toggled = ComputeValNOR4BB    (sim_wf); break; }
           case OA21      : { toggled = ComputeValOA21      (sim_wf); break; }
           case OA22      : { toggled = ComputeValOA22      (sim_wf); break; }
           case OAI21     : { toggled = ComputeValOAI21     (sim_wf); break; }
           case OAI211    : { toggled = ComputeValOAI211    (sim_wf); break; }
           case OAI21B    : { toggled = ComputeValOAI21B    (sim_wf); break; }
           case OAI22     : { toggled = ComputeValOAI22     (sim_wf); break; }
           case OAI221    : { toggled = ComputeValOAI221    (sim_wf); break; }
           case OAI222    : { toggled = ComputeValOAI222    (sim_wf); break; }
           case OAI2B1    : { toggled = ComputeValOAI2B1    (sim_wf); break; }
           case OAI2B11   : { toggled = ComputeValOAI2B11   (sim_wf); break; }
           case OAI2B2    : { toggled = ComputeValOAI2B2    (sim_wf); break; }
           case OAI2BB1   : { toggled = ComputeValOAI2BB1   (sim_wf); break; }
           case OAI2BB2   : { toggled = ComputeValOAI2BB2   (sim_wf); break; }
           case OAI31     : { toggled = ComputeValOAI31     (sim_wf); break; }
           case OAI32     : { toggled = ComputeValOAI32     (sim_wf); break; }
           case OAI33     : { toggled = ComputeValOAI33     (sim_wf); break; }
           case OR2       : { toggled = ComputeValOR2       (sim_wf); break; }
           case OR3       : { toggled = ComputeValOR3       (sim_wf); break; }
           case OR4       : { toggled = ComputeValOR4       (sim_wf); break; }
           case PREICG    : { toggled = ComputeValPREICG    (sim_wf); break; }
           case RF1R1WS   : { toggled = ComputeValRF1R1WS   (sim_wf); break; }
           case RFWL      : { toggled = ComputeValRFWL      (sim_wf); break; }
           case SDFF      : { toggled = ComputeValSDFF      (sim_wf); break; }
           case SDFFH     : { toggled = ComputeValSDFFH     (sim_wf); break; }
           case SDFFHQ    : { toggled = ComputeValSDFFHQ    (sim_wf); break; }
           case SDFFQ     : { toggled = ComputeValSDFFQ     (sim_wf); break; }
           case SDFFR     : { toggled = ComputeValSDFFR     (sim_wf); break; }
           case SDFFRHQ   : { toggled = ComputeValSDFFRHQ   (sim_wf); break; }
           case SDFFRPQ   : { toggled = ComputeValSDFFRPQ   (sim_wf); break; }
           case SDFFRQ    : { toggled = ComputeValSDFFRQ    (sim_wf); break; }
           case SDFFS     : { toggled = ComputeValSDFFS     (sim_wf); break; }
           case SDFFSHQ   : { toggled = ComputeValSDFFSHQ   (sim_wf); break; }
           case SDFFSQ    : { toggled = ComputeValSDFFSQ    (sim_wf); break; }
           case TLATNTSCA : { toggled = ComputeValTLATNTSCA (sim_wf); break; }
           case XNOR2     : { toggled = ComputeValXNOR2     (sim_wf); break; }
           case XNOR3     : { toggled = ComputeValXNOR3     (sim_wf); break; }
           case XOR2      : { toggled = ComputeValXOR2      (sim_wf); break; }
           case XOR3      : { toggled = ComputeValXOR3      (sim_wf); break; }
            default : assert(0);
         }

         print_gate_debug_info();
         return toggled;
    }

    bool Gate::allInpNetsVisited()
    {
       for (int i = 0; i < faninTerms.size(); i++)
        {
            Terminal* fanin_term = faninTerms[i];
            Net* fanin_net = fanin_term->getNet(0);
            if (fanin_net->getTopoSortMarked() == false)
              return false;
        }
        return true;
    }

    void Gate::getDriverTopoIds(vector<int>& topo_ids)
    {
        for(int i = 0; i < faninTerms.size(); i++)
        {
            Net* net = faninTerms[i]->getNet(0);
            Gate* gate = net->getDriverGate();
            int id;
            if (gate != NULL) id = gate->getTopoId();
            else if (net->getPadNum() == 0) {// net has no driver (pad or gate) so it must be a forced constant (net_name == terminal_name)
              assert (net->getName() == net->getTerminal(0)->getFullName());
              id = -1;
            }
            else {
              id = net->getPad(0)->getTopoId();
            }

            topo_ids.push_back(id);
        }
    }

    bool Gate::check_for_dead_toggle(int cycle_num)
    {
      //cout << "Checking for dead toggle for gate " << name << endl;
      if (!toggled) return false;
      if (visited && dead_toggle) return false; // returning false will ensure that we won't trace back this gate again.
      bool all_fanout_dead = true;
      for(int i = 0; i < fanout.size(); i++)
      {
        Gate* fanout_gate = fanout[i];
        if (fanout_gate->isToggled() && !(fanout_gate->isDeadToggle())) // toggled and is not a dead toggle
          all_fanout_dead = false;
        if (fanout_gate->getFFFlag()) all_fanout_dead = false;
      }
      if (all_fanout_dead && !isClkTree)
        gate_debug_file << " Gate " << name << " is dead_ended in cycle " << cycle_num << endl;
      return all_fanout_dead;
    }


    void Gate::trace_back_dead_gate(int& dead_gates_count, int cycle_num)
    {
      assert(dead_toggle);
      if (visited) return; // returning false will ensure that we won't trace back this gate again.
      visited = true;
      if (isClkTree == true) return;
      dead_gates_count ++;
      //cout << "Tracing back dead gate " << name << endl;
      for (int i = 0; i < fanin.size(); i++)
      {
        Gate* fanin_gate = fanin[i];
        if (fanin_gate->getFFFlag()) continue;
        bool dead_gate = fanin_gate->check_for_dead_toggle(cycle_num);
        if (dead_gate)
        {
          fanin_gate->setDeadToggle(true);
          fanin_gate->trace_back_dead_gate(dead_gates_count, cycle_num);
        }
      }
    }

}
