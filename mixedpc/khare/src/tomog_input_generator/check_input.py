#!/usr/bin/env python

import sys
from PIL import Image

img_name = sys.argv[1]

img_name_no_ext = img_name[:img_name.rindex('.')]

img = Image.open('images/' + img_name)
(w, h) = img.size

m = w * h

x = map(int, open('images/' + img_name_no_ext + '_actual_pixel_values').read().split())

input = open('glpk_inputs/' + img_name_no_ext + '_input')

found_error = False
for line in input:
    if line.startswith('con'):
        con = line.replace('+ ', '').split()
        i = 1
        l = len(con)
        value = 0
        while i < l - 2:
            coeff = float(con[i]) 
            index = int(con[i+1].replace('x', ''))
            if index > m:
                value += coeff
            else:
                value += coeff * x[index]
            i += 2
        check_str = str(value) + ' ' + con[i] + ' ' + con[i+1]
        check_str = check_str.replace(' = ', ' == ')
#        print check_str
        if not eval(check_str):
            found_error = True
            print 'Error:'
            print '   ', con
            print '   ', check_str

if not found_error:
    print 'All constraints are valid.'
