#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>

#include "Gate.h"
#include "Terminal.h"
#include "Path.h"
#include "Pad.h"

using namespace std;

namespace POWEROPT {
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

}
