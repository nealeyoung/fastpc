import random
import os
import sys

def main():

    #------- INPUT FILE PARAMETERS - BEGIN -------------
    #total number of input sets to be generated (each set has dif num of rows/cols)
    input_sets = 4
    
    #no of rows, r, starts in the range 10* [row_min, row_max] and increases by factor of 10 for each run
    row_min = 700
    row_max = 750
    #no of columns, c, is in the range [col_min, col_max]    
    col_min = 700
    col_max = 750

    #range of densities for each matrix size
    densities = [0.3, 0.5]
    
    #the range for coefficients in an input are in range [min_lower,max_upper]
    lower = 1
    upper = 1

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

    for i in range(input_sets):
        #set up parameters for the test
        test_row += 200
        test_col += 200

        for d in densities:
            #create input file
            cmd_create = 'python generate_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(d) + ' ' + str(lower) + ' ' + str(upper) + ' ' + input_file_prefix
            os.system(cmd_create)
            
main()
