#pragma once

#include <fstream>
#include <limits>
#include <map>
#include <regex>
#include <vector>
#include <algorithm>
using namespace std;

#define MAKE_COORD(i, j) make_pair(i, j)
typedef pair<unsigned, unsigned> COORD;
typedef unsigned NODE_ID;

struct Edge {
  unsigned id, source, destination, weight;
  Edge(unsigned _id, NODE_ID _s, NODE_ID _d, unsigned _w)
      : id(_id), source(_s), destination(_d), weight(_w) {}
};

struct Node {
  unsigned id;
  vector<Edge*> incoming;
  vector<Edge*> outgoing;
  Node() : id(){};
  Node(NODE_ID nid)
      : id(nid), incoming(vector<Edge*>()), outgoing(vector<Edge*>()) {}
  void add_incoming_edge(Edge* e) { incoming.push_back(e); }
  void add_outgoing_edge(Edge* e) { outgoing.push_back(e); }
};

class Digraph {

private:
  unsigned edge_id_counter = 0;
  NODE_ID node_id_counter = 0;
  vector<COORD> id_to_coord;
  map<COORD, NODE_ID> coord_to_id;
  vector<Node> nodes;
  vector<Edge*> edges;


public:
  Digraph() = default;

  ~Digraph() {
    for (Edge* e : edges) delete e;
  }
  
  // TODO: check for repeated nodes
  void add_node(COORD coord) {
    id_to_coord.push_back(coord);
    coord_to_id[coord] = node_id_counter;
    nodes.push_back(Node(node_id_counter));
    node_id_counter++;
  }

  void add_edge(COORD source, COORD destination, unsigned weight) {
    NODE_ID s = coord_to_id[source];
    NODE_ID d = coord_to_id[destination];
    Edge* e = new Edge(edge_id_counter++, s, d, weight);
    edges.push_back(e);
    nodes[s].add_outgoing_edge(e);
    nodes[d].add_incoming_edge(e);
  }

  const vector<Edge*> &get_outgoing_edges(NODE_ID node_id) {
    return nodes[node_id].outgoing;
  }

  const vector<Edge*> &get_incoming_edges(NODE_ID node_id) {
    return nodes[node_id].incoming;
  }

  unsigned get_edge_cost(NODE_ID pred, NODE_ID succ){
    for(const Edge* e : nodes[pred].outgoing)
      if(e->destination == succ) return e->weight;
    cerr << "Node " << get_node_signature(pred) << " is not a neighbor of node " << get_node_signature(succ) << endl;
    exit(1);
  }

  unsigned get_number_of_nodes() { return nodes.size(); }

  unsigned get_number_of_edges() { return edges.size(); }

  COORD get_node_coord(NODE_ID nid) { return id_to_coord[nid];}

  NODE_ID get_node_id(COORD ncoord) { return coord_to_id[ncoord];}

  const vector<COORD>& get_list_of_coords() {
    return id_to_coord;
  }

  bool is_valid(const COORD& ncoord) {
    return coord_to_id.contains(ncoord);
  }

  string get_node_signature(NODE_ID id) {
    COORD c = id_to_coord[id];
    return "(" + to_string(c.first) + "," + to_string(c.second) + ")";
  }
};

