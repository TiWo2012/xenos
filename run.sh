#!/bin/sh
set -e
cmake -B build -G Ninja > /dev/null 2>&1
cmake --build build --target run
