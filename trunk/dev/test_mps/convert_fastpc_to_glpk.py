#!/usr/bin/env python

import sys
import os

usage = 'python convert_fastpc_to_glpk <fastpc_file> <glpk_file>'

args = sys.argv

if len(args) < 3:
    print usage

fastpc = open(sys.argv[1])

glpk = open(sys.argv[2], 'w')

(rows, cols, total) = map(int, fastpc.readline().split())

cons = {}

count = 0

error = False

for line in fastpc:
    try:
        (r, c, coeff) = line.split()
        count += 1
        if int(r) > rows:
            print line, ': not valid since row is more than the limit'
            error = True
        if int(c) > cols:
            print line, ': not valid since cols is more than the limit'
            error = True
        if int(count) > total:
            print line, ': not valid since total non-zeroes are more than the limit'
            error = True
        if error:
            print 'Fix the fastpc input and try again'
            exit()
        con = cons.get(r, '')
        con += ' + ' + str(coeff) + ' x' + str(c)
        cons[r] = con
    except:
        print 'Error reading input'
        
glpk.write('Maximize\nvalue:')
obj_func = ''
for c in range(cols):
    obj_func += ' + x' + str(c)

glpk.write(obj_func + '\n')

glpk.write('subject to\n')
for r, con in cons.iteritems():
    con_str = 'con' + str(r) + ':' + con + ' <= 1 \n'
    glpk.write(con_str)

glpk.write('Bounds\n')
for c in range(cols):
    glpk.write('x' + str(c) + ' >= 0 \n')
