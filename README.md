# Iterated-Beam-Search-for-Wildfire-Suppression

Accompanying repository of the paper "Iterated Beam Search for Wildfire Suppression". 


### Building & Compiling & Running
The codebase can be compiled by running the script  ```configure.sh```. You will need to have [Boost](https://www.boost.org/) installed.

```bash
./configure.sh
```

You can run the algorithm on the instance ```LA0``` by typing
```bash
./build/fire --instance ./instances/L0_a.json --beta 50 --eta 70 --c 30 --p 0.5 --zmax 3 --timelimit 60 --seed 1
```

### Running the experiments
Type the following commands to reproduce our results:
```bash
cd experiments
./exec.sh
```
The codebase will be compiled and our algorithm will run on all 16 instances in ```./instances```. You will need to have [GNU Parallel](https://www.gnu.org/software/parallel/) installed.


### Instances

In the directory ```./instances``` you will find the 16 instances used in the paper. These instances are originally from
[this repository](https://github.com/mitchopt/Logic-based-Benders-decomposition-for-wildfire-suppression). 


## How to cite

TBD.


