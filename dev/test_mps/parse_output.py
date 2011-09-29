#!/usr/bin/env python

import sys
import re
import os

def parse_fastpc_output_file(file_name, version):
    v07 = False
    if len(version) > 0:
        if version == "v07":
            v07 = True
        version = '_' + version
    else:
        version = '_fastpc'

    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    if not v07:
        sort_reg = re.compile(r'sort ratio = [0-9]*.[0-9]*')
    time_reg = re.compile(r' time = [0-9]*.*[0-9]*')
    ans_reg =  re.compile(r'.*iterations = [0-9]* primal = [0-9]*.[0-9]* dual = [0-9]*.[0-9]* ratio = [0-9]*.[0-9]*')
    eps_reg = re.compile(r'epsilon = 0.[0-9]*')
    input_reg = re.compile(r"ROWS: [0-9]* COLUMNS: [0-9]* NON-ZEROS: [0-9]* DENSITY: [0-9]*.*[0-9]*")
    name_reg = re.compile(r"INPUT FILE: .[/a-zA-Z0-9._]*")
    preprocess_reg = re.compile(r'preprocessing_time = [0-9]*.[0-9]*')
    main_loop_reg = re.compile(r' main_loop_time = [0-9]*.[0-9]*')
    ops_reg = re.compile(r'basic_ops = [0-9]*')

#    ops_array = ops_reg.findall(total_string)
    time_array = time_reg.findall(total_string)
    ans_array = ans_reg.findall(total_string)
    input_array = input_reg.findall(total_string)
    name_array = name_reg.findall(total_string)
    preprocess_array = preprocess_reg.findall(total_string)
    main_loop_array = main_loop_reg.findall(total_string)
    eps_array = eps_reg.findall(total_string)

    parse_main_loop_time = False
    if len(main_loop_array) == len(time_array):
        parse_main_loop_time = True

    if not v07:
        s_array = sort_reg.findall(total_string)

    for index, item in enumerate(time_array):
#    print "Filename,Rows,Columns,Nonzeros,Density,Time(s),Iterations,Ratio,Epsilon,TotalTime,TotalIter,HybridIter,HybridTime,"
        print str(name_array[index][name_array[index].rfind('/')+1:]) + version + ',',
        
        input_list = input_array[index].split()
        rows = input_list[input_list.index("ROWS:")+1]
        cols =  input_list[input_list.index("COLUMNS:")+1]
        non_zeros = input_list[input_list.index("NON-ZEROS:")+1]
        density = float(non_zeros)/(float(rows)*float(cols))
        print str(rows)+',', str(cols)+',', str(non_zeros)+',', str(density)+',',
        time_list = time_array[index].split()
        time = str(time_list[time_list.index("=")+1][:-1])
        print time + ',',

        ans_list = ans_array[index].split()
        iterations = ans_list[ans_list.index("iterations")+2]
        print iterations + ',',
        ratio = ans_list[ans_list.index("ratio")+2]
        print str(ratio)+',',

        eps_list = eps_array[index].split()
        print eps_list[eps_list.index("=")+1]+',',

        print '-1,-1,-1,',

#        if not v07:
#            s_list = s_array[index].split()
#            print str(s_list[s_list.index("=")+1]) + ','#,
#        else:
#            print ','#,
        print '-1,'
        
#        if parse_main_loop_time:
#            main_loop_list = main_loop_array[index].split()
#            print str(main_loop_list[main_loop_list.index("=")+1][:-1])+',',
#        else:
#            print ',',

#       preprocess_list = preprocess_array[index].split()
#       print str(preprocess_list[preprocess_list.index("=")+1][:-1])+','

def parse_glpk_output_file(file_name):
    try:
        my_file = open(file_name)
        total_string = my_file.read()
    except:
        print file_name + ', NOT FOUND'
        return

    time_reg = re.compile(r'[0-9]*.*[0-9]* secs')
    ans_reg = re.compile(r'obj =[ ]*[0-9]*.[0-9]*e[+-][0-9]*')
    input_reg = re.compile(r'[.]*: [0-9]* rows, [0-9]* columns, [0-9]* non-zeros') 
    name_reg = re.compile(r"[.]* reading problem data from `[/0-9a-zA-Z._]*'")

    time_array = time_reg.findall(total_string)
    ans_array = ans_reg.findall(total_string)
    input_array = input_reg.findall(total_string)
    name_array = name_reg.findall(total_string)

    for index, item in enumerate(time_array):
#    print "Filename,Rows,Columns,Nonzeros,Density,Time(s),Iterations,Ratio,Epsilon,TotalTime,TotalIter,HybridIter,HybridTime,"
        print str(name_array[index][name_array[index].rfind('/')+1:-1])+',',
        input_list = input_array[index].split()
        rows = input_list[input_list.index("rows,")-1]
        cols =  input_list[input_list.index("columns,")-1]
        non_zeros = input_list[input_list.index("non-zeros")-1]
        density = float(non_zeros)/(float(rows)*float(cols))
        print str(rows)+',', str(cols)+',', str(non_zeros)+',', str(density)+',',
        time_list = time_array[index].split()
        print time_list[time_list.index("secs")-1]

def parse_cplex_output_file(file_prefix, output_file_name):
    os.system('python parse_iterations_new.py ' + file_prefix)

#    Look for the cplex run stats file and append it to the general run_stats file
    try:
        cplex_file_name = './output_cplex/' + file_prefix + '_cplex_run_stats.csv'
        try:
            output_file = open(output_file_name, 'a')
            cplex_stats_file = open(cplex_file_name)
            cplex_stats_file.readline()
            for line in cplex_stats_file:
                output_file.write(line)
        except:
            output_file.write(output_file_name + ', NOT FOUND')
    except:
        print 'Error finding run stats file to append cplex data'
    
def main():
    args = sys.argv
    try:
        file_prefix = args[1]
    except:
        print 'Usage: python parse_output.py <file_prefix>'
        sys.exit(1)

    fp_file_name = './output/' + file_prefix + '_output'
    glpk_file_name = fp_file_name + '_glpk'
    v07_file_name = fp_file_name + '_v07'
    neal_file_name = fp_file_name + '_neal'

    output_file_name = './output/' + file_prefix + '_run_stats.csv'
    sys.stdout = open(output_file_name, 'w')
    print "Filename,Rows,Columns,Nonzeros,Density,Time(s),Iterations,Ratio,Epsilon,TotalTime,TotalIter,HybridIter,HybridTime,"

    parse_fastpc_output_file(fp_file_name, "")
    parse_fastpc_output_file(neal_file_name, "neal")
    parse_glpk_output_file(glpk_file_name)
    parse_fastpc_output_file(v07_file_name, "v07")
    
    sys.stdout.close()
    sys.stdout = sys.__stdout__
    
    parse_cplex_output_file(file_prefix, output_file_name)
    
main()
