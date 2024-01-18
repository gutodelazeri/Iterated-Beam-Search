#pragma once

#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <vector>
using namespace std;

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "instance.hpp"
#include "shortest_path_tree.hpp"
#include "heap.hpp"

class Solution {
   private:
    void read_solution(const string& solution_file) {
        ifstream f(solution_file);
        if (!f.good()) {
            cerr << "Could not open file: '" <<  solution_file << "'"<< endl;
            exit(1);
        }
        json solution = json::parse(f);
        f.close();

        objv = solution["objv"].get<unsigned>();
        for (auto entry : solution["resourceAllocation"]) {
            unsigned i = entry.at(0).at(0).get<unsigned>();
            unsigned j = entry.at(0).at(1).get<unsigned>();
            unsigned instant = entry.at(1).get<unsigned>();
            unsigned n_id = I.G.get_node_id(MAKE_COORD(i, j));
            allocation.emplace_back(n_id, instant);
            has_resource[n_id] = true;
            
        }
        for (auto entry : solution["fireArrivalTime"]) {
            unsigned i = entry.at(0).at(0).get<unsigned>();
            unsigned j = entry.at(0).at(1).get<unsigned>();
            unsigned instant = entry.at(1).get<unsigned>();
            unsigned n_id = I.G.get_node_id(MAKE_COORD(i, j));
            fire_path.a[n_id] = instant;
        }
    }

   public:
    Solution(const string& solution_file, Instance &_I) : I(_I), fire_path(_I.ign, _I.n) {
        timestamp = 0;
        iter = 0;
        has_resource = string(I.n, false);
        if(solution_file != "")
            read_solution(solution_file);
    }
    
    Solution(Instance &_I) : I(_I), fire_path(_I.ign, I.n) {
        timestamp = 0;
        iter = 0;
        has_resource = string(I.n, false);
        objv = numeric_limits<unsigned>::max();
    }

    Instance& I;
    ShortestPathTree fire_path;
    vector<pair<unsigned, unsigned>> allocation;
    string has_resource;

    // Statistics
    unsigned objv;
    unsigned timestamp;
    unsigned iter;
    unsigned time_to_survival;
  
    Solution& operator=(const Solution& other) {   
        I = other.I;
        fire_path = other.fire_path;
        allocation = other.allocation;
        has_resource = other.has_resource;
        objv = other.objv;
        timestamp = other.timestamp;
        iter = other.iter;
        time_to_survival = other.time_to_survival;
        return *this;
    }

    void write_solution(ofstream& fout) {
        json data;
        data["objv"] = objv;
        data["timestamp"] = timestamp;
        data["pred"] = json::array();
        data["fireArrivalTime"] = json::array();
        data["resourceAllocation"] = json::array();
        for (unsigned n = 0; n < I.n; n++){
            unsigned pred_n = fire_path.p[n];
            COORD coord_n = I.G.get_node_coord(n);
            COORD coord_pred_n = I.G.get_node_coord(pred_n);
            data["pred"].push_back({{coord_n.first, coord_n.second}, {coord_pred_n.first, coord_pred_n.second}});
            data["fireArrivalTime"].push_back({{coord_n.first, coord_n.second}, fire_path.a[n]});
        }
        for (auto const& [node, instant] : allocation){
            COORD n_coord = I.G.get_node_coord(node);
            data["resourceAllocation"].push_back({{n_coord.first, n_coord.second}, instant});
        }
        string output_str = data.dump(4);
        fout << output_str << std::endl;
    }
};
