#!/bin/bash

printf "benchmark.sh on $(date)\n\n\n" > bandwidth_perf.log

exec &> >(tee  -a bandwidth_perf.log)
python3 tests/bandwidth.py