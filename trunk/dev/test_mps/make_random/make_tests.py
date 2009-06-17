import random
import os
import sys

def main():

    #------- INPUT FILE PARAMETERS - BEGIN -------------
    # Total number of input sets to be generated (each set has dif num of rows/cols)
    input_sets = 12
    
    # Number of rows, r, starts in the range 10* [row_min, row_max] and increases by factor of 10 for each run
    row_min = 500
    row_max = 550
    # Number of columns, c, is in the range [col_min, col_max]    
    col_min = 900
    col_max = 950
    # Increment
    row_increment = 800
    col_increment = 400

    # Densities for each matrix size
    # These will be considered only when the sparse option below is set to 'False'
    #densities = [0.01, 0.1, 0.4]
    densities = [0.1, 0.5]
    
    # The range for coefficients in an input are in range [min_lower,max_upper]
    lower = 0
    upper = 100

    # Turn this option to 'True' to generate very sparse matrices with a constant number of non-zeroes in each row
    # This method is more suitable for reasonably large matrices
    # If you turn this option on, the denisities defined above will be ignored
    # SHIFT THIS OPTION TO ARGUMENTS!!
    sparse = True

    # Number of non zeroes in each row
    # It will be considered only when the sparse option is set to 'True'
    max_per_row = 12

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
        test_row += row_increment
        test_col += col_increment

        if not sparse:
            for d in densities: #create input file
                cmd_create = 'python ' + pathname + 'generate_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(d) + ' ' + str(lower) + ' ' + str(upper) + ' ' + input_file_prefix
                os.system(cmd_create)
        else:
            cmd_create = 'python ' + pathname + 'generate_sparse_input.py ' + ' ' + str(test_row) + ' ' + str(test_col) + ' ' + str(max_per_row) + ' ' + str(lower) + ' ' + str(upper) + ' ' + input_file_prefix
            os.system(cmd_create)
            
main()
