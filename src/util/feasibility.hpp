#pragma once

#include "solution.hpp"
#include "heap.hpp"

void dijkstra(Solution& sol){
    constexpr int INF = numeric_limits<int>::max();
    Instance& I = sol.I;
    ShortestPathTree reference_fire_path = ShortestPathTree(I.ign, I.n);
    MyHeap Q(I.n);
    fill(reference_fire_path.a.begin(), reference_fire_path.a.end(), INF);
    Q.insertElement(I.ign, 0);
    reference_fire_path.a[I.ign] = 0;
    reference_fire_path.p[I.ign] = I.ign;
    unsigned reference_objective = 0;
    while (!Q.empty()) {
        unsigned u = Q.findAndDeleteMinElement();
        unsigned du = reference_fire_path.a[u];
        if (I.H > du)
          reference_objective++;
        for (Edge *e : I.get_outgoing_edges(u)) {
            unsigned v = e->destination;
            unsigned w = sol.has_resource[u]? e->weight + I.Delta : e->weight;
            if (reference_fire_path.a[v] > du + w && u != v) {
                Q.adjustHeap(v, du + w);
                reference_fire_path.a[v] = du + w;
                reference_fire_path.p[v] = u;
            }
        }
    }
    if(reference_objective != sol.objv){
        cout << "Objective values don't match." << endl;
        cout << "       Stored: " << sol.objv << "   Correct: " << reference_objective << endl;
        exit(1);
    }
    for(unsigned u = 0; u < I.n; u++){
        if(reference_fire_path.a[u] != sol.fire_path.a[u]){
            cout << "Fire arrival time at node " << u << " is wrong." << endl;
            cout << "       Stored: " << sol.fire_path.a[u] << "   Correct: " << reference_fire_path.a[u] << endl;
            exit(1);
        }
    }
}

void check_feasibility(Solution& sol) {
    Instance& I = sol.I;
    ShortestPathTree& fire_path = sol.fire_path;
    vector<pair<unsigned, unsigned>>& allocation = sol.allocation;
    string& has_resource = sol.has_resource;
    unsigned objv = sol.objv;
    unsigned tts = sol.time_to_survival;

    string resource(I.n, false);
    map<unsigned, unsigned> bookkeeping;
    for(auto const& [instant, quantity] : I.R)
        bookkeeping[instant] = quantity;


    /*
        Check if:
            i) Each node received at most one resource
            ii) At most |R_t| resources were deployed at time instant t
            iii) A burned node didn't receive a resource
    */
    for (auto const& [node, instant] : allocation) {
        if (resource[node]) {
            cout << "Node " << I.get_node_signature(node) << " received a resource twice." << endl;
            exit(1);
        }else
            resource[node] = true;
        if(bookkeeping[instant] == 0){
            cout << "More resources were deployed at instant " << instant << " than the available quantity." << endl;
            exit(1);
        }else
            bookkeeping[instant]--;
        if (fire_path.a[node] < instant) {
            cout << "Node " << I.get_node_signature(node) << " received a resource after fire arrival." << endl;
            exit(1);
        }
    }

    // Check if 'has_resource' and 'allocation' are consistent
    if(resource != has_resource){
        cout << "'has_resource' and 'allocation' are inconsistent." << endl;
        exit(1);
    }

    // Check if the shortest-path tree is correct
    unsigned burned_nodes = 0;
    unsigned time_to_survival = 0;
    for (unsigned node = 0; node < I.n; node++) {
        unsigned pred_n = fire_path.p[node]; 
        string pred_sig = I.get_node_signature(pred_n);
        string n_sig = I.get_node_signature(node);
        if (node != I.ign) {
            if(node == pred_n){
                cout << "Node " << n_sig << " does not have a predecessor."<< endl;
                exit(1);
            }
            unsigned delta = resource[pred_n] ? I.Delta : 0;
            if (fire_path.a[node] != fire_path.a[pred_n] + delta + I.get_edge_cost(pred_n, node)) {
                cout << "Inconsistent fire arrival times between nodes " << pred_sig << " and " << n_sig << endl;
                exit(1);
            }
        }else{
            if (fire_path.a[node] != 0) {
                cout << "Fire arrival time at ignition node is not zero."<< endl;
                exit(1);
            }
            if (node != pred_n) {
                cout << "Ignition node is not the predecessor of itself."<< endl;
                exit(1);
            }
        }
        if(fire_path.a[node] < I.H)
            burned_nodes++;
        time_to_survival += max(int(I.H) - int(fire_path.a[node]), 0);
    }

    if(burned_nodes != objv){
        cout << "Objective value is incorrect! The current value is " << objv << " but the correct value is " << burned_nodes << endl;
        exit(1);
    }
    if(time_to_survival != tts){
        cout << "Time to survival is incorrect! The current value is " << tts << " but the correct value is " << time_to_survival << endl;
        exit(1);
    }
    
    // Check if fire arrival times are correct
    dijkstra(sol);
}