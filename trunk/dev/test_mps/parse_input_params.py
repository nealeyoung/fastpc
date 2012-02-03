#!/usr/bin/env python

import sys
import re
import os

dict = {}

def parse(p):
    data_dir = 'output/'
    fastpc = data_dir + p + '_output_fastpc'
    neal = data_dir + p + '_output_neal'
    fp = True
    try:
        data_file = open(fastpc)
    except:
        try:
            data_file = open(neal)
        except:
            data_file = None
            print 'Could not find fastpc/neal file for prefix: ', p

    if data_file != None:
        for line in data_file:
            if line.startswith('INPUT FILE:'):
#                print line
                identifier = p + '#' + line.split('/')[-1].replace('\n', '')
                if not identifier in dict:
                    write = True
            elif line.startswith('ROWS:'):
                arr = line.split()
#                print arr
                if write:
                    val = arr[1] + "#" + arr[3] + "#" + arr[5] + "#" + arr[7]
#                    print val
                    dict[identifier] = val
                    write = False

def main():
    prefix_file = open('prefixes_to_parse.txt')

    prefixes = []
    for line in prefix_file:
        prefixes.append(line.split()[0])
    print 'Parsing input parameters for prefixes: ', prefixes

    for p in prefixes:
        parse(p)


    parsed_data_file = open('parsed_input_params.txt', 'w')
    for key in dict.iterkeys():
#        print key, ' --> ', dict[key]
        parsed_data_file.write(key + '$' + dict[key] + '\n')

main()
