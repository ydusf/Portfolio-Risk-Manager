#!/bin/bash

set -e

# Python setup
source venv/bin/activate
pip3 install -r ./scripts/requirements.txt

python3 scripts/assets.py

# C++ setup
mkdir -p build
cd build

cmake ..
make -j

ctest --output-on-failure
