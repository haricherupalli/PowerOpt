#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <limits.h>
#include <bitset>
#include <stdlib.h>

#include "Gate.h"
#include "Terminal.h"
#include "Path.h"
#include "Pad.h"


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

std::ofstream gate_debug_file ("PowerOpt/gate_debug_file", std::ofstream::out);

int Gate::max_toggle_profile_size = 0;

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

        file << name << " --> ";
        for (int i = 0; i < toggle_profile.size(); i++)
        {
          file << toggle_profile[i].to_string() << ",";
        }
        file << endl;
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
     
    void Gate::transferDtoQ(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(FFFlag);
        if (func == DFF)
        {
          string Dval,SetVal, ClrVal;
          for(int i = 0; i < faninTerms.size(); i++)
          {
            Terminal* term = faninTerms[i];

            if (term->getName() == "D")  Dval = term->getSimValue();
            else if (term->getName() == "SDN")  SetVal = term->getSimValue();
            else if (term->getName() == "CDN")  ClrVal = term->getSimValue();
          }
          Terminal* fo_term = fanoutTerms[0];
          assert(fo_term->getName() == "Q");
          if (ClrVal == "0") 
          { 
            fo_term->setSimValue("0", sim_wf);
          }
          else if (SetVal == "0")
          {
            fo_term->setSimValue("1", sim_wf);
          }
          else
          {
            fo_term->setSimValue(Dval, sim_wf);
          }
        }
        if (func == LH)
        {
          string Dval,EnVal;
          for(int i = 0; i < faninTerms.size(); i++)
          {
            Terminal* term = faninTerms[i];

            if (term->getName() == "D")  Dval = term->getSimValue();
            else if (term->getName() == "E")  EnVal = term->getSimValue();
          }
          Terminal* fo_term = fanoutTerms[0];
          assert(fo_term->getName() == "Q");
          if (EnVal == "0") 
          {
            fo_term->setSimValue("0", sim_wf);
          }
          else 
          {
            fo_term->setSimValue(Dval, sim_wf);
          }
        }
    }

    void Gate::computeVal(priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      switch (func)
      {
        case AND  :  computeANDVal ( sim_wf ) ; break ; 
        case AOI  :  computeAOIVal ( sim_wf ) ; break ; 
        case BUFF :  computeBUFFVal( sim_wf ) ; break ; 
        case DFF  :  computeDFFVal ( sim_wf ) ; break ; 
        case INV  :  computeINVVal ( sim_wf ) ; break ; 
        case LH   :  computeLHVal  ( sim_wf ) ; break ; 
        case MUX  :  computeMUXVal ( sim_wf ) ; break ; 
        case NAND :  computeNANDVal( sim_wf ) ; break ; 
        case NOR  :  computeNORVal ( sim_wf ) ; break ; 
        case OAI  :  computeOAIVal ( sim_wf ) ; break ; 
        case OR   :  computeORVal  ( sim_wf ) ; break ; 
        case XNOR :  computeXNORVal( sim_wf ) ; break ; 
        case XOR  :  computeXORVal ( sim_wf ) ; break ; 
        default   :  assert(0);
      }

    }

    void Gate::computeANDVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "0")
        {
            fo_term->setSimValue("0", sim_wf);
        }
        if (fi_0_val == "1")
        {
            //fo_term->setSimValue(fi_1_val, sim_wf); // simpler version
            if (fi_1_val == "0") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "0") fo_term->setSimValue("0", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
    } 
    void Gate::computeAOIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
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

      if (B_val == "0")
      {
        if (A1_val == "0")
        {
            fo_term->setSimValue("1", sim_wf);
        }
        if (A1_val == "1")
        {
            if (A2_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (A2_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (A2_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (A1_val == "X")
        {
            if (A2_val == "0") fo_term->setSimValue("1", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
      }
      else if (B_val == "1")
      {
        fo_term->setSimValue("0", sim_wf);
      }
      else if (B_val == "X")
      {
        if(A1_val == "1" && A2_val == "1") fo_term->setSimValue("0", sim_wf);
        else fo_term->setSimValue("X", sim_wf);
      }
    } 
    void Gate::computeBUFFVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 1);
        Terminal* fo_term = fanoutTerms[0]; 
        Terminal* fi_term = faninTerms[0]; 
        fo_term->setSimValue(fi_term->getSimValue(), sim_wf);
    } 
    void Gate::computeDFFVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf){ } 
    void Gate::computeINVVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 1);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_val = faninTerms[0]->getSimValue(); 
        if (fi_val == "0") fo_term->setSimValue("1", sim_wf);
        else if (fi_val == "1") fo_term->setSimValue("0", sim_wf);
        else if (fi_val == "X") fo_term->setSimValue("X", sim_wf);
    } 
    void Gate::computeLHVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf){ } 
    void Gate::computeMUXVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
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
      if (S_val == "0") fo_term->setSimValue(I0_val, sim_wf);
      else if (S_val == "1") fo_term->setSimValue(I1_val, sim_wf);
      else if (S_val == "X") fo_term->setSimValue("X", sim_wf);
    } 
    void Gate::computeNANDVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "0")
        {
            fo_term->setSimValue("1", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "0") fo_term->setSimValue("1", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
    } 
    void Gate::computeNORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "1")
        {
            fo_term->setSimValue("0", sim_wf);
        }
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "1") fo_term->setSimValue("0", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
    } 
    void Gate::computeOAIVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
      assert(fanoutTerms.size() == 1);
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

      if (B_val == "1")
      {
        if (A1_val == "1")
        {
            fo_term->setSimValue("0", sim_wf);
        }
        if (A1_val == "0")
        {
            if (A2_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (A2_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (A2_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (A1_val == "X")
        {
            if (A2_val == "1") fo_term->setSimValue("0", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
      }
      else if (B_val == "0")
      {
        fo_term->setSimValue("1", sim_wf);
      }
      else if (B_val == "X")
      {
        if(A1_val == "0" && A2_val == "0") fo_term->setSimValue("0", sim_wf);
        else fo_term->setSimValue("X", sim_wf);
      }
    } 
    void Gate::computeORVal  ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "1")
        {
            fo_term->setSimValue("1", sim_wf);
        }
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            if (fi_1_val == "1") fo_term->setSimValue("1", sim_wf);
            else fo_term->setSimValue("X", sim_wf);
        }
    } 
    void Gate::computeXNORVal( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            fo_term->setSimValue("X", sim_wf);
        }
    } 
    void Gate::computeXORVal ( priority_queue<GNode*, vector<GNode*>, sim_wf_compare>& sim_wf)
    {
        assert(fanoutTerms.size() == 1);
        assert(faninTerms.size() == 2);
        Terminal* fo_term = fanoutTerms[0]; 
        string fi_0_val = faninTerms[0]->getSimValue();
        string fi_1_val = faninTerms[1]->getSimValue();
        if (fi_0_val == "0")
        {
            if (fi_1_val == "0") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "1")
        {
            if (fi_1_val == "0") fo_term->setSimValue("1", sim_wf);
            else if (fi_1_val == "1") fo_term->setSimValue("0", sim_wf);
            else if (fi_1_val == "X") fo_term->setSimValue("X", sim_wf);
        }
        if (fi_0_val == "X")
        {
            fo_term->setSimValue("X", sim_wf);
        }
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
            else {id = net->getPad(0)->getTopoId(); }
            topo_ids.push_back(id);
        }
    }

}
