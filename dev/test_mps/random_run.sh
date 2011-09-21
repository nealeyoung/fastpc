#!/bin/bash

key = '2011_sep20'

python make_random/make_tests.py key;

python run_neal_fastpc.py key;

python run_cplex_algs.py key;
