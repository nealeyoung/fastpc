import random
import math
import os
import sys

def main():

    #-------RUN PARAMETERS - BEGIN -------------
    #total number of input files to be generated
    max_inputs = 2
    #total number of runs to be done for each input file generated
    max_runs_per_input = 2

    #no of rows, r, is in the range [row_min, row_max]
    row_min = 30
    row_max = 100
    #no of columns, c, is in the range [col_min, col_max]    
    col_min = 30
    col_max = 100

    #total number of non zero's = f*r*c (r = rows, c = columns) where f is in the range [fraction_nonzero_low, fraction_nonzero_high]
    fraction_nonzero_low = 0.1
    fraction_nonzero_high = 0.9

    #the range for coefficients in an input are in range [a,b] such that min_lower <= a <= b <= max_upper
    min_lower = 0
    max_upper = 50

    #eps for a run is in range [eps_low, eps_high]
    eps_low = 0.005
    eps_high = 0.1

    #each run is done with sort_ratio 1 (exact sorting)
    #and also for the sort_ratio defined here (approximate sorting)
    a_sort_ratio = 2 #if set to 0 or 1, only exact sorting run will be done

    input_file_prefix = 'input_test_m'
    output_file_name = 'output_test_m'
    #-------RUN PARAMETERS - END -------------    

    output_file = open(output_file_name, 'w')
    input_config_name = input_file_prefix + '_config'
    sys.stdout = open(input_config_name, 'w')

    for i in range(max_inputs):
        #set up parameters for the test
        test_row = random.randint(row_min, row_max)
        test_col = random.randint(col_min, col_max)
        test_total = test_row*test_col
        test_nonzero = random.randint(math.ceil(fraction_nonzero_low*test_total), math.floor(fraction_nonzero_high*test_total))
        test_lower = random.randint(min_lower, math.ceil((min_lower+max_upper)/2))
        test_upper = random.randint(test_lower, max_upper)
        if (test_upper == 0):
            test_upper = 1
        test_input_file = input_file_prefix + "_"  + str(i)

        #record the parameters for the input file in the input_config_file
        print "Input file name: ", test_input_file
        print "Rows : ", test_row
        print "Columns : ", test_col
        print "Coefficient lower bound: ", test_lower
        print "Coefficient upper bound: ", test_upper, "\n"

        #create input file
        cmd_create = 'python generate_full_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(test_nonzero) + ' ' + str(test_lower) + ' ' + str(test_upper) + ' ' + test_input_file
        os.system(cmd_create)
        #the following line has been commented because instead of the method call we use generate_full_input to create the input file
        #create_input_file(test_row, test_col, test_nonzero, test_lower, test_upper, test_input_file)
        
        #for each file run test for different values of eps
        for j in range(max_runs_per_input):
            #run the test and store the output
            test_eps = random.uniform(eps_low, eps_high)

            #run with sort_ratio = 1 (exact sorting)
            cmd_exact = '../fastpc' + ' ' + str(test_eps) + ' ' + test_input_file + ' 1 >> ' + output_file_name
            os.system(cmd_exact)

            if (a_sort_ratio > 1):
                #run with sort_ratio > 1 (approx sorting)
                cmd_approx = '../fastpc' + ' ' + str(test_eps) + ' ' + test_input_file + ' ' + str(a_sort_ratio) + ' >> ' + output_file_name
                os.system(cmd_approx)
            
    output_file.close()

main()
