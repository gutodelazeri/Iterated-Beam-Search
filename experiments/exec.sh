#!/bin/bash

set -e

bs() {
    local EXE=$1 
    local instances=$2
    local cores=$3
    local reps=$4
    local timelimit=$5
    local seeds=$(seq 1 ${reps})
    parallel -j ${cores} ${EXE} --instance ${INSTANCES}/L{1}_{2}.json --timelimit ${timelimit} --seed {3} ::: $(seq 0 7) ::: a b ::: ${seeds} > results.csv
}

ROOT=$(git rev-parse --show-toplevel)
BASE=$(pwd)
EXE="${ROOT}/build/fire"
INSTANCES="${ROOT}/instances"
CORES=2
REPS=20
TIMELIMIT=600

${ROOT}/configure.sh
bs ${EXE} ${INSTANCES} ${CORES} ${REPS} ${TIMELIMIT} 
