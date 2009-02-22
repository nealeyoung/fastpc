import sys
eps = 0.5

in_file = open(sys.argv[1],'r')
out_file = open(sys.argv[2],'w')

packing = []
covering = []

first_line= [int(x) for x in in_file.readline().split()]
num_cons,num_vars,num_coeff = first_line[0],first_line[1],first_line[2]




def madd(line,old_line):
    
    line_new = line.split()    
    line_new[2] = str(float(line.split()[2]) + float(old_line.split()[2]))
    return ' '.join(line_new) + '\n'

#old_line = in_file.readline()
#old = old_line.split()[0]

#one_done = False

for line in in_file:

    #if old != line.split()[0]:
    #    if not one_done:            
    #        packing.append(old_line)
    #        covering.append(old_line)

    #    old_line = line
    #    old = line.split()[0]
    #    one_done = False
    #    continue

        
    #if not one_done:
    #    packing.append(line)
    #    covering.append(madd(line,old_line))
    #else:
#	packing.append(line)
#        covering.append(line)

    packing.append(line)
    covering.append(line)
    one_done = True

covering = covering[0:len(covering)-num_vars]
packing = packing[0:len(packing)-num_vars]

num_cov = num_cons-num_vars
num_pack = num_cons-num_vars

out_file.write(str(eps) + ' ' + str(num_pack) + ' ' +str(num_cov) + ' ' +str(num_vars) + '\n')

out_file.write('covering_coeffs\n')

for item in covering:
    out_file.write(item)


out_file.write('covering_constraint_values\n')

for i in range(num_cov):
    out_file.write('1\n')


out_file.write('packing_coeffs\n')

for item in packing:
    out_file.write(item)

out_file.write('packing_constraint_values\n')


for i in range(num_pack):
    out_file.write('1\n')



