#include <fstream>
#include <iostream>
#include <vector>
#include <limits>
#include <map>
using namespace std;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <chrono>
using namespace std::chrono;

#include "algorithm.hpp"
#include "feasibility.hpp"


int main(int argc, char *argv[]) {
    // 1) Parse input
    struct Options {
        string instance;
        unsigned timelimit, seed, max_iterations;
        unsigned target;
        unsigned beta, eta, zmax, c;
        unsigned long long int budget;
        double p, phat;
        bool verbose, save;
    };
    Options opt;
    po::options_description general("General options");
    general.add_options()("instance", po::value<string>(&opt.instance), "Path to instance specification file.")
                         ("verbose", po::bool_switch(&opt.verbose)->default_value(false), "Verbosity.")
                         ("target", po::value<unsigned>(&opt.target)->default_value(0), "Target objective value.")
                         ("maxiter", po::value<unsigned>(&opt.max_iterations)->default_value(numeric_limits<unsigned>::max()), "Maximum number of iterations.")
                         ("budget", po::value<unsigned long long int>(&opt.budget)->default_value(numeric_limits<unsigned long long int>::max()), "Maximum number of subtree updates.")
                         ("timelimit", po::value<unsigned>(&opt.timelimit)->default_value(7200), "Maximum running time (in seconds).")
                         ("save", po::bool_switch(&opt.save)->default_value(false), "Save best-found solution.")
                         ("seed", po::value<unsigned>(&opt.seed)->default_value(123), "Seed value.");
    po::options_description beams("Beam Search");
    beams.add_options()("p", po::value<double>(&opt.p)->default_value(0.5), "Probability of picking an element of N.")
                       ("beta", po::value<unsigned>(&opt.beta)->default_value(50), "Number of starting nodes at each level.")
                       ("eta", po::value<unsigned>(&opt.eta)->default_value(70), "Number of expansions.")
                       ("c", po::value<unsigned>(&opt.c)->default_value(30), "Multiplier for the number of iterations performed by Step.")
                       ("phat", po::value<double>(&opt.phat)->default_value(0.4), "Transition instant as a percentage of the free burning time.")
                       ("zmax", po::value<unsigned>(&opt.zmax)->default_value(3), "Maximum value for z.");
    general.add(beams);
    po::positional_options_description pod;
    pod.add("instance", 1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(general).positional(pod).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << general << endl;
        return 0;
    }
    if (!vm.count("instance")) {
        cerr << "Please provide an input instance." << endl;
        cout << general << endl;
        return 1;
    }
    string base_filename = opt.instance.substr(opt.instance.find_last_of("/") + 1);
    string::size_type const p(base_filename.find_last_of('.'));
    string instance_without_extension = base_filename.substr(0, p);  
    
    // 2) Load instance
    Instance I;
    I.read_instance(opt.instance);

    // 3) Create algorithm
    Algorithm alg (I, opt.seed, opt.p, opt.phat, opt.beta, opt.eta, opt.zmax, opt.c);
    
    // 4) Run algorithm
    Solution B(I);
    Solution current(I);
    unsigned iter = 0;
    unsigned elapsed_time = 0;
    bool global_optimum = false;
    vector<tuple<unsigned, unsigned, unsigned, unsigned>> trajectory;
    steady_clock::time_point begin = steady_clock::now();
    while (!global_optimum && elapsed_time < opt.timelimit && iter < opt.max_iterations && alg.get_budget() < opt.budget) {
        current = alg.beam_search(B.objv);
        iter++;
        elapsed_time = duration_cast<seconds>(steady_clock::now() - begin).count();
        if (current.objv < B.objv) {
            if (opt.verbose)
                cout << current.objv << " " << elapsed_time << " " << iter << " " << alg.get_budget() << endl;
            B = current;
            B.timestamp = elapsed_time;
            B.iter = iter;
            if(opt.target >= B.objv)
                global_optimum = true;
            trajectory.emplace_back(B.objv, elapsed_time, iter, alg.get_budget());
        }
    }

    // 5) Save best solution
    if (opt.save) {
        ofstream outfile("Sol_" + instance_without_extension + ".json");
        B.write_solution(outfile);
    }

    // 6) Check solution feasibility
    check_feasibility(B);

    // 7) Print results
    string traj = "\"[";
    for(const auto& [obj, et, i, b] : trajectory)
        traj = traj + "(" +  to_string(obj) + "," + to_string(et) + "," + to_string(i) + "," + to_string(b) + "), ";
    traj.pop_back();traj.pop_back();
    traj = traj + "]\"";
    cout << instance_without_extension << ","
         << opt.seed << ","
         << opt.p << ","
         << opt.phat << ","
         << opt.beta << ","
         << opt.eta << ","
         << opt.zmax << ","
         << opt.c << ","
         << B.objv << ","
         << B.timestamp << ","
         << B.iter << ","
         << iter << ","
         << elapsed_time << "," 
         << alg.get_budget() << ","
         << traj << endl;
}