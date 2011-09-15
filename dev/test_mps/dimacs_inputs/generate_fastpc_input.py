#!/usr/bin/env python

import sys
import os

usage = './generate_fastpc_input.py <file.col> <fastpc_file>'

args = sys.argv
graph = open(args[1])
fastpc = open(args[2], 'w')

num_nodes = 0
num_edges = 0

edges = set()

count = 0

def dummy(u):
    return u + num_nodes

def write_entries(e):
    global count
    fastpc.write(str(count) + ' ' + str(e[0]) + ' 1\n')
    fastpc.write(str(count) + ' ' + str(e[1]) + ' 1\n')
    count += 1

def write_constraint(e):
    u = e[0]
    u_dummy = dummy(u)
    v = e[1]
    v_dummy = dummy(v)
    write_entries((u, v_dummy))
    write_entries((u_dummy, v))

for line in graph:
    arr = line.split()
    if arr[0] == 'p':
        num_nodes = int(arr[2])
        num_edges = int(arr[3])
        header = str(num_edges * 2) + ' ' + str(num_nodes * 2) + ' ' + str(num_edges * 4) + '\n'
        fastpc.write(header)
    elif arr[0] == 'e':
        u = int(arr[1]) - 1
        v = int(arr[2]) - 1
        if num_edges == 0:
            edges.add((u, v))
        else:
            for e in edges:
                write_constraint(e)
            edges.clear()
            write_constraint((u, v))

graph.close()
fastpc.close()
