#!/bin/bash
#deletes all test cases-- fastpc, glpk, v2007
#deletes everything from output folder

#prefixes=( May1_2009 apr17_2009 apr20_2009 jun18_2009 jun16_sparse TOMOG frb DIMACS 2011_Nov25 2011_sep20 )
prefixes=( 2011_Nov25 2011_sep20 DIMACS frb )
suffix=_run_stats.csv
length=${#prefixes[*]}

#create all_run_stats.tmp
echo "" > all_run_stats.tmp

#append contents of run_stat files to all_run_stats
for prefix in ${prefixes[*]}
do
    file=$prefix'/'$prefix$suffix
    echo $file
    cat $file >> all_run_stats.tmp
done

awk '{gsub(",", "\t"); print }' all_run_stats.tmp > all_run_stats

#delete temp file
rm -rf all_run_stats.tmp

algos=( neal fastpc Primal Dual Barrier )

#split data by algorithms
for alg in ${algos[*]}
do
	cat all_run_stats | grep $alg | grep -v 'NOT FOUND' > $alg
	for prefix in ${prefixes[*]}
	do
		cat $alg | grep $prefix > $prefix'_'$alg
	done

done

cat fastpc > fandn
cat neal >> fandn

exit 0
