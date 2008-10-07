import random
import sys

def main():
    #setup parameters
    error = False
    try:
        args = sys.argv
        r = int(args[1])
        c = int(args[2])
        non_zero = int(args[3])
        lower = int(args[4])
        upper  = int(args[5])
        out_file_name = args[6]
        glpk_file_name = 'glpk_' + out_file_name
    except :
        error = True

    if(error):
        print "Arguments expected: <rows> <columns> <non_zero> <coeff lower> <coeff upper> <file name to generate>"
    else:
        #open files to write
        glpk_file = open(glpk_file_name,'w')
        file = open(out_file_name,'w')

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
        
        for x in range( c):
            glpk_file.write('0 < var' +str(x)+ ' \n')

        glpk_file.write('End \n')
    
main()
