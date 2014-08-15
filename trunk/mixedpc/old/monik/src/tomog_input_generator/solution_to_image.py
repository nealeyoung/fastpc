#!/usr/bin/env python

from PIL import Image
import sys, os
import math
import re

def name_no_format(img_name):
    return img_name[:img_name.rfind('.')]

def variable_index(v, h):
    i = int(math.floor(v/h))
    j = int(v - h*i)
    return (i, j)

def mixedpc_sol_to_img(img_name, img_dir, mixedpc_dir):
    img_name_no_format = name_no_format(img_name)
    img = Image.open(img_dir + img_name).convert('L')
    (w, h) = img.size
    pixels = img.load()

    # number of variables
    num_vars = w * h

    files = os.listdir(mixedpc_dir)
    for file_name in files:
#        print file_name
        if file_name.startswith(img_name_no_format) and file_name.endswith('_out'):
            print '  Generating image for mixedpc solution file: ', file_name
            sol_file = open(mixedpc_dir + file_name)
            sol_line = False
            x = []
            for line in sol_file:
                if sol_line:
                    x = map(lambda x: float(x), line.split())
                    break
                if line.startswith('Variable values'):
                    sol_line = True
            if len(x) < num_vars:
                print '    Error:Solution infeasible, so no image generated.'
                continue
            for var in range(num_vars):
                (i, j) = variable_index(var, h)
                pixels[(i, j)] = x[var]
            img.save(mixedpc_dir + file_name + '.png', 'PNG')
            print '    Done'

def glpk_sol_to_img(img_name, img_dir, glpk_dir):
    glpk_sol_file_name = glpk_dir + name_no_format(img_name) + '_input_glpk_sol'
    print '  Generating image for glpk solution file: ', glpk_sol_file_name
    try:
        glpk_sol = open(glpk_sol_file_name).read()
    except:
        print '    glpk file not found: ', glpk_sol_file_name
        return
    var_row_re = re.compile('\s*\d+\sx\d+\s.*')
    var_rows = var_row_re.findall(glpk_sol)
    img = Image.open(img_dir + img_name).convert('L')
    pixels = img.load()
    (w, h) = img.size
    num_vars = w * h
    for row in var_rows:
        cols = row.split()
        var = int(cols[1].replace('x', ''))
        if var >= num_vars:
            continue
        try:
            (i, j) = variable_index(var, h)
            pixels[i, j] = float(cols[3])
        except:
            print '    Error: GLPK solution not correct'
            return
    img.save(glpk_dir + name_no_format(img_name) + '_glpk.png', 'PNG')
    print '    Done'

def cplex_sol_to_img(img_name, img_dir, glpk_dir):
    print '  Generating images for cplex solution files'
    algs = ['primopt', 'tranopt', 'baropt']
    cplex_file_prefix = glpk_dir + name_no_format(img_name)
    for alg in algs:
        file_name = cplex_file_prefix + '_input_' + alg + '_cplex.sol'
        try:
            sol_file = open(file_name)
        except:
            print '    cplex file now found: ', file_name
            continue
        print '    Generating image for ', file_name
        for line in sol_file:
            temp = line.strip()
            img = Image.open(img_dir + img_name).convert('L')
            (w, h) = img.size
            num_vars = w * h
            pixels = img.load()
            if temp.startswith('<variable '):
                arr = temp.split()
                var = int(arr[1].split('"')[1].replace('x', ''))
                if var >= num_vars:
                    continue
                (i, j) = variable_index(var, h)
                value = float(arr[4].split('"')[1].replace('x', ''))
#                print var, value
                pixels[(i, j)] = value
        img.save(glpk_dir + name_no_format(img_name) + '_' + alg + '.png')

def main():
    args = sys.argv
    if len(args) < 5:
        print 'Usage: python solution_to_image.py <run_identifier> <img_dir> <mixedpc_dir> <glpk_dir>'
        return
    run_identifier = args[1]
    img_dir = args[2]
    mixedpc_dir = args[3]
    glpk_dir = args[4]
    img_files = os.listdir(img_dir)
    for img_name in img_files:
        if img_name.startswith(run_identifier) and not img_name.endswith('values'):
            mixedpc_sol_to_img(img_name, img_dir, mixedpc_dir)
            glpk_sol_to_img(img_name, img_dir, glpk_dir)
            cplex_sol_to_img(img_name, img_dir, glpk_dir)

main()
