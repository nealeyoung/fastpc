import random
#import math
import os
import sys

def main():

    #------- INPUT FILE PARAMETERS - BEGIN -------------
    #total number of input sets to be generated (each set has dif num of rows/cols)
    input_sets = 4
    
    #no of rows, r, starts in the range 10* [row_min, row_max] and increases by factor of 10 for each run
    row_min = 3
    row_max = 10
    #no of columns, c, is in the range [col_min, col_max]    
    col_min = 3
    col_max = 10

    #range of densities for each matrix size
    densities = [0.2, 0.4, 0.6, 0.8]
    
    #the range for coefficients in an input are in range [min_lower,max_upper]
    lower = 0
    upper = 100

    try:
        args = sys.argv
        input_file_prefix = args[1]
    except:
        print 'did not input argument so using default file name'
        input_file_prefix = 'input_test_m'

    #------- INPUT FILE PARAMETERS - END -------------    

    test_row = random.randint(row_min, row_max)
    test_col = random.randint(col_min, col_max)

    for i in range(input_sets):
        #set up parameters for the test
        test_row *= 10
        test_col *= 10

        for d in densities:
            #create input file
            cmd_create = 'python generate_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(d) + ' ' + str(lower) + ' ' + str(upper) + ' ' + input_file_prefix
            os.system(cmd_create)
            
main()
