#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build results
for target in demo benchmark; do
  g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread -I. "$target.cpp" -o "build/$target"
done
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread -I. test.cpp -o build/tests
./build/tests
python3 validate_trace.py results/test_trace.json
./build/demo results/demo_trace.json
python3 validate_trace.py results/demo_trace.json
./build/benchmark > results/benchmark.csv 2> results/benchmark_summary.json
python3 report.py
