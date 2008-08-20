#!/bin/bash

if [ $# -eq 0 ]
then
  echo "Usage: $0 fileprefix"
  exit 1
fi
python make_tests.py $1
echo 'Done creating tests'
python run_tests.py $1
echo 'Done running tests'
python parse_output.py $1

