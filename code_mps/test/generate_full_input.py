import random


c = 50
r  = 50
non_zero = 1000.0
upper  = 100.0
lower = 0.0
out_file_name = 'lp_test_sort_3'
glpk_file_name = 'glpk_test'

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
    

    

#print stuff so we can use it

for entry in entries:
    file.write(str(entry[0]) +' ' + str(entry[1])+ ' ' + str(entry[2]) + '\n')

    
row_array =[[]]*r

    



#print stuff so glpk can use it

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


