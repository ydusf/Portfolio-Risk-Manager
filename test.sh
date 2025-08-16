#!/bin/bash
set -e

mkdir -p build
cd build

cmake ..
make -j

ctest --output-on-failure
