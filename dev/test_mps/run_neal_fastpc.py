import sys
import os

def print_help():
    print "Usage: python run_tests.py [input_file_prefix]"

def main():
    try:
        args = sys.argv
        l = len(args)
        if l > 1:
            input_file_prefix = args[1]
    except:
        print_help()
        exit()

    print ''

    # epsilons to be run for each input file generated        
    epsilons = [0.05, 0.01]
    
    output_file_name = input_file_prefix + '_output_neal'
    output_dir = 'output/'
    output_file_location = output_dir + output_file_name
    
    curr_dir = os.getcwd()
    fp_input_dir = curr_dir + '/test_cases/'

    fastpc_files = os.listdir(fp_input_dir)
    exec_cmd = '../../neal/eclipse/fastpc/src/fastpc '

    for fp_file in fastpc_files:
        if fp_file.startswith(input_file_prefix) and not fp_file.startswith('.'):
            #for each file run test for different values of eps
            for eps in epsilons:
                print 'fastpc: ', eps, ' ', fp_file
                cmd_exact = exec_cmd + ' ' + str(eps) + ' ' + fp_input_dir + fp_file + ' >> ' + output_file_location
                #print cmd_exact
                os.system(cmd_exact)

main()
