#!/bin/bash

set -e

source venv/bin/activate
pip3 install -r ./requirements.txt

python3 scripts/assets.py "$@"

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-g -O0" \
      -DCMAKE_C_FLAGS="-g -O0" ..
make -j

lldb -- ./PortfolioRiskManager "$@"
