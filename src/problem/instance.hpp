#pragma once

#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <cmath>
using namespace std;

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <boost/multi_array.hpp>
using namespace boost;

#include "digraph.hpp"

struct Instance {
  unsigned H;                  // Optimization horizon
  unsigned n;                  // Number of nodes
  unsigned ign;                // Ignition node
  unsigned Delta;              // Delay

  vector<pair<unsigned, unsigned>> R; // Number of resources that become available at instant k
  vector<unsigned> T; // Time instants at which resources are available
  Digraph G; // Graph

  vector<set<NODE_ID>> N; // Immediate neighborhood
  vector<set<NODE_ID>> Nstar; // Extended neighborhood

  void build_neighborhoods() {
      N = vector<set<NODE_ID>>(G.get_number_of_nodes());
      Nstar = vector<set<NODE_ID>>(G.get_number_of_nodes());
      for(const COORD& n_c : G.get_list_of_coords()) {
        unsigned nid = G.get_node_id(n_c);
        unsigned x = n_c.first, y = n_c.second;
        COORD n1 = {x == 0 ? 0 : x - 1, y};
        COORD n2 = {x+1, y};
        COORD n3 = {x, y == 0 ? 0 : y - 1};
        COORD n4 = {x, y + 1};
        for(COORD ni : {n1, n2, n3, n4})
          if(ni != n_c && G.is_valid(ni)) {
            N[nid].insert(G.get_node_id(ni));
            Nstar[nid].insert(G.get_node_id(ni));
          }
        COORD n5 = {x == 0 ? 0 : x - 1, y == 0 ? 0 : y - 1};
        COORD n6 = {x == 0 ? 0 : x - 1, y + 1};
        COORD n7 = {x + 1, y == 0 ? 0 : y - 1};
        COORD n8 = {x + 1, y + 1};
        for(COORD ni : {n5, n6, n7, n8})
          if(ni != n_c && G.is_valid(ni)) 
            Nstar[nid].insert(G.get_node_id(ni));   
    } 
  }

  void read_instance(const string &instance_file) {
    ifstream f(instance_file);
    if (!f.good()) {
      cerr << "Could not open instance specification file: " << instance_file << endl;
      exit(1);
    }
    json instance = json::parse(f);
    f.close();

    // Optimization Horizon
    H = instance["ArrivalTimeTarget"].get<unsigned>();
    R = vector<pair<unsigned, unsigned>>();

    // Resources
    for (const auto &res : instance["ResAtTime"].items()) {
      unsigned instant = stoul(res.key());
      unsigned quantity = res.value();
      R.emplace_back(instant, quantity);
    }
    sort(R.begin(), R.end(), [](pair<unsigned, unsigned> a, pair<unsigned, unsigned> b){return a.first < b.first;});
    for(const auto& entry : R)
      T.push_back(entry.first);

    // Delta
    Delta = instance["Delay"].get<unsigned>();

    // Nodes
    G = Digraph();
    for (const auto &n : instance["Nodes"]) {
      unsigned i = n.at(0).get<unsigned>();
      unsigned j = n.at(1).get<unsigned>();
      G.add_node(MAKE_COORD(i, j));
    }
    n = G.get_number_of_nodes();

    // Ignition node
    unsigned ign_x = instance["Ignitions"].at(0).at(0).get<unsigned>();
    unsigned ign_y = instance["Ignitions"].at(0).at(1).get<unsigned>();
    ign = G.get_node_id(MAKE_COORD(ign_x, ign_y));

    // Edges
    regex rgx("\\((-?\\d+), (-?\\d+)\\)");
    for (const auto &edge : instance["Arcs"].items()) {
      const string coords = edge.key();
      unsigned weight = edge.value();

      vector<int> values;
      sregex_iterator iterator(coords.begin(), coords.end(), rgx);
      sregex_iterator endIterator;
      while (iterator != endIterator) {
        smatch match = *iterator;
        int firstValue = stoi(match[1]);
        int secondValue = stoi(match[2]);
        values.push_back(firstValue);
        values.push_back(secondValue);
        ++iterator;
      }
      COORD c1 = MAKE_COORD(values[0], values[1]);
      COORD c2 = MAKE_COORD(values[2], values[3]);
      G.add_edge(c1, c2, weight);
    }

    build_neighborhoods();
  }

  const vector<Edge*> &get_outgoing_edges(NODE_ID node_id) {
    return G.get_outgoing_edges(node_id);
  }

  const vector<Edge*> &get_incoming_edges(NODE_ID node_id) {
    return G.get_incoming_edges(node_id);
  }

  unsigned get_number_of_edges() { return G.get_number_of_edges(); }
  
  unsigned get_edge_cost(NODE_ID pred, NODE_ID succ){
    return G.get_edge_cost(pred, succ);
  }

  unsigned alpha(unsigned t) {
    if(t >= T.back())
      return H;
    else
      return *upper_bound(T.begin(), T.end(), t);
  }

  const set<NODE_ID>& get_extended_neighborhood(NODE_ID nid){
    return Nstar[nid];
  }

  const set<NODE_ID>& get_neighborhood(NODE_ID nid){
    return N[nid];
  }

  string get_node_signature(NODE_ID id) {
      return G.get_node_signature(id);
  }
};
