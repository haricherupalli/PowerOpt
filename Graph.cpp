#include <iostream>
#include "GNode.h"
#include "Graph.h"

using namespace std;

namespace POWEROPT {
  
  void Graph::print()
  {
    for (int i = 0; i < nodes.size(); i++)
    {
      GNode * node = nodes[i];
      if (node->getIsPad())
        cout << i << " " <<  node->getPad()->getName() << " : " << node->getVisited() << " : " <<  node->getTopoId()  << endl;
      else
        cout << i << " " << node->getGate()->getName() << " : " << node->getVisited() << " : " <<  node->getTopoId()  << endl;
    }

  }

  void Graph::print_sources()
  {
    cout << "SOURCES" << endl;
    for (int i = 0; i < sources.size(); i++)
    {
      GNode * node = sources[i];
      if (node->getIsPad())
        cout << "Pad:" << node->getPad()->getName() << endl;
      else
        cout << "Gate:" << node->getGate()->getName() << endl;
    }
    cout << "DONE SOURCES" << endl;

  }
  void Graph::print_wf()
  {
    cout << "WAVEFRONT" << endl;
    while (getWfSize() != 0)
    {
       GNode* node = getWfNode();
      if (node->getIsPad())
        cout << "Pad:" << node->getPad()->getName() << endl;
      else
        cout << "Gate:"  << node->getGate()->getName() << endl;
      popWfNode(); 
    }
    cout << "DONE WAVEFRONT" << endl;

  }
  
  void Graph::clear_visited()
  {
     for(int i = 0; i < nodes.size(); i++)
     {
        GNode* node = nodes[i];
        node->setVisited(false);
     }
  }
}
