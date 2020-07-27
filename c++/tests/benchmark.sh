#!/bin/bash

printf "benchmark.sh on $(date)\n\n\n" > bandwidth_perf.log

run() {
  echo "$(date) build/test $1 $2 $3 $4"
  { /usr/bin/time -f "%U user %S system %E(%e s) elapsed  %P CPU\n(%Xtext+%Ddata %Mmax)k\n%I inputs + %O outputs (%F major + %R minor)pagefaults %W swaps" build/test $1 $2 $3 $4 ; } 2>> bandwith_perf.log | \
  head -n 110 | tee -a bandwidth_perf.log
  printf "\n\n" >> bandwidth_perf.log
}

run 1 100 4 100
run 2 100 4 100
run 3 100 4 100

run 3 100 1 100
run 3 100 2 100
run 3 100 4 100
run 3 100 8 100
run 3 100 16 100
run 3 100 32 100

run 3 10000 4 1
run 3 1000 4 10
run 3 100 4 100
run 3 10 4 1000
run 3 1 1 10000

