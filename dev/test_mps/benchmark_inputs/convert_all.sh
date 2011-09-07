#!/bin/bash

for f in *.tar.gz
	do
		echo Generating fastpc inputs for $f;
		./convert_set.py $f
	done;

