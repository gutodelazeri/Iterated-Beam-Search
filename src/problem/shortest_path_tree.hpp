#pragma once

#include <vector>
using namespace std;

typedef struct ShortestPathTree {
    unsigned s;
    vector<unsigned> p;
    vector<unsigned> a;
    ShortestPathTree(unsigned _s, unsigned n): s(_s), p(vector<unsigned>(n)), a(vector<unsigned>(n)){}
    ShortestPathTree(unsigned _s): s(_s) {}
    ShortestPathTree() = default;
} ShortestPathTree;