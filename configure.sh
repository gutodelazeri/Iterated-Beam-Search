#!/bin/bash

set -e

ROOT=$(git rev-parse --show-toplevel)

cmake -S ${ROOT} -B ${ROOT}/build -DCMAKE_BUILD_TYPE=Release
cmake --build ${ROOT}/build