#!/bin/bash

key="2011_sep20"
echo $key

echo "Generating random inputs"
python make_random/make_tests.py $key;

echo "Running fastpc"
python run_neal_fastpc.py $key;

echo "Running cplex algorithms"
python run_cplex_algs.py $key;
