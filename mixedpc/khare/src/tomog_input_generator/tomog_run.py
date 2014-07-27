#!/usr/bin/env python

import sys
import os
import ConfigParser

env='monik_linux'
python_path=''
cplex_path=''
glpk_path=''

def do_setup(env):
    global python_path, cplex_path, glpk_path
    Config = ConfigParser.ConfigParser()
    Config.read('env.properties')
    python_path = Config.get(env, 'python')
    cplex_path = Config.get(env, 'cplex')
    glpk_path = Config.get(env, 'glpk')

def main():
    args = sys.argv
    if len(args) < 2:
        print 'Usage: python tomog_run.py <run_identifier> [env]'
        print '\t run_identifier: prefix for the image files to be used for the run'
        print '\t env: one of the environments defined in env.properties file. This is an optional argument.'
        return
    run_identifier = args[1]

    global env
    if len(args) > 2:
        env = args[2]
    else:
        print 'Using default enviroment:', env

    do_setup(env)

    curr_dir = os.getcwd()

    img_dir = curr_dir + '/images/'
    mixedpc_input_dir = curr_dir + '/mixedpc_inputs/'
    glpk_input_dir = curr_dir + '/glpk_inputs/'
    cplex_algs = ['primopt', 'tranopt', 'baropt']

    dir_str = img_dir + ' ' + mixedpc_input_dir + ' ' + glpk_input_dir

    # generate inputs for mixedpc and glpk/cplex
    print 'Generating inputs for ', run_identifier
    gen_input_cmd = python_path + ' gen_tomog_input.py ' + run_identifier + ' ' + dir_str
    os.system(gen_input_cmd)
    print 'Done.'
    
    # run solvers
    print 'Running solvers for ', run_identifier

    eps_list = [0.1, 0.05, 0.01]
    mixedpc_path = '../mxpc'
    mixedpc_outfile = mixedpc_input_dir + run_identifier + '_mixedpc_dump'
    os.system('echo > ' + mixedpc_outfile)

    glpk_exists = os.access(glpk_path, os.F_OK)
    glpk_outfile = glpk_input_dir + run_identifier + '_glpk_dump'
    os.system('date > ' + glpk_outfile)
    if not glpk_exists:
        print '  WARNING: GLPK not found at the specified location.'
        print ''
        os.system('echo GLPK not found at the specified location >> ' + glpk_outfile)

    cplex_outfile = glpk_input_dir + run_identifier + '_cplex_dump'
    cplex_exists = os.access(cplex_path, os.F_OK)
    os.system('date > ' + cplex_outfile)
    if not cplex_exists:
        print '  WARNING: CPLEX not found at the specified location'
        print ''
        os.system('echo CPLEX not found at the specified location >> ' + cplex_outfile)

    input_files = os.listdir(mixedpc_input_dir)
    glpk_files = os.listdir(glpk_input_dir)
    for file_name in input_files:
        if file_name.startswith(run_identifier) and file_name.endswith('_input'):
            # mixedpc
            for eps in eps_list:
                print '  Running mxpc on ', file_name, ' with eps = ', eps
                mixedpc_cmd = mixedpc_path + ' ' + str(eps) + ' ' + mixedpc_input_dir + file_name + ' >> ' + mixedpc_outfile
                os.system(mixedpc_cmd)

            glpk_input = file_name.replace('mixedpc', 'glpk')
            if glpk_input in glpk_files:

                # glpk
                if glpk_exists:
                    glpk_sol = glpk_input + '_glpk_sol'
                    glpk_out = glpk_input + '_glpk_out'
                    print '  Running glpk on ', glpk_input
                    glpk_cmd = glpk_path + ' --lp ' + glpk_input_dir + glpk_input + ' -o ' + glpk_input_dir + glpk_sol + ' >> ' + glpk_outfile
                    os.system(glpk_cmd)

                # cplex
                if cplex_exists:
                    for cplex_alg in cplex_algs:
                        print '  Running cplex (', cplex_alg,') on ', glpk_input
                        cplex_temp_input = 'cplex_input'
                        file = open(cplex_temp_input, 'w')
                        file.write('read ' + glpk_input_dir + glpk_input + '\n')
                        file.write('lp\n')
                        file.write(cplex_alg + '\n')
                        file.write('write ' + glpk_input_dir + glpk_input + '_' + cplex_alg + '_cplex.sol\n')
                        file.write('y\n')
                        file.close()
                        cplex_cmd = cplex_path + ' >> ' + cplex_outfile + ' < ' + cplex_temp_input
                        os.system(cplex_cmd)

    print 'Finished running solvers for ', run_identifier

    # generate images from solutions
    generate_images = True
    if generate_images:
        print 'Generating images from solutions'
        sol_to_img_cmd = python_path + ' solution_to_image.py ' + run_identifier + ' ' + dir_str
        os.system(sol_to_img_cmd)
        return

main()
