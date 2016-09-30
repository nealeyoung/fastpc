#!/usr/bin/env python2.6

import sys

last_line = 0
for line in sys.stdin:
    line = line.strip()
    fields = line.split(",")
    if fields[4] == '0.0': 
        try:
            fields = last_line.split(",")
            info = fields[0].split("_")
            (x,year,r,c,d,alg) = info[-6:]
            eps = float(fields[4])+float(fields[5])
            rows_plus_cols = float(r)+float(c)
            print "\t".join(str(x) for x in fields + [r,c,d,eps])
        except:
            print "ERROR:", line
            raise
    last_line = line


    
