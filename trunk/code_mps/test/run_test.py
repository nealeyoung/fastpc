import random
import math
import os

def create_input_file(r, c, non_zero, lower, upper, file_name):
    #setup parameters
    error = False
    glpk_file_name = 'glpk_' + file_name

    #open files to write
    glpk_file = open(glpk_file_name,'w')
    file = open(file_name,'w')

    file.write(str(r) +' ' + str(c)+ ' ' + str(non_zero) + '\n')

    population = []
    for x in range(r):
        for y  in range(c):
           population.append( [x,y])

    entries = random.sample(population,int(non_zero))
    for index, entry in enumerate(entries):
        entries[index].append(random.uniform(lower,upper))

    #write entries in file
    for entry in entries:
        file.write(str(entry[0]) +' ' + str(entry[1])+ ' ' + str(entry[2]) + '\n')

    row_array =[[]]*r

    #write entries in GLPK file
    glpk_file.write('Maximize \n')
    glpk_file.write('value: ')
    for x in range(c-1):
        glpk_file.write('1.0  var' + str(x) + ' + ')
    glpk_file.write('1.0  var' + str(c-1) + ' \n')
    glpk_file.write('Subject To \n')

    row_array = [ ['row' + str(index) + ':  ']  for index in range(r)]

    for entry in entries:
        row_array[entry[0]].append( ' '+ str(entry[2]) +  ' var' + str(entry[1]) + ' +')

    for row in row_array:
        for item in row[:-1]:
            glpk_file.write(item)
        glpk_file.write(row[-1][:-1] + ' < 1 \n')
    glpk_file.write('Bounds \n')
    glpk_file.write('End \n')

def main():

    max_inputs = 2
    max_runs_per_input = 2

    row_min = 10
    row_max = 500
    col_min = 30
    col_max = 500
    fraction_nonzero_low = 0.1
    fraction_nonzero_high = 0.9
    max_lower = 0
    max_upper = 100

    eps_low = 0.001
    eps_high = 0.1

    sort_ratio = 1 #change this to more than 1 for approximate sorting

    input_file_prefix = 'input_test_m'
    output_file_name = 'output_test_m'
    output_file = open(output_file_name, 'w')

    for i in range(max_inputs):
        #set up parameters for the test
        test_row = random.randint(row_min, row_max)
        test_col = random.randint(col_min, col_max)
        test_total = test_row*test_col
        test_nonzero = random.randint(math.ceil(fraction_nonzero_low*test_total), math.floor(fraction_nonzero_high*test_total))
        test_lower = random.randint(max_lower, max_upper/2)
        test_upper = random.randint(test_lower, max_upper)
        test_input_file = input_file_prefix + "_"  + str(i)
        
        create_input_file(test_row, test_col, test_nonzero, test_lower, test_upper, test_input_file)
        
        #for each file run test for different values of eps
        for j in range(max_runs_per_input):
            #run the test and store the output
            test_eps = random.uniform(eps_low, eps_high)

            cmd = "../fastpc" + ' ' + str(test_eps) + ' ' + test_input_file + ' ' + str(sort_ratio) + " >> " + output_file_name

            os.system(cmd)
            
    output_file.close()

main()
