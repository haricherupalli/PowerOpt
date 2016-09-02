#ifndef __SETTRIE_H__
#define __SETTRIE_H__

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <list>

using namespace std;

class Node
{
public:
  int element;
  map<int, Node*> children;

  bool find_child (int child) ;
  Node* get_next(int child) {
    return children.find(child)->second;
  }
  bool essr(vector<int>& set, int i, bool strict);
  void insert_elems (vector<int>& set, int i);
  void print_recurs();
  static int indent;
};

class SetTrie
{
  public :
  void insert ( vector<int>& set);
  bool essr (vector<int>& set, bool proper);
  void print ();
  void remove_subsets();
  int num_sets () { return collection.size(); }
  list < pair< double,  vector<int> > > & get_collection () { return collection; }
  SetTrie ()
  {
    top = new Node;
    top->element = -1;
  }

  private:
  Node * top;
  list < pair < double , vector<int> > > collection;
};

#endif
