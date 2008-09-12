import random
import os
import sys

def main():

    #------- INPUT FILE PARAMETERS - BEGIN -------------
    #total number of input sets to be generated (each set has dif num of rows/cols)
    input_sets = 8
    
    #no of rows, r, starts in the range 10* [row_min, row_max] and increases by factor of 10 for each run
    row_min = 500
    row_max = 600
    #no of columns, c, is in the range [col_min, col_max]    
    col_min = 500
    col_max = 600

    #range of densities for each matrix size
    densities = [0.2, 0.4]
    
    #the range for coefficients in an input are in range [min_lower,max_upper]
    lower = 0
    upper = 100

    random.seed()

    try:
        args = sys.argv
        input_file_prefix = args[1]
    except:
        print 'Did not input argument so using default file name'
        input_file_prefix = 'input_test_m'

    #------- INPUT FILE PARAMETERS - END -------------    

    test_row = random.randint(row_min, row_max)
    test_col = random.randint(col_min, col_max)

    #get absolute path to generate_input script
    full_pathname = args[0]
    pathname = full_pathname[:full_pathname.rfind('/')+1]

    for i in range(input_sets):
        #set up parameters for the test
        test_row += 500
        test_col += 500

        for d in densities:
            #create input file
            cmd_create = 'python ' + pathname + 'generate_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(d) + ' ' + str(lower) + ' ' + str(upper) + ' ' + input_file_prefix
            os.system(cmd_create)
            
main()
