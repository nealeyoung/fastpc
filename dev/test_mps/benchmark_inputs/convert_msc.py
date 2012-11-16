#!/usr/bin/env python

import sys, os

usage = 'python convert_msc.py <msc_file_path>'

try:
    msc_path = sys.argv[1]
    msc_file = open(msc_path)
except:
    print usage
    exit()

E = {}

set_num = 0
entries = 0
for line in msc_file:
    if line.startswith('p'):
        arr = line.split()
        U = arr[2]
        S = arr[3]
    elif line.startswith('s'):
        arr = line.split()
        for i in xrange(1, len(arr)):
            e = arr[i]
            M = E.get(e, [])
            M.append(set_num)
            E[e] = M
            entries += 1
        set_num += 1

base_name = msc_path[msc_path.rindex('/') + 1 : msc_path.index('.msc')]
fastpc_input = base_name + '_input'
fastpc = open(fastpc_input, 'w')

header = U + ' ' + S + ' ' + str(entries) + '\n'
fastpc.write(header)

con = 0
for e in iter(E):
    for s in E[e]:
        fastpc.write(str(con) + ' ' + str(s) + ' 1\n')
    con += 1

fastpc.close()

os.system('python ../convert_fastpc_to_glpk.py ' + fastpc_input + ' ' + fastpc_input + '_glpk')

msc_file.close()
