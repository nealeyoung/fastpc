#generate random lp matrices

import random
import sys
import math
import os
import shutil

def random_i(p):
    '''Return non-negative integer i from distribution p_i = p(1-p)^i.
    That is, i is the number of trials until an event w/ probability p occurs.'''
    # Method: select min i such that p_0+p_1+...+p_i >= r (r uniform in [0,1])
    # the sum is 1 - p_{i+1} * 1/(1-(1-p))
    # so equiv to select min i such that p_{i+1}/p <= r
    # equiv to (1-p)^(i+1) <= r
    # (i+1) log(1-p) <= log(r)
    # i >= log(r)/log(1-p) - 1
    if p == 1: return 0
    return int(math.ceil(math.log(random.random())/math.log(1-p))) - 1

def main() :
    fp_dir = './test_cases/'
    glpk_dir = './test_cases_glpk/'
    v07_dir = './test_cases_v07/'
    max_write_rows = 500  #periodically write output to save memory
    non_zeros_actual = 0
    error = False
    try:
        args = sys.argv
        r = int(args[1])
        c = int(args[2])
        density = float(args[3])
	non_zero = int(density*r*c)
        lower = int(args[4])
        upper  = int(args[5])
        out_file_prefix = args[6]
        glpk_file_name = out_file_prefix + '_glpk'
        p = float(non_zero)/(float(r)*float(c))
    except :
        error = True

    if(error):
        print "Arguments expected: <rows> <columns> <density> <coeff lower> <coeff upper> <file name prefix>"
    else:
        #open files to write
        
        file = open(fp_dir + out_file_prefix,'w')
        glpk_file = open(glpk_dir + glpk_file_name,'w')

        # Enter spaces in the first line
        # This is required because the final density and number of non-zeros can be different
        # from what was in the argument. Once the file is prepared, the density and non-zeros
        # are recalculated and the first line of the file and the name are updated accordingly
        my_string = str(r) +' ' + str(c)+ ' ' + str(non_zero)
        my_spaces = reduce(lambda a,b: a+b, [' ']*(len(my_string)+5))
        file.write( my_spaces + '\n')

        #write entries in GLPK file
        glpk_file.write('Maximize \n')
        glpk_file.write('value: ')
        for x in range(c-1):
            glpk_file.write('1.0  var' + str(x) + ' + ')
        glpk_file.write('1.0  var' + str(c-1) + ' \n')
        glpk_file.write('Subject To \n')

        #generate random input 
        exact = False

        #exact method, slow for sparse matrices
        if exact:
            slots = r*c
            for rows in range(r):
                glpk_file.write('row' + str(rows) + ':  ')
                first = True
                for cols in range(c):
                    if random.random()*slots <= nonzero:
                        value = random.uniform(lower,upper)
                        non_zeros_actual = non_zeros_actual + 1
                        if not first:
                            glpk_file.write(' + ')
                        glpk_file.write(str(value) +  ' var' + str(cols) + ' ')
                        file.write(str(rows) +' ' + str(cols)+ ' ' + str(value) + '\n')
                        nonzero -= 1
                        first = False
                    slots -= 1
                glpk_file.write(' < 1 \n')
        else:
        #inexact method, faster for sparse matrices
            for rows in range(r):
                glpk_file.write('row' + str(rows) + ':  ')
                col = 0
                first = True
                while True:
                    col += random_i(p)
                    if col >= c:
                        break
                    value = random.uniform(lower,upper)
                    non_zeros_actual = non_zeros_actual + 1
                    if not first:
                        glpk_file.write(' + ')
                    glpk_file.write(str(value) +  ' var' + str(col) + ' ')
                    file.write(str(rows) +' ' + str(col)+ ' ' + str(value) + '\n')
                    col += 1
                    first = False
                glpk_file.write(' < 1 \n') #format of end of each constraint
                
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
        name_list =[out_file_prefix,str(r),str(c), d_str[0:4]]
        input_file_name = '_'.join(name_list)
        os.rename(fp_dir + out_file_prefix, fp_dir + input_file_name)
        os.rename(glpk_dir + glpk_file_name, glpk_dir + input_file_name + '_glpk')

        #copy fastpc (mps version) to v07
        v07_file_name = v07_dir + input_file_name + '_v07'
        shutil.copy(fp_dir + input_file_name, v07_file_name)
        #convert the file to v07 format
        cmd_convert = 'convert_codemps_v2007 ' + v07_file_name
        os.system(cmd_convert)
    
main()
