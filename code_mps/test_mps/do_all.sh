#!/bin/bash

python make_tests.py $1
python run_tests.py $1
python parse_output.py $1

