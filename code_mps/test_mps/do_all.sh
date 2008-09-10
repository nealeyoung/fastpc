#!/bin/bash

if [ $# -eq 0 ]
then
  echo "Usage: $0 fileprefix"
  exit 1
fi
echo -n 'Make random tests (y or n)?'
read $random
echo -n 'Make tomog tests (y or n)?'
read $tomog
if [ "$random" = "y" ]
  echo 'Creating random tests...'
  python make_tests.py $1
  echo 'Done creating random tests'
fi
if [ "$tomog" = "y" ]
  echo 'Creating tomog tests...'
  python tomog/scan_all.py $1 test_cases test_cases_glpk 0.01 2
  echo 'Done making tomog tests'
fi
python run_tests.py $1
echo 'Done running tests'
python parse_output.py $1
echo 'Output parsed'

