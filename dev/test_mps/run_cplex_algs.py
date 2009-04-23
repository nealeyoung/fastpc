import sys
import os

def print_help():
    print "Usage: python run_cplex_algs.py [input_file_prefix] [-a] [-p] [-d] [-b]"
    print "Order of arguments is not relevant."
    print "input_file_prefix: specifies the prefix to look for in the file names. If none specified, runs cplex on all input files in the cplex input directory."
    print "-a: Run all cplex algorithms."
    print "-p: Run Primal method."
    print "-d: Run Dual method."
    print "-b: Run Barrier method."

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
    output_dir = 'output_cplex/'
    output_file_location = output_dir + output_file_name
    output_file_cplex_location = output_file_location + '_cplex'
    
    curr_dir = os.getcwd()
    cplex_input_dir = curr_dir + '/test_cases_glpk/'
    cplex_files = os.listdir(cplex_input_dir)

    for cplex_file in cplex_files:
        if cplex_file.startswith(input_file_prefix) and not cplex_file.startswith('.'):
            print 'CPLEX: ', cplex_file

            # Put in commands for running various algorithms
            cplex_command_input = 'cplex_command_input'
            file = open(cplex_command_input, 'w')

            if primal_run:
                print "    cplex will run Primal"
                file.write('read ' + cplex_input_dir + cplex_file + ' \n')
                file.write('lp \n')
                file.write('primopt \n')
                file.write('write ' + output_dir + cplex_file + '_cplex_primal_out.sol \n')
            if dual_run:
                print "    cplex will run Dual"
                file.write('read ' + cplex_input_dir + cplex_file + ' \n')
                file.write('lp \n')
                file.write('tranopt \n')
                file.write('write ' + output_dir + cplex_file + '_cplex_dual_out.sol \n')
            if bar_run:
                print "    cplex will run Barrier"
                file.write('read ' + cplex_input_dir + cplex_file + ' \n')
                file.write('lp \n')
                file.write('baropt \n')
                file.write('write ' + output_dir + cplex_file + '_cplex_bar_out.sol \n')
            
            file.close()
            
            cplex_cmd = '../../../cplex111/bin/x86_debian4.0_4.1/cplex >> ' + output_file_cplex_location + ' < ' + cplex_command_input
            os.system(cplex_cmd)

main()
