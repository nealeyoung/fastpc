#!/bin/bash

echo 'Running fastpc'
python run_neal_fastpc.py frb;

echo 'Running cplex'
python run_cplex_algs.py frb -a;

