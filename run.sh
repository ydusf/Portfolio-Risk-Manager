#!/bin/bash
set -e

source venv/bin/activate
pip3 install -r ./requirements.txt

python3 scripts/assets.py "$@"

rm -rf build
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

./PortfolioRiskManager "$@"