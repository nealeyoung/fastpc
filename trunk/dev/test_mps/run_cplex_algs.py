#!/usr/bin/env python

import sys
import os

output_dir = 'output_cplex/'
curr_dir = os.getcwd()
cplex_input_dir = curr_dir + '/test_cases_glpk/'

cplex_path = '/opt/ibm/ILOG/CPLEX_Studio_Preview123/cplex/bin/x86-64_sles10_4.1/cplex '
#cplex_path = '../../../../cplex/cplex/bin/x86-64_sles10_4.1/cplex '

def print_help():
    print "Usage: python run_cplex_algs.py [input_file_prefix] [-a] [-p] [-d] [-b]"
    print "Order of arguments is not relevant."
    print "input_file_prefix: specifies the prefix to look for in the file names. If none specified, runs cplex on all input files in the cplex input directory."
    print "-a: Run all cplex algorithms."
    print "-p: Run Primal method."
    print "-d: Run Dual method."
    print "-b: Run Barrier method."

def run_cplex_alg(cplex_file, output_file_cplex_location, method, method_desc):
    cplex_command_input = 'cplex_command_input'
    file = open(cplex_command_input, 'w')
    
    file.write('set preprocessing presolve n\n')
    file.write('set preprocessing repeatpresolve 0\n')
    file.write('set preprocessing reduce 0\n')
    file.write('set preprocessing aggregator 0\n')

    file.write('read ' + cplex_input_dir + cplex_file + ' \n')
    file.write('lp \n')
    file.write(method + ' \n')
    file.write('write ' + output_dir + cplex_file + '_cplex_' + method_desc + '_out.sol \n')
    file.close()

    print "    cplex is now running " + method_desc

    cplex_cmd = cplex_path + ' >> ' + output_file_cplex_location + ' < ' + cplex_command_input
    print cplex_cmd
    os.system(cplex_cmd)

def main():

    primal_run = False
    dual_run = False
    bar_run = False
    input_file_prefix = ''

    try:
        args = sys.argv        
        l = len(args)
        if l == 1:
            dual_run = True
        else:
            for i in range(1,l):
                arg = args[i]
                if arg == '--help' or arg == '-h':
                    print_help()
                    sys.exit()
                elif arg == '-a':
                    primal_run = True
                    dual_run = True
                    bar_run = True
                elif arg == '-p':
                    primal_run = True
                elif arg == '-d':
                    dual_run = True
                elif arg == '-b':
                    bar_run = True
                else:
                    input_file_prefix = arg
        if not primal_run and not dual_run and not bar_run:
            primal_run = True
    except:
        print_help()
        sys.exit()

    output_file_name = input_file_prefix + '_output'
    output_file_location = output_dir + output_file_name
    output_file_cplex_location = output_file_location + '_cplex'
    
    cplex_files = os.listdir(cplex_input_dir)

    for cplex_file in cplex_files:
        if cplex_file.startswith(input_file_prefix) and not cplex_file.startswith('.'):
            print 'CPLEX: ', cplex_file
            if primal_run:
                run_cplex_alg(cplex_file, output_file_cplex_location, 'primopt', 'Primal')
            if dual_run:
                run_cplex_alg(cplex_file, output_file_cplex_location, 'tranopt', 'Dual')
            if bar_run:
                run_cplex_alg(cplex_file, output_file_cplex_location, 'baropt', 'Barrier')

main()
