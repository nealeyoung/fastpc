#generate random sparse lp matrices
#limits the number of non-zeroes in each row
#execpt in the last row where non-zeroes are added to make sure that no column is empty

import random
import sys
import math
import os
import shutil
import sets

def main() :
    
    non_zeros_actual = 0
    error = False
    try:
        args = sys.argv
        full_pathname = args[0]
        pathname = full_pathname[:full_pathname.rfind('/')+1]
        fp_dir = pathname + '../test_cases/'
        glpk_dir = pathname + '../test_cases_glpk/'
        r = int(args[1])
        c = int(args[2])
        non_zeroes_per_row = int(args[3])
        non_zero = r*non_zeroes_per_row
        lower = int(args[4])
        upper  = int(args[5])
        out_file_prefix = args[6]
        glpk_file_name = out_file_prefix + '_glpk'
    except :
        error = True

    if(error):
        print "Arguments expected: <rows> <columns> <non zeroes per row> <coeff lower> <coeff upper> <file name prefix>"
    else:
        #open files to write
        
        file = open(fp_dir + out_file_prefix,'w')
        glpk_file = open(glpk_dir + glpk_file_name,'w')

        # Enter spaces in the first line
        # This is required because the final density and number of non-zeros can be different
        # from what was in the argument. Once the file is prepared, the density and non-zeros
        # are recalculated and the first line of the file and the name are updated accordingly
        my_string = str(r) +' ' + str(c)+ ' ' + str(non_zero)
        my_spaces = reduce(lambda a,b: a+b, [' ']*(len(my_string)+7))
        file.write( my_spaces + '\n')
        
        #write entries in GLPK file
        glpk_file.write('Maximize \n')
        glpk_file.write('value: ')
        for x in range(c-1):
            glpk_file.write('1.0  var' + str(x) + ' + ')
        glpk_file.write('1.0  var' + str(c-1) + ' \n')
        glpk_file.write('Subject To \n')

        written= False
        row_count = 0
        empty_cols = [True]*c
        for rows in range(r):
            row_string = ''
            cols = set()
            first = True
            while len(cols) < non_zeroes_per_row:
                col = int(random.uniform(0,c))
                if col in cols:
                    continue
                cols.add(col)
                empty_cols[col] = False
                value = random.uniform(lower,upper)
                non_zeros_actual = non_zeros_actual + 1
                if first:
                    row_string ='row' + str(row_count) + ':  '
                if not first:
                    row_string = row_string + ' + '
                first = False
                written = True
                row_string = row_string +str(value) +  ' var' + str(col) + ' '
                file.write(str(rows) +' ' + str(col)+ ' ' + str(value) + '\n')

            if rows == r-1:
                print_warning = True
                for col, is_empty in enumerate(empty_cols):
                    if is_empty:
                        if print_warning:
                            print "WARNING: Following columns intentionally were made non zero in the last row for the sake of feasibility: "
                            print "File (r, c, non zeroes): " + my_string
                            print_warning = False
                        print col,
                        empty_cols[col] = False
                        non_zeros_actual = non_zeros_actual + 1
                        value = random.uniform(lower, upper)
                        file.write(str(rows) +' ' + str(col)+ ' ' + str(value) + '\n')
                        row_string = row_string +str(value) +  ' var' + str(col) + ' '

            if  written:
                glpk_file.write(row_string +' < 1 \n')
                written = False
                row_count = row_count + 1
                
        for col, is_empty in enumerate(empty_cols):
            if is_empty: print col

        glpk_file.write('Bounds \n')
        for x in range(c):
            glpk_file.write('0 < var' +str(x)+ ' \n')

        #overwrite first line of fastpc file to reflect true nonzeros 
        file.seek(0)
        file.write(str(r)+' ' +str(c)+ ' '+ str(non_zeros_actual))

        glpk_file.write('End \n')
        glpk_file.close()
        file.close()

        d_str = str(float(non_zeros_actual)/(float(r)*float(c)))
        name_list =[out_file_prefix,str(r),str(c), d_str[0:5]]
        input_file_name = '_'.join(name_list)
        os.rename(fp_dir + out_file_prefix, fp_dir + input_file_name)
        os.rename(glpk_dir + glpk_file_name, glpk_dir + input_file_name + '_glpk')

main()
