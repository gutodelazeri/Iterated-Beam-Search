#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <algorithm>
#include <iterator>
#include <set>
#include <cmath>
#include <string>
using namespace std;

#include <boost/heap/binomial_heap.hpp>
using namespace boost::heap;

#include "instance.hpp"
#include "solution.hpp"
#include "shortest_path_tree.hpp"
#include "heap.hpp"
#include "feasibility.hpp"

constexpr int INF = numeric_limits<int>::max();

class Algorithm {
   private:
    // Budget counter
    unsigned long long int _budget;

    // Parameters
    mt19937 _gen;
    Instance &_I;
    unsigned _seed;
    double _p;
    double _phat;
    double _that;
    unsigned _beta;
    unsigned _eta;
    unsigned _z;
    unsigned _zmax;
    unsigned _c;

    // State
    Solution _A0;
    unsigned _free_burning_time;
    MyHeap _Heap;

    /* ----------------- BEAM SEARCH ---------------------------- */
    void step(Solution& sol, unsigned instant, unsigned quantity, vector<Solution>& E){
        bool last_round = (_I.alpha(instant) == _I.H);
        set<unsigned> F;
        set<unsigned> N;
        set<unsigned> N_copy;
        vector<unsigned> to_remove_N;
        vector<unsigned> to_insert_N;
        vector<unsigned> to_insert_F;
        vector<unsigned> selection;
        set<string> cache;
        vector<vector<unsigned>> candidates;
        vector<pair<unsigned, unsigned>> scores;
        uniform_real_distribution<double> dist01(0, 1);

        // Compute f(t, z)
        double fp_cutoff = instant;
        unsigned lb = instant, ub = instant;
        unsigned zp = ceil((_z+1)/2.0);
        while(zp >= 1) { 
            lb = _I.alpha(lb);
            zp--;
        }
        zp = ceil((_z+2)/2.0);
        while(zp >= 1) { 
            ub = _I.alpha(ub);
            zp--;
        }
        fp_cutoff = 0.5 * lb + 0.5 * ub;

        for(const Solution& s : E)
            cache.insert(s.has_resource); 
        
        // Build F
        for(unsigned n = 0; n < _I.n; n++) 
            if(!sol.has_resource[n] && sol.fire_path.a[n] >= instant && sol.fire_path.a[n] <= fp_cutoff)
                F.insert(n);

        // Build N
        for(pair<unsigned, unsigned>& a :  sol.allocation){
            for(unsigned v : _I.get_extended_neighborhood(a.first))
                if(F.contains(v)) {
                    N_copy.insert(v);
                    N.insert(v);        
                }
        }
        
        for(unsigned trial = 0; trial < _c * F.size(); trial++) {
            unsigned u;
            // Expand
            for(unsigned res = 0; res < quantity; res++){
                if(!N.empty() && dist01(_gen) < _p){
                    uniform_int_distribution<> distC(0, N.size()-1); 
                    u = *next(N.begin(), distC(_gen));
                } else{
                    assert(!F.empty());
                    uniform_int_distribution<> distF(0, F.size()-1); 
                    u = *next(F.begin(), distF(_gen));
                }
                selection.push_back(u); 
                F.erase(u);
                to_insert_F.push_back(u);
                for(unsigned v : _I.get_extended_neighborhood(u)){
                    if(F.contains(v) && !N.contains(v)){
                        N.insert(v);
                        to_remove_N.push_back(v);
                    }
                }
                if(N.contains(u)){
                    N.erase(u);
                    if(N_copy.contains(u))
                        to_insert_N.push_back(u);
                } 
            }
            
            for(unsigned n : selection) 
                sol.has_resource[n] = true;
            
            if(!cache.contains(sol.has_resource)) {
                cache.insert(sol.has_resource);
                auto metrics = delta(sol, selection);
                unsigned h1 =  (sol.objv - metrics.first);
                unsigned h2 =  (sol.time_to_survival - metrics.second);
                if(candidates.empty() || !last_round){
                    candidates.emplace_back(selection);
                    if(instant >= _that || last_round)
                        scores.emplace_back(h1, candidates.size()-1);
                    else   
                        scores.emplace_back(h2, candidates.size()-1);
                }else{
                    if(scores[0].first > h1){
                        scores[0] = {h1, 0};
                        candidates[0] = selection;
                    }
                }
            }

            for(unsigned u : to_remove_N)
                N.erase(u);
            for(unsigned u : to_insert_N)
                N.insert(u);
            for(unsigned u : to_insert_F)
                F.insert(u);
            for(unsigned n : selection) 
                sol.has_resource[n] = false;
            
            to_remove_N.clear();
            to_insert_N.clear();
            to_insert_F.clear();
            selection.clear();
        }

        sort(scores.begin(), scores.end(), [](pair< unsigned, unsigned>& a, pair<unsigned,unsigned>& b){ return a.first < b.first;});
        for(unsigned n = 0; n < min(scores.size(), size_t(_eta)); n++) {
            Solution a_prime = sol;
            add_resource(a_prime, instant, candidates[scores[n].second]);
            E.push_back(a_prime);
        }
    }

    void prune(vector<Solution>& A, vector<Solution>& E, unsigned instant){ 
        if(instant >= _that)
            sort(E.begin(), E.end(), [](Solution& a, Solution& b){return a.objv < b.objv;});
        else
            sort(E.begin(), E.end(), [](Solution& a, Solution& b){return a.time_to_survival < b.time_to_survival;});
        A.clear();
        for(unsigned i = 0; i < min(size_t(_beta), E.size()); i++)
            A.emplace_back(E[i]);
    }
 
     /* ----------------- SPT OPERATIONS ------------------------- */
    void add_resource(Solution& sol, unsigned instant, vector<unsigned>& nodes){
        vector<unsigned> affected_nodes, old_a, old_p;
        for (auto n : nodes) {
            sol.has_resource[n] = true;
            sol.allocation.emplace_back(n, instant);
        }
        update_subtree(sol, nodes, affected_nodes, old_p, old_a);
        update_solution(sol, affected_nodes, nodes, old_a);
    }
    
    pair<unsigned, unsigned> delta(Solution& sol, vector<unsigned>& selection){
        unsigned objv_delta = 0;
        unsigned tts_delta = 0;
        vector<unsigned> affected_nodes, old_p, old_a;
        for(auto n : selection) 
            sol.has_resource[n] = true;
        update_subtree(sol, selection, affected_nodes, old_p, old_a);
        for(unsigned idx_u = 0;  idx_u < affected_nodes.size(); idx_u++){
            unsigned u = affected_nodes[idx_u];
            if (old_a[idx_u] < _I.H && sol.fire_path.a[u] >= _I.H)
                objv_delta++;
            if (old_a[idx_u] < _I.H)
                tts_delta += (min(sol.fire_path.a[u], _I.H) - old_a[idx_u]);
        }
        for(auto n : selection) 
            sol.has_resource[n] = false;
        undo_update(sol, affected_nodes, old_p, old_a);

        return {objv_delta, tts_delta};
    }

    void undo_update(Solution& sol, vector<unsigned>& affected_nodes, vector<unsigned>& old_p, vector<unsigned>& old_a){
        ShortestPathTree& spt = sol.fire_path;
        for(unsigned idx = 0; idx < affected_nodes.size(); idx++){
            unsigned node = affected_nodes[idx];
            spt.a[node] = old_a[idx];
            spt.p[node] = old_p[idx];
        }
    }

    void update_subtree(Solution& sol, const vector<unsigned>& sources, vector<unsigned>& affected_nodes, vector<unsigned>& old_p, vector<unsigned>& old_a){
        // Update budget counter
        _budget++;
        _Heap.clear();
        ShortestPathTree& spt = sol.fire_path;
        vector<unsigned> Q;
        affected_nodes.clear();
        for(unsigned  s : sources){
            for(Edge *e : _I.get_outgoing_edges(s)){
                unsigned u = e->destination;
                if(spt.p[e->destination] == s)
                    _Heap.insertElement(u, spt.a[u]);
            }
        }
        while(!_Heap.empty()){
            unsigned u = _Heap.findAndDeleteMinElement();
            unsigned pred_u = spt.p[u];
            for(Edge* e : _I.get_incoming_edges(u)){
                unsigned v = e->source;
                unsigned w = sol.has_resource[v] ? e->weight + _I.Delta : e->weight;
                if(spt.a[u] == spt.a[v] + w){
                    affected_nodes.push_back(u);
                    old_p.push_back(spt.p[u]);
                    old_a.push_back(spt.a[u]);
                    spt.p[u] = v;
                    break;
                }
            }
            if(pred_u == spt.p[u]){
                affected_nodes.push_back(u);
                old_p.push_back(spt.p[u]);
                old_a.push_back(spt.a[u]);
                Q.push_back(u);
                spt.a[u] = INF;
                for(Edge* e : _I.get_outgoing_edges(u)){
                    unsigned v = e->destination;
                    if(spt.p[v] == u && spt.a[v] != INF)
                        _Heap.adjustHeap(v, spt.a[v]);
                }
            }            
        }
        for(unsigned u : Q){
            for(Edge* e : _I.get_incoming_edges(u)){
                unsigned v = e->source;
                unsigned w = sol.has_resource[v] ? e->weight + _I.Delta : e->weight;
                if(spt.a[u] > spt.a[v] + w){
                    spt.a[u] = spt.a[v] + w;
                    spt.p[u] = v;
                }
            }
            if(spt.a[u] != INF)
                _Heap.adjustHeap(u, spt.a[u]);
        }
        while(!_Heap.empty()){
            unsigned u = _Heap.findAndDeleteMinElement();
            for(Edge* e : _I.get_outgoing_edges(u)){
                unsigned v = e->destination;
                unsigned w = sol.has_resource[u] ? e->weight + _I.Delta : e->weight;
                if(spt.a[v] > spt.a[u] + w){
                    spt.a[v] = spt.a[u] + w;
                    spt.p[v] = u;
                    _Heap.adjustHeap(v, spt.a[v]);
                }
            }
        }
    } 

    void update_solution(Solution& sol, vector<unsigned>& affected_nodes, vector<unsigned>& selected_candidates, vector<unsigned>& old_a) {
        for(unsigned idx_u = 0; idx_u < affected_nodes.size(); idx_u++){
            unsigned u = affected_nodes[idx_u];
            if(sol.fire_path.a[u] >= _I.H && old_a[idx_u] < _I.H)
                    sol.objv--;
            if (old_a[idx_u] < _I.H){
                sol.time_to_survival -= (_I.H - old_a[idx_u]);
                sol.time_to_survival += max(int(_I.H) - int(sol.fire_path.a[u]), 0);
            }
        } 
    }

    /* ----------------- INITIALIZATION ------------------------- */  
    void build_A0(Solution& sol){
        _Heap.clear();
        fill(sol.fire_path.a.begin(), sol.fire_path.a.end(), INF);
        _Heap.insertElement(_I.ign, 0);
        sol.fire_path.a[_I.ign] = 0;
        sol.fire_path.p[_I.ign] = _I.ign;
        sol.objv = 0;
        sol.time_to_survival = 0;
        _free_burning_time = 0;
        while (!_Heap.empty()) {
            unsigned u = _Heap.findAndDeleteMinElement();
            unsigned du = sol.fire_path.a[u];
            if (_I.H > du){
              sol.objv++;
              sol.time_to_survival += max(_I.H - du, 0u);
              _free_burning_time = max(_free_burning_time, du);
            }
            for (Edge *e : _I.get_outgoing_edges(u)) {
                unsigned v = e->destination;
                unsigned w = e->weight;
                if (sol.fire_path.a[v] > du + w && u != v) {
                    _Heap.adjustHeap(v, du + w);
                    sol.fire_path.a[v] = du + w;
                    sol.fire_path.p[v] = u;
                }
            }
        }
    }
    /* ---------------------------------------------------------- */  

public:
    Algorithm(Instance &__I, unsigned __seed,  double __p,  double __phat,  unsigned __beta, unsigned __eta, unsigned __zmax, unsigned __c) : _budget(0),
                                                                                                                          _gen(mt19937(__seed)), 
                                                                                                                          _I(__I), 
                                                                                                                          _seed(__seed),
                                                                                                                          _p(__p),
                                                                                                                          _phat(__phat),
                                                                                                                          _that(0),
                                                                                                                          _beta(__beta),
                                                                                                                          _eta(__eta),
                                                                                                                          _z(0),
                                                                                                                          _zmax(__zmax),
                                                                                                                          _c(__c),
                                                                                                                          _A0(__I),
                                                                                                                          _Heap(__I.n){
        build_A0(_A0);
        _that = _phat * _free_burning_time;
    }

    Solution beam_search(unsigned bkv) {
        vector<Solution> A = {_A0};
        for(const auto& [instant, quantity] : _I.R) {
            vector<Solution> E;
            for(Solution& s : A)
                step(s, instant, quantity, E);
            prune(A, E, instant);
        }
        if(A[0].objv >= bkv)
            _z = (_z+1) % _zmax;
        return A[0]; 
    }

    void reset_random_state(){ _gen = mt19937(_seed); }

    inline unsigned long long int get_budget(){ return _budget;}
};